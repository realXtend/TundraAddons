// For conditions of distribution and use, see copyright notice in LICENSE

#include "HttpServer.h"

#include "Framework.h"
#include "CoreDefines.h"
#include "CoreJsonUtils.h"
#include "CoreStringUtils.h"
#include "LoggingFunctions.h"
#include "Profiler.h"

#include "SceneAPI.h"
#include "Scene.h"
#include "Entity.h"
#include "EC_DynamicComponent.h"
#include "QScriptEngineHelpers.h"

#include <websocketpp/frame.hpp>

#include <QMutexLocker>
#include <QByteArray>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>
#include <QVariantList>
#include <QProcess>
#include <QThreadPool>
#include <QFile>
#include <QTime>
#include <QLocale>
#include <QDebug>
#include <QUrl>
#include <QDomDocument>
#include <QDomElement>

#include <algorithm>

Q_DECLARE_METATYPE(HttpServer*)

#ifdef Q_WS_WIN
#include "Win.h"
#else
#include <sys/stat.h>
#include <utime.h>
#endif

#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif

HttpServer::HttpServer(Framework *framework, ushort port) :
    framework_(framework),
    port_(port)
{
}

HttpServer::~HttpServer()
{
    Reset();
}

void HttpServer::Update(float frametime)
{
    if (server_)
    {
        // Update server in main thread so that scenes can be accessed safely in HTTP requests
        PROFILE(ServerPoll);
        server_->get_io_service().poll_one();
    }
}

bool HttpServer::Start()
{
    Reset();
    
    try
    {
        server_ = ServerPtr(new websocketpp::server<websocketpp::config::asio>());

        // Initialize ASIO transport
        server_->init_asio();

        // Register handler callbacks
        server_->set_http_handler(boost::bind(&HttpServer::OnHttpRequest, this, ::_1));

        // Setup logging
        server_->get_alog().clear_channels(websocketpp::log::alevel::all);
        server_->get_elog().clear_channels(websocketpp::log::elevel::all);
        server_->get_elog().set_channels(websocketpp::log::elevel::rerror);
        server_->get_elog().set_channels(websocketpp::log::elevel::fatal);

        server_->listen(port_);

        // Start the server accept loop
        server_->start_accept();
    } 
    catch (std::exception &e) 
    {
        LogError(QString::fromStdString(e.what()));
        return false;
    }
    
    LogInfo("HttpServer started on port " + QString::number(port_));
    emit ServerStarted();
    
    return true;
}

void HttpServer::Stop()
{    
    try
    {
        if (server_)
        {
            server_->stop();
            emit ServerStopped();
        }
    }
    catch (std::exception &e) 
    {
        LogError("Error while closing server: " + QString::fromStdString(e.what()));
        return;
    }
    
    LogDebug("Stopped HttpServer"); 
    
    Reset();
}

void HttpServer::Reset()
{
    server_.reset();
}

Scene* HttpServer::GetActiveScene()
{
    Scene *scene = framework_->Scene()->MainCameraScene();
    if (scene)
        return scene;
    else
        return framework_->Scene()->SceneByName("TundraServer").get();
}

void HttpServer::OnHttpRequest(ConnectionHandle connection)
{
    ConnectionPtr connectionPtr = server_->get_con_from_hdl(connection);

    QString path = QString::fromStdString(connectionPtr->get_resource()).toUtf8();
    QString verb = QString::fromStdString(connectionPtr->get_request().get_method());

    // Handle SceneAPI REST requests internally, otherwise defer to a signal
    if (path.startsWith("/entities") || path.startsWith("/scene"))
        HandleSceneHttpRequest(connectionPtr, path, verb);
    else
        emit HttpRequestReceived(connectionPtr, path, verb);
}

void HttpServer::SetHttpRequestReply(ConnectionPtr connection, const QByteArray& replyData, const QString& contentType, websocketpp::http::status_code::value status)
{
    std::string payload;
    payload.resize(replyData.size());
    memcpy((void*)payload.data(), (void*)replyData.data(), replyData.size());
    
    connection->set_status(status);
    connection->set_body(payload);
    connection->replace_header("Content-Length", QString::number(replyData.size()).toStdString());
    connection->replace_header("Content-Type", contentType.toStdString());
}

void HttpServer::SetHttpRequestReply(ConnectionPtr connection, const QString& reply, const QString& contentType, websocketpp::http::status_code::value status)
{
    QByteArray replyData = reply.toUtf8();
    SetHttpRequestReply(connection, replyData, contentType, status);
}

