# Define target name and output directory
init_target (HttpServerModule OUTPUT plugins)

if (TUNDRA_NO_BOOST)
    configure_boost()
endif()

MocFolder ()

# Define source files
file (GLOB CPP_FILES *.cpp)
file (GLOB H_FILES *.h)
file (GLOB MOC_FILES HttpServer.h HttpServerModule.h)

# Qt4 Wrap
QT4_WRAP_CPP(MOC_SRCS ${MOC_FILES})

add_definitions (-DHTTP_SERVER_MODULE_EXPORTS)

# Includes
UseTundraCore ()
use_core_modules (TundraCore Math Asset Scene Console OgreRenderingModule PhysicsModule TundraProtocolModule)

build_library (${TARGET_NAME} SHARED ${CPP_FILES} ${H_FILES} ${MOC_SRCS})

# Linking
link_ogre ()
link_package_knet ()
link_modules (TundraCore Math OgreRenderingModule TundraProtocolModule)

if (WIN32)
    target_link_libraries (${TARGET_NAME}
        ws2_32.lib
    )
else()
    # Visual Studio uses library auto-linking, so this is only necessary for the other platforms.
    target_link_libraries(${TARGET_NAME} ${Boost_LIBRARIES})
endif()

SetupCompileFlags()

final_target ()
