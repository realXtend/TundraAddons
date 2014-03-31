// For conditions of distribution and use, see copyright notice in LICENSE
// 
#pragma once

#include "HttpServerModuleApi.h"

#include "IModule.h"
#include "CoreTypes.h"

#include <QString>

class HttpServer;

class HTTP_SERVER_MODULE_API HttpServerModule : public IModule
{
    Q_OBJECT

public:
    HttpServerModule();
    virtual ~HttpServerModule();

    void Load();
    void Initialize();
    void Uninitialize();
    
    void Update(f64 frametime);
    
public slots:
    HttpServer* GetServer();
    
signals:
    void ServerStarted(HttpServer* server);
    
private slots:
    void StartServer();
    void StopServer();
    
private:
    HttpServer* server_;
};