void HttpServer::SetHttpRequestReply(ConnectionPtr connection, const char* reply, const QString& contentType, websocketpp::http::status_code::value status)
{
    QByteArray replyData(reply);
    SetHttpRequestReply(connection, replyData, contentType, status);
}

void HttpServer::SetHttpRequestStatus(ConnectionPtr connection, websocketpp::http::status_code::value status)
{
    connection->set_status(websocketpp::http::status_code::not_found);
}

void HttpServer::HandleSceneHttpRequest(ConnectionPtr connection, const QString& path, const QString& verb)
{
    Scene* scene = GetActiveScene();
    if (!scene)
    {
        SetHttpRequestStatus(connection, websocketpp::http::status_code::not_found);
        return;
    }

    QUrl pathUrl(path);
    QString sanitatedPath = pathUrl.path();
    if (sanitatedPath.startsWith('/'))
        sanitatedPath = sanitatedPath.mid(1);
    if (sanitatedPath.endsWith('/'))
        sanitatedPath.resize(sanitatedPath.length() - 1);

    QStringList pathParts = sanitatedPath.split('/');

    // Queries
    if (verb.compare("GET", Qt::CaseInsensitive) == 0)
    {
        // Whole scene or entity by name
        if (pathParts.length() == 1)
        {
            if (pathUrl.hasQueryItem("name"))
            {
                QString name = pathUrl.queryItemValue("name");
                EntityPtr entity = scene->EntityByName(name);
                if (entity)
                {
                    QByteArray entityXml;
                    entityXml += entity->SerializeToXMLString(true, true, false);
                    SetHttpRequestReply(connection, entityXml, "application/xml", websocketpp::http::status_code::ok);
                    return;
                }
            }
            else
            {
                QByteArray sceneXml = scene->SerializeToXmlString(true, true);
                SetHttpRequestReply(connection, sceneXml, "application/xml", websocketpp::http::status_code::ok);
                return;
            }
        }
        // Specific entity
        else if (pathParts.length() == 2)
        {
            bool ok = false;
            entity_id_t entityId = pathParts[1].toUInt(&ok);
            EntityPtr entity = scene->EntityById(entityId);
            if (entity)
            {
                QByteArray entityXml;
                entityXml += entity->SerializeToXMLString(true, true, false);
                SetHttpRequestReply(connection, entityXml, "application/xml", websocketpp::http::status_code::ok);
                return;
            }
        }
        // Specific component in specific entity
        else if (pathParts.length() == 3)
        {
            bool ok = false;
            entity_id_t entityId = pathParts[1].toUInt(&ok);
            EntityPtr entity = scene->EntityById(entityId);
            if (entity)
            {
                Entity::ComponentVector comps = entity->ComponentsOfType(pathParts[2]);
                /// \todo Returns only the first component
                if (comps.size())
                {
                    QByteArray componentXml;
                    QDomDocument componentDoc("Component");
                    QDomElement empty;
                    comps[0]->SerializeTo(componentDoc, empty, true);
                    componentXml = componentDoc.toByteArray();
                    SetHttpRequestReply(connection, componentXml, "application/xml", websocketpp::http::status_code::ok);
                    return;
                }
            }
        }
        // Specific attribute in specific component in specific entity
        else if (pathParts.length() == 4)
        {
            bool ok = false;
            entity_id_t entityId = pathParts[1].toUInt(&ok);
            EntityPtr entity = scene->EntityById(entityId);
            if (entity)
            {
                Entity::ComponentVector comps = entity->ComponentsOfType(pathParts[2]);
                /// \todo Uses only the first component
                if (comps.size())
                {
                    // Try both name & id
                    IAttribute* attr = comps[0]->AttributeById(pathParts[3]);
                    if (!attr)
                        attr = comps[0]->AttributeByName(pathParts[3]);

                    if (attr)
                    {
                        SetHttpRequestReply(connection, attr->ToString(), "text/plain", websocketpp::http::status_code::ok);
                        return;
                    }
                }
            }
        }
    }

    /// \todo Access control

    // Deletions
    if (verb.compare("DELETE", Qt::CaseInsensitive) == 0)
    {
        // Entity by name
        if (pathParts.length() == 1 && pathUrl.hasQueryItem("name"))
        {
            QString name = pathUrl.queryItemValue("name");
            EntityPtr entity = scene->EntityByName(name);
            if (entity)
            {
                scene->RemoveEntity(entity->Id());
                SetHttpRequestReply(connection, "Deleted", "text/plain", websocketpp::http::status_code::ok);
                return;
            }
        }
        // Specific entity
        else if (pathParts.length() == 2)
        {
            bool ok = false;
            entity_id_t entityId = pathParts[1].toUInt(&ok);
            if (scene->RemoveEntity(entityId))
            {
                SetHttpRequestReply(connection, "Deleted", "text/plain", websocketpp::http::status_code::ok);
                return;
            }
        }
        // Specific component in specific entity
        else if (pathParts.length() == 3)
        {
            bool ok = false;
            entity_id_t entityId = pathParts[1].toUInt(&ok);
            EntityPtr entity = scene->EntityById(entityId);
            if (entity)
            {
                Entity::ComponentVector comps = entity->ComponentsOfType(pathParts[2]);
                /// \todo Uses only the first component
                if (comps.size())
                {
                    entity->RemoveComponent(comps[0]);
                    SetHttpRequestReply(connection, "Deleted", "text/plain", websocketpp::http::status_code::ok);
                    return;
                }
            }
        }
        // Specific attribute in specific component in specific entity (DynamicComponent only)
        else if (pathParts.length() == 4)
        {
            bool ok = false;
            entity_id_t entityId = pathParts[1].toUInt(&ok);
            EntityPtr entity = scene->EntityById(entityId);
            if (entity)
            {
                Entity::ComponentVector comps = entity->ComponentsOfType(pathParts[2]);
                /// \todo Uses only the first component
                if (comps.size())
                {
                    // Try both name & id
                    IAttribute* attr = comps[0]->AttributeById(pathParts[3]);
                    if (!attr)
                        attr = comps[0]->AttributeByName(pathParts[3]);
                    EC_DynamicComponent* dc = dynamic_cast<EC_DynamicComponent*>(comps[0].get());

                    if (attr && dc)
                    {
                        dc->RemoveAttribute(attr->Id());
                        SetHttpRequestReply(connection, "Deleted", "text/plain", websocketpp::http::status_code::ok);
                        return;
                    }
                }
            }
        }

        SetHttpRequestReply(connection, "Bad Request", "text/plain", websocketpp::http::status_code::bad_request);
        return;
    }

    // Modifications
    if (verb.compare("PUT", Qt::CaseInsensitive) == 0)
    {
        // Attribute mod inside component by query
        if (pathParts.length() == 3 && pathUrl.hasQuery())
        {
            bool ok = false;
            entity_id_t entityId = pathParts[1].toUInt(&ok);
            EntityPtr entity = scene->EntityById(entityId);
            if (entity)
            {
                Entity::ComponentVector comps = entity->ComponentsOfType(pathParts[2]);
                /// \todo Uses only the first component
                if (comps.size())
                {
                    const QList<QPair<QString, QString> >& queryItems = pathUrl.queryItems();
                    for (QList<QPair<QString, QString> >::const_iterator i = queryItems.begin(); i != queryItems.end(); ++i)
                    {
                        IAttribute* attr = comps[0]->AttributeById(i->first);
                        if (!attr)
                            attr = comps[0]->AttributeByName(i->first);

                        if (attr)
                            attr->FromString(i->second, AttributeChange::Default);
                    }

                    // Reply is the new content of the component
                    QByteArray componentXml;
                    QDomDocument componentDoc("Component");
                    QDomElement empty;
                    comps[0]->SerializeTo(componentDoc, empty, true);
                    componentXml = componentDoc.toByteArray();
                    SetHttpRequestReply(connection, componentXml, "application/xml", websocketpp::http::status_code::ok);
                    return;
                }
            }
        }
        // Injection of whole component's data
        else if (pathParts.length() == 3 && connection->get_request_body().length())
        {
            bool ok = false;
            entity_id_t entityId = pathParts[1].toUInt(&ok);
            EntityPtr entity = scene->EntityById(entityId);
            if (entity)
            {
                Entity::ComponentVector comps = entity->ComponentsOfType(pathParts[2]);
                /// \todo Uses only the first component
                if (comps.size())
                {
                    QByteArray data(connection->get_request_body().c_str());
                    QTextStream stream(&data);
                    stream.setCodec("UTF-8");
                    QDomDocument comp_doc("Component");
                    QString errorMsg;
                    int errorLine, errorColumn;
                    if (!comp_doc.setContent(stream.readAll(), &errorMsg, &errorLine, &errorColumn))
                    {
                        SetHttpRequestReply(connection, QString("XML decode error " + errorMsg + " at line " + QString::number(errorLine)), "text/plain", websocketpp::http::status_code::bad_request);
                        return;
                    }
                    else
                    {
                        QDomElement root = comp_doc.firstChildElement("component");
                        if (!root.isNull())
                        {
                            /// \todo Duplicate code from Component, to be more relaxed and not require component type
                            QDomElement attribute_element = root.firstChildElement("attribute");
                            while(!attribute_element.isNull())
                            {
                                IAttribute* attr = 0;
                                QString id = attribute_element.attribute("id");
                                // Prefer lookup by ID if it's specified, but fallback to using attribute human-readable name if not defined
                                if (id.length())
                                    attr = comps[0]->AttributeById(id);
                                else
                                {
                                    id = attribute_element.attribute("name");
                                    attr = comps[0]->AttributeByName(id);
                                }

                                // If DynamicComponent, can also create attribute
                                if (!attr)
                                {
                                    EC_DynamicComponent* dc = dynamic_cast<EC_DynamicComponent*>(comps[0].get());
                                    if (dc)
                                    {
                                        QString newAttrName = attribute_element.attribute("id");
                                        if (newAttrName.isEmpty())
                                            newAttrName = attribute_element.attribute("name");
                                        attr = dc->CreateAttribute(attribute_element.attribute("type"), newAttrName);
                                    }
                                }

                                if (!attr)
                                    LogWarning(comps[0]->TypeName() + "::DeserializeFrom: Could not find attribute \"" + id + "\" specified in the XML element.");
                                else
                                    attr->FromString(attribute_element.attribute("value"), AttributeChange::Default);
                                attribute_element = attribute_element.nextSiblingElement("attribute");
                            }

                            // Reply is the new content of the component
                            QByteArray componentXml;
                            QDomDocument componentDoc("Component");
                            QDomElement empty;
                            comps[0]->SerializeTo(componentDoc, empty, true);
                            componentXml = componentDoc.toByteArray();
                            SetHttpRequestReply(connection, componentXml, "application/xml", websocketpp::http::status_code::ok);
                            return;
                        }
                    }
                }
            }
        }
        // Injection of whole entity's data (removes existing components and child entities)
        else if (pathParts.length() == 2 && connection->get_request_body().length())
        {
            bool ok = false;
            entity_id_t entityId = pathParts[1].toUInt(&ok);
            EntityPtr entity = scene->EntityById(entityId);
            if (entity)
            {
                QByteArray data(connection->get_request_body().c_str());
                QTextStream stream(&data);
                stream.setCodec("UTF-8");
                QDomDocument ent_doc("Entity");
                QString errorMsg;
                int errorLine, errorColumn;
                if (!ent_doc.setContent(stream.readAll(), &errorMsg, &errorLine, &errorColumn))
                {
                    SetHttpRequestReply(connection, QString("XML decode error " + errorMsg + " at line " + QString::number(errorLine)), "text/plain", websocketpp::http::status_code::bad_request);
                    return;
                }
                else
                {
                    QDomElement root = ent_doc.firstChildElement("entity");
                    if (!root.isNull())
                    {
                        entity->RemoveAllComponents();
                        entity->RemoveAllChildren();

                        CreateComponentsToEntity(entity, root);
                        QDomElement childEnt_elem = root.firstChildElement("entity");
                        while (!childEnt_elem.isNull())
                        {
                            CreateEntity(entity, childEnt_elem);
                            childEnt_elem = childEnt_elem.nextSiblingElement("entity");
                        }
                        
                        // Reply is the new content of the entity
                        QByteArray entityXml;
                        entityXml += entity->SerializeToXMLString(true, true, false);
                        SetHttpRequestReply(connection, entityXml, "application/xml", websocketpp::http::status_code::ok);
                        return;
                    }
                }
            }
        }

        SetHttpRequestReply(connection, "Bad Request", "text/plain", websocketpp::http::status_code::bad_request);
        return;
    }

    // Create new
    if (verb.compare("POST", Qt::CaseInsensitive) == 0)
    {
        // New entity without specifying ID, with or without initial data
        if (pathParts.length() == 1)
        {
            QByteArray data(connection->get_request_body().c_str());
            QTextStream stream(&data);
            stream.setCodec("UTF-8");
            QDomDocument ent_doc("Entity");
            if (data.length())
            {
                QString errorMsg;
                int errorLine, errorColumn;
                if (!ent_doc.setContent(stream.readAll(), &errorMsg, &errorLine, &errorColumn))
                {
                    SetHttpRequestReply(connection, QString("XML decode error " + errorMsg + " at line " + QString::number(errorLine)), "text/plain", websocketpp::http::status_code::bad_request);
                    return;
                }
            }

            QDomElement ent_elem = ent_doc.firstChildElement("entity");

            const bool replicated = ParseBool(ent_elem.attribute("sync"), true);
            const bool temporary = ParseBool(ent_elem.attribute("temporary"), false);

            entity_id_t id = replicated ? scene->NextFreeId() : scene->NextFreeIdLocal();

            EntityPtr entity = scene->CreateEntity(id);
            if (entity)
            {
                entity->SetTemporary(temporary);
                CreateComponentsToEntity(entity, ent_elem);
                // Reply is the new content of the entity
                QByteArray entityXml;
                entityXml += entity->SerializeToXMLString(true, true, false);
                SetHttpRequestReply(connection, entityXml, "application/xml", websocketpp::http::status_code::ok);
                return;
            }
        }
        // New entity, with or without initial data
        else if (pathParts.length() == 2)
        {
            QByteArray data(connection->get_request_body().c_str());
            QTextStream stream(&data);
            stream.setCodec("UTF-8");
            QDomDocument ent_doc("Entity");
            if (data.length())
            {
                QString errorMsg;
                int errorLine, errorColumn;
                if (!ent_doc.setContent(stream.readAll(), &errorMsg, &errorLine, &errorColumn))
                {
                    SetHttpRequestReply(connection, QString("XML decode error " + errorMsg + " at line " + QString::number(errorLine)), "text/plain", websocketpp::http::status_code::bad_request);
                    return;
                }
            }

            QDomElement ent_elem = ent_doc.firstChildElement("entity");

            const bool replicated = ParseBool(ent_elem.attribute("sync"), true);
            const bool temporary = ParseBool(ent_elem.attribute("temporary"), false);

            bool ok = false;
            entity_id_t id = pathParts[1].toUInt(&ok);
            if (id == 0 || scene->HasEntity(id))
                id = replicated ? scene->NextFreeId() : scene->NextFreeIdLocal();

            EntityPtr entity = scene->CreateEntity(id);
            if (entity)
            {
                entity->SetTemporary(temporary);
                CreateComponentsToEntity(entity, ent_elem);
                // Reply is the new content of the entity
                QByteArray entityXml;
                entityXml += entity->SerializeToXMLString(true, true, false);
                SetHttpRequestReply(connection, entityXml, "application/xml", websocketpp::http::status_code::ok);
                return;
            }
        }
        // New component, with or without initial data
        else if (pathParts.length() == 3)
        {
            QByteArray data(connection->get_request_body().c_str());
            QTextStream stream(&data);
            stream.setCodec("UTF-8");
            QDomDocument comp_doc("Component");
            if (data.length())
            {
                QString errorMsg;
                int errorLine, errorColumn;
                if (!comp_doc.setContent(stream.readAll(), &errorMsg, &errorLine, &errorColumn))
                {
                    SetHttpRequestReply(connection, QString("XML decode error " + errorMsg + " at line " + QString::number(errorLine)), "text/plain", websocketpp::http::status_code::bad_request);
                    return;
                }
            }

            bool ok = false;
            entity_id_t entityId = pathParts[1].toUInt(&ok);
            EntityPtr entity = scene->EntityById(entityId);
            if (entity)
            {
                /// \todo Only creates replicated components
                ComponentPtr comp = entity->GetOrCreateComponent(pathParts[2]);
                if (comp)
                {
                    QDomElement root = comp_doc.firstChildElement("component");
                    if (!root.isNull())
                    {
                        /// \todo Duplicate code from Component, to be more relaxed and not require component type
                        QDomElement attribute_element = root.firstChildElement("attribute");
                        while(!attribute_element.isNull())
                        {
                            IAttribute* attr = 0;
                            QString id = attribute_element.attribute("id");
                            // Prefer lookup by ID if it's specified, but fallback to using attribute human-readable name if not defined
                            if (id.length())
                                attr = comp->AttributeById(id);
                            else
                            {
                                id = attribute_element.attribute("name");
                                attr = comp->AttributeByName(id);
                            }
                            // If DynamicComponent, can also create attribute
                            if (!attr)
                            {
                                EC_DynamicComponent* dc = dynamic_cast<EC_DynamicComponent*>(comp.get());
                                if (dc)
                                {
                                    QString newAttrName = attribute_element.attribute("id");
                                    if (newAttrName.isEmpty())
                                        newAttrName = attribute_element.attribute("name");
                                    attr = dc->CreateAttribute(attribute_element.attribute("type"), newAttrName);
                                }
                            }

                            if (!attr)
                                LogWarning(comp->TypeName() + "::DeserializeFrom: Could not find attribute \"" + id + "\" specified in the XML element.");
                            else
                                attr->FromString(attribute_element.attribute("value"), AttributeChange::Default);
                            attribute_element = attribute_element.nextSiblingElement("attribute");
                        }

                        // Reply is the new content of the component
                        QByteArray componentXml;
                        QDomDocument componentDoc("Component");
                        QDomElement empty;
                        comp->SerializeTo(componentDoc, empty, true);
                        componentXml = componentDoc.toByteArray();
                        SetHttpRequestReply(connection, componentXml, "application/xml", websocketpp::http::status_code::ok);
                        return;
                    }
                }
            }

        }


        SetHttpRequestReply(connection, "Bad Request", "text/plain", websocketpp::http::status_code::bad_request);
        return;
    }

    // If we got here, request illegal or otherwise failed
    SetHttpRequestStatus(connection, websocketpp::http::status_code::not_found);
}

