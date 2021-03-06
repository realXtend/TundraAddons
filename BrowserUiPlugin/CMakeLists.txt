# BROWSER_SHARED_DATA_ENABLED controls if we need to link SceneWidgetComponents against BrowserUiPlugin
# in order to get shared data accross all WebView components. This is optional and can be turned off with setting this to 0.
set(BROWSER_SHARED_DATA_ENABLED 0)

init_target(BrowserUiPlugin OUTPUT plugins)

file(GLOB CPP_FILES *.cpp)
file(GLOB H_FILES *.h)

# Install BrowserUi script and UI sources under bin:
file(COPY bin DESTINATION ${TUNDRA_BIN}/..)

QT4_WRAP_CPP(MOC_FILES BrowserUiPlugin.h CookieJar.h)

UseTundraCore()
use_core_modules(TundraCore Math)
use_app_modules(JavascriptModule)

build_library(${TARGET_NAME} SHARED ${CPP_FILES} ${H_FILES} ${MOC_FILES})

SetupCompileFlags()

link_modules(TundraCore JavascriptModule)

link_package(QT4)
if (APPLE)
    link_ogre()
endif()

setup_install_files (${CMAKE_CURRENT_SOURCE_DIR}/bin/tundra-client-browser.json) # Browser viewer startup config

final_target()
