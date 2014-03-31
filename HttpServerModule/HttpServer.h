// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "HttpServerModuleApi.h"
#include "Win.h"

#include "FrameworkFwd.h"
#include "SceneFwd.h"
#include "CoreTypes.h"

#include <QObject>
#include <QThread>
#include <QString>
#include <QStringList>
#include <QFileInfo>
#include <QDateTime>
#include <QMutex>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/http/constants.hpp>

#include "kNet/DataSerializer.h"
#include "boost/weak_ptr.hpp"

class QDomElement;
class QScriptEngine;
class Scene;

class HTTP_SERVER_MODULE_API HttpServer : public QObject, public enable_shared_from_this<HttpServer>
{
    Q_OBJECT

public:
    typedef shared_ptr<websocketpp::server<websocketpp::config::asio> > ServerPtr;
    typedef websocketpp::server<websocketpp::config::asio>::connection_ptr ConnectionPtr;
    typedef boost::weak_ptr<websocketpp::server<websocketpp::config::asio>::connection_type> ConnectionWeakPtr;
    typedef websocketpp::connection_hdl ConnectionHandle;
    typedef websocketpp::server<websocketpp::config::asio>::message_ptr MessagePtr;

    HttpServer(Framework *framework, ushort port);
    ~HttpServer();
    
    bool Start();
    void Stop();
    void Update(float frametime);
    
public slots:
    /// \todo Expose types to scripting
    void SetHttpRequestReply(ConnectionPtr connection, const QByteArray& replyData, const QString& contentType, websocketpp::http::status_code::value status = websocketpp::http::status_code::ok);
    void SetHttpRequestReply(ConnectionPtr connection, const QString& reply, const QString& contentType, websocketpp::http::status_code::value status = websocketpp::http::status_code::ok);
    void SetHttpRequestReply(ConnectionPtr connection, const char* reply, const QString& contentType, websocketpp::http::status_code::value status = websocketpp::http::status_code::ok);

    void SetHttpRequestStatus(ConnectionPtr connection, websocketpp::http::status_code::value status);

    Scene* GetActiveScene();

private slots:
    void OnScriptEngineCreated(QScriptEngine *engine);

signals:
    /// The server has been started
    void ServerStarted();

    /// The server has been stopped
    void ServerStopped();
    
    /// A http request that is not internally handled has been received. Use SetRequestReply() to handle it.
    /// \todo Expose types to scripting
    void HttpRequestReceived(ConnectionPtr connection, const QString& path, const QString& verb);

protected:
    void OnHttpRequest(ConnectionHandle connection);
    
private:
    void HandleSceneHttpRequest(ConnectionPtr connection, const QString& path, const QString& verb);
    void CreateEntity(EntityPtr parent, const QDomElement& childEnt_elem);
    void CreateComponentsToEntity(EntityPtr entity, const QDomElement& root);

    void Reset();

    ushort port_;
    
    Framework *framework_;
    
    ServerPtr server_;
};
