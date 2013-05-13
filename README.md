Additional modules and plugins for realXtend Tundra, which are not part of the 
core build & repository. To enable these modules, clone this repository to the 
"src" directory of your Tundra SDK checkout (ie. src/TundraAddons), then add 
the necessary modules at the bottom of your CMakeBuildConfig.txt using CMake's
add_subdirectory command. For example

add_subdirectory (src/TundraAddons/PythonScriptModule)