void HttpServer::CreateEntity(EntityPtr parent, const QDomElement& ent_elem)
{
    /// \todo Partially duplicate code from Scene
    Scene* scene = GetActiveScene();

    const bool replicated = ParseBool(ent_elem.attribute("sync"), true);
    const bool temporary = ParseBool(ent_elem.attribute("temporary"), false);

    QString id_str = ent_elem.attribute("id");
    entity_id_t id = !id_str.isEmpty() ? static_cast<entity_id_t>(id_str.toInt()) : 0;
    if (id == 0 || scene->HasEntity(id))
        id = replicated ? scene->NextFreeId() : scene->NextFreeIdLocal();
    
    EntityPtr entity;
    if (!parent)
        entity = scene->CreateEntity(id);
    else
        entity = parent->CreateChild(id);

    if (entity)
    {
        entity->SetTemporary(temporary);
        CreateComponentsToEntity(entity, ent_elem);

        QDomElement childEnt_elem = ent_elem.firstChildElement("entity");
        while (!childEnt_elem.isNull())
        {
            CreateEntity(entity, childEnt_elem);
            childEnt_elem = childEnt_elem.nextSiblingElement("entity");
        }
    }
}

void HttpServer::CreateComponentsToEntity(EntityPtr entity, const QDomElement& root)
{
    /// \todo Duplicate code from Scene
    QDomElement comp_elem = root.firstChildElement("component");
    while(!comp_elem.isNull())
    {
        const QString typeName = comp_elem.attribute("type");
        const u32 typeId = ParseUInt(comp_elem.attribute("typeId"), 0xffffffff);
        const QString name = comp_elem.attribute("name");
        const bool compReplicated = ParseBool(comp_elem.attribute("sync"), true);
        const bool temporary = ParseBool(comp_elem.attribute("temporary"), false);

        // If we encounter an unknown component type, now is the time to register a placeholder type for it
        // The XML holds all needed data for it, while binary doesn't
        SceneAPI* sceneAPI = framework_->Scene();
        if (!sceneAPI->IsComponentTypeRegistered(typeName))
            sceneAPI->RegisterPlaceholderComponentType(comp_elem);
            
        ComponentPtr new_comp = (!typeName.isEmpty() ? entity->GetOrCreateComponent(typeName, name, AttributeChange::Default, compReplicated) :
            entity->GetOrCreateComponent(typeId, name, AttributeChange::Default, compReplicated));
        if (new_comp)
        {
            new_comp->SetTemporary(temporary);
            new_comp->DeserializeFrom(comp_elem, AttributeChange::Default);
        }

        comp_elem = comp_elem.nextSiblingElement("component");
    }
}

void HttpServer::OnScriptEngineCreated(QScriptEngine *engine)
{
    qScriptRegisterQObjectMetaType<HttpServer*>(engine);
}

