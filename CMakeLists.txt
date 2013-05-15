file(COPY addons.xml DESTINATION ${TUNDRA_BIN})

add_subdirectory (BrowserUiPlugin)
add_subdirectory (KinectModule)
add_subdirectory (PythonScriptModule)
add_subdirectory (VlcPlugin)
add_subdirectory (XMPPModule)
