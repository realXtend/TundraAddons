// For conditions of distribution and use, see copyright notice in LICENSE

#include "VlcPlugin.h"
#include "EC_MediaPlayer.h"

#include "Framework.h"
#include "SceneAPI.h"
#include "IComponentFactory.h"

#ifdef __APPLE__
#include "vlc/libvlc.h"
#include <stdlib.h>
#include <QDir>
#include "Application.h"
#endif

VlcPlugin::VlcPlugin() :
    IModule("VlcPlugin")
{
}

void VlcPlugin::Load()
{
#ifdef __APPLE__
    QString vlcLibVersion(libvlc_get_version());
    if (vlcLibVersion.startsWith("2"))
    {
        QString envValue = Application::InstallationDirectory() + "plugins/vlcplugins";
        if (QDir(envValue).exists())
        {
            std::string value = envValue.toStdString();
            setenv("VLC_PLUGIN_PATH", value.c_str(), 1); // Add an environment variable
        }
    }
#endif

    framework_->Scene()->RegisterComponentFactory(ComponentFactoryPtr(new GenericComponentFactory<EC_MediaPlayer>));
}

void VlcPlugin::Uninitialize()
{
}

void VlcPlugin::Unload()
{
}

extern "C"
{
    DLLEXPORT void TundraPluginMain(Framework *fw)
    {
        Framework::SetInstance(fw); // Inside this DLL, remember the pointer to the global framework object.
        IModule *module = new VlcPlugin();
        fw->RegisterModule(module);
    }
}
