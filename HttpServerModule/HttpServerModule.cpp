
#include "HttpServerModule.h"

#include "HttpServer.h"

#include "Framework.h"
#include "CoreDefines.h"
#include "LoggingFunctions.h"

HttpServerModule::HttpServerModule() :
    IModule("HttpServerModule"),
    server_(0)
{
}

HttpServerModule::~HttpServerModule()
{
    StopServer();
}

void HttpServerModule::Load()
{
}

void HttpServerModule::Initialize()
{
    if (framework_->HasCommandLineParameter("--httpPort"))
        StartServer();
}

void HttpServerModule::Uninitialize()
{
    StopServer();
}

void HttpServerModule::Update(f64 frametime)
{
    if (server_)
        server_->Update(frametime);
}

HttpServer* HttpServerModule::GetServer()
{
    return server_;
}

void HttpServerModule::StartServer()
{
    if (server_)
    {
        LogWarning("Server already started.");
        return;
    }

    int port = 0;
    QStringList portParam = framework_->CommandLineParameters("--httpPort");
    if (!portParam.isEmpty())
    {
        bool ok = false;
        port = portParam.first().toUShort(&ok);
    }
    if (!port)
    {
        LogWarning("No valid --httpPort parameter given; can not start http server");
        return;
    }
    server_ = new HttpServer(framework_, port);
    server_->Start();

    framework_->RegisterDynamicObject("httpserver", server_);

    emit ServerStarted(server_);
}

void HttpServerModule::StopServer()
{
    if (server_)
    {
        server_->Stop();
        delete server_;
        server_ = 0;
    }
}

extern "C"
{
    DLLEXPORT void TundraPluginMain(Framework *fw)
    {
        Framework::SetInstance(fw); // Inside this DLL, remember the pointer to the global framework object.
        IModule *module = new HttpServerModule();
        fw->RegisterModule(module);
    }
}
