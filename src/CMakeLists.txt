# Library
find_package(glfw3 3.3 REQUIRED)
find_package(glm REQUIRED)
find_package(Boost 1.66.0 REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

add_library(eveoverlay STATIC 
	gfx/displaywindow.cpp
	gfx/imguiwindow.cpp
	requests.cpp
	base64.cpp)

target_link_libraries(eveoverlay PUBLIC 
	Imgui 
	Glad 
	Spdlog 
	glfw 
	glm 
	IconFontCppHeaders
	Boost::boost
	Threads::Threads
	OpenSSL::SSL OpenSSL::Crypto)

# Executable for running the application
add_executable(eve-overlay eve-overlay.cpp)

target_link_libraries(eve-overlay PUBLIC eveoverlay)
