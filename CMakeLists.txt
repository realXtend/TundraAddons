# Add TundraAddons solution folder to the tundra solution if applicable.
set(PROJECT_TYPE "TundraAddons")

file(COPY tundra-addons.json DESTINATION ${TUNDRA_BIN})
file(COPY Scenes DESTINATION ${TUNDRA_BIN})
file(COPY jsmodules DESTINATION ${TUNDRA_BIN})

add_subdirectory (BrowserUiPlugin)
#add_subdirectory (CaveStereoModule)
add_subdirectory (KinectModule)
add_subdirectory (PythonScriptModule)
add_subdirectory (VlcPlugin)
add_subdirectory (XMPPModule)
