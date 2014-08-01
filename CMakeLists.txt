
# Add TundraAddons solution folder to 
# the tundra solution if applicable.

set(PROJECT_TYPE "TundraAddons")

# Declare options that can be overridden via 
# cmake cmd line -D<var>=ON/OFF/TRUE/FALSE

option(TUNDRA_ADDONS_INSTALL_CONFIG         "" ON)
option(TUNDRA_ADDONS_INSTALL_SCENES         "" ON)
option(TUNDRA_ADDONS_INSTALL_JSMODULES      "" ON)

option(TUNDRA_ADDONS_VLC_PLUGIN             "" ON)
option(TUNDRA_ADDONS_HTTP_SERVER_PLUGIN     "" ON)
option(TUNDRA_ADDONS_BROWSER_UI_PLUGIN      "" OFF)
option(TUNDRA_ADDONS_CAVE_STEREO_PLUGIN     "" OFF)
option(TUNDRA_ADDONS_KINECT_PLUGIN          "" OFF)
option(TUNDRA_ADDONS_PYTHON_PLUGIN          "" OFF)
option(TUNDRA_ADDONS_XMPP_PLUGIN            "" OFF)

# Print config

message("** TundraAddons")
message(STATUS "  TUNDRA_ADDONS_INSTALL_CONFIG      = ${TUNDRA_ADDONS_INSTALL_CONFIG}")
message(STATUS "  TUNDRA_ADDONS_INSTALL_SCENES      = ${TUNDRA_ADDONS_INSTALL_SCENES}")
message(STATUS "  TUNDRA_ADDONS_INSTALL_JSMODULES   = ${TUNDRA_ADDONS_INSTALL_JSMODULES}")
message(" ")
message(STATUS "  TUNDRA_ADDONS_VLC_PLUGIN          = ${TUNDRA_ADDONS_VLC_PLUGIN}")
message(STATUS "  TUNDRA_ADDONS_HTTP_SERVER_PLUGIN  = ${TUNDRA_ADDONS_HTTP_SERVER_PLUGIN}")
message(STATUS "  TUNDRA_ADDONS_BROWSER_UI_PLUGIN   = ${TUNDRA_ADDONS_BROWSER_UI_PLUGIN}")
message(STATUS "  TUNDRA_ADDONS_CAVE_STEREO_PLUGIN  = ${TUNDRA_ADDONS_CAVE_STEREO_PLUGIN}")
message(STATUS "  TUNDRA_ADDONS_KINECT_PLUGIN       = ${TUNDRA_ADDONS_KINECT_PLUGIN}")
message(STATUS "  TUNDRA_ADDONS_PYTHON_PLUGIN       = ${TUNDRA_ADDONS_PYTHON_PLUGIN}")
message(STATUS "  TUNDRA_ADDONS_XMPP_PLUGIN         = ${TUNDRA_ADDONS_XMPP_PLUGIN}")
message(" ")

# Install files to Tundra bin

if (TUNDRA_ADDONS_INSTALL_CONFIG)
    # todo: This addons file should be always rebuilt in code
    # depending on what plugins are actually included in the build.
    file(COPY tundra-addons.json DESTINATION ${TUNDRA_BIN})
endif()
if (TUNDRA_ADDONS_INSTALL_SCENES)
    file(COPY Scenes DESTINATION ${TUNDRA_BIN})
endif()
if (TUNDRA_ADDONS_INSTALL_JSMODULES)
    file(COPY jsmodules DESTINATION ${TUNDRA_BIN})
endif()

# Include plugins to the build 

if (TUNDRA_ADDONS_VLC_PLUGIN)
    add_subdirectory(VlcPlugin)
endif()
if (TUNDRA_ADDONS_HTTP_SERVER_PLUGIN)
    add_subdirectory(HttpServerModule)
endif()
if (TUNDRA_ADDONS_BROWSER_UI_PLUGIN)
    add_subdirectory(BrowserUiPlugin)
endif()
if (TUNDRA_ADDONS_CAVE_STEREO_PLUGIN)
    add_subdirectory(CaveStereoModule)
endif()
if (TUNDRA_ADDONS_KINECT_PLUGIN)
    add_subdirectory(KinectModule)
endif()
if (TUNDRA_ADDONS_PYTHON_PLUGIN)
    add_subdirectory(PythonScriptModule)
endif()
if (TUNDRA_ADDONS_XMPP_PLUGIN)
    add_subdirectory(XMPPModule)
endif()
