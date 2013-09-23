# TODO remove the todo_prefix when this commit is in the mainline https://github.com/Adminotech/tundra/commit/aa8a93383bb1aec6c03dec82ce8275c0f3667ec2
macro(todo_configure_python)
    sagase_configure_package(PYTHON
        NAMES PythonLibs Python python Python26 python26 Python2.6 python2.6
        COMPONENTS Python python Python26 python Python2.6 python2.6
        PREFIXES ${ENV_TUNDRA_DEP_PATH})

    # FindPythonLibs.cmake
    if (PYTHONLIBS_FOUND)
        set(PYTHON_LIBRARIES ${PYTHON_LIBRARY})
        set(PYTHON_INCLUDE_DIRS ${PYTHON_INCLUDE_PATH})
        #unset(PYTHON_DEBUG_LIBRARIES ${PYTHON_DEBUG_LIBRARY})
    endif()

    # FindPythonLibs.cmake prefers the system-wide Python, which does not
    # include debug libraries, so we force to TUNDRA_DEP_PATH.
    if (MSVC)
        set(PYTHON_LIBRARY_DIRS ${ENV_TUNDRA_DEP_PATH}/Python/lib)
        set(PYTHON_INCLUDE_DIRS ${ENV_TUNDRA_DEP_PATH}/Python/include)
        set(PYTHON_LIBRARIES python26)
        set(PYTHON_DEBUG_LIBRARIES python26_d)
    endif()

    sagase_configure_report(PYTHON)
endmacro()
# TODO remove the todo_prefix when this commit is in the mainline https://github.com/Adminotech/tundra/commit/aa8a93383bb1aec6c03dec82ce8275c0f3667ec2
macro(todo_configure_python_qt)
    sagase_configure_package(PYTHON_QT
        NAMES PythonQt
        COMPONENTS PythonQt PythonQt_QtAll
        PREFIXES ${ENV_TUNDRA_DEP_PATH})

    sagase_configure_report(PYTHON_QT)
endmacro()

# Python and PythonQt are not available currently on Mac.
if (NOT APPLE AND "$ENV{TUNDRA_PYTHON_ENABLED}" STREQUAL "TRUE")

    configure_python()
    configure_python_qt()
    
    # Define target name and output directory
    init_target (PythonScriptModule OUTPUT plugins)

    # Get EC defines that are added to this build.
    GetEntityComponents()

    # Define source files
    # todo: check the moc list and in general what can be removed.
    file (GLOB CPP_FILES *.cpp)
    file (GLOB H_FILES *.h)
    file (GLOB XML_FILES *.xml)
    file (GLOB MOC_FILES PythonScriptModule.h PythonQtScriptingConsole.h TundraWrapper.h)
    set (SOURCE_FILES ${CPP_FILES} ${H_FILES})

    # Copy Python script plugins to bin/pyplugins
    file(COPY pyplugins DESTINATION ${TUNDRA_BIN}/pyplugins)

    # Qt4 Wrap
    QT4_WRAP_CPP(MOC_SRCS ${MOC_FILES})
    QT4_WRAP_UI(UI_SRCS ${UI_FILES})
    QT4_ADD_RESOURCES(RESOURCE_SRCS ${RESOURCE_FILES})

    set (FILES_TO_TRANSLATE ${FILES_TO_TRANSLATE} ${H_FILES} ${CPP_FILES} PARENT_SCOPE)

    add_definitions (-DPYMODULE_MODULE_EXPORTS)

    # Use python and python qt.
    # Boost/Ogre includes are automatic.
    use_package (PYTHON)
    use_package (PYTHON_QT)

    # Use core and app modules we need. Notice separate macros due to source locations separation.
    UseTundraCore()
    use_core_modules(TundraCore Math OgreRenderingModule TundraProtocolModule)

    # Build PythonScriptModule
    build_library (${TARGET_NAME} SHARED ${SOURCE_FILES} ${MOC_SRCS})

    # Link to all: this include core and app (as the linking is just done by name not absolute path)
    link_modules(TundraCore OgreRenderingModule TundraProtocolModule)

    # Link to ECs. This macro will smartly opt out ECs that are not included in the build, info for this provided by GetEntityComponents() on top.
    link_entity_components (EC_Script EC_Highlight EC_HoveringText EC_Sound EC_TransformGizmo EC_PlanarMirror EC_ProximityTrigger)

    # Use python, pythonqt and ogre
    link_package (PYTHON)
    link_package (PYTHON_QT)
    link_ogre()

    # Use PCH with StableHeaders.h
    SetupCompileFlagsWithPCH()

    # Finalize our target, this will do some nice automation for us.
    final_target ()

    # make install: Install files
    setup_install_directory (${TUNDRA_BIN}/pyplugins)

    # make install: Install windows deps
    # todo: resurect the dep install steps when the windows deps are fixed. The macro signature has changed, look at how other working code is using it when porting!
    #setup_install_windows_deps ("python26.dll;PythonQt.dll;PythonQt_QtAll.dll" Release|RelWithDebInfo "python26_d.dll;PythonQt_d.dll;PythonQt_QtAll_d.dll" Debug)
    #setup_install_windows_deps_regexp ("[/]pyplugins[/]python[/]DLLs[/].*[^d][.]pyd$" Release|RelWithDebInfo "[/]pyplugins[/]python[/]DLLs[/].*[d][.]pyd$" Debug)
    #setup_install_windows_deps_regexp ("[/]pyplugins[/]python[/].*[.]zip$" Release|RelWithDebInfo|Debug)

endif()
