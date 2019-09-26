# Library
find_package(glfw3 3.3 REQUIRED)
find_package(glm REQUIRED)
find_package(Boost 1.70.0 REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json 3.1.2 REQUIRED)
find_package(SQLite3 3.3 REQUIRED)

add_library(eveoverlay STATIC 
	displaywindow.cpp
	imguiwindow.cpp
	systeminfowindow.cpp
	requests.cpp
	base64.cpp
	authentication.cpp
	util.cpp
	db.cpp
	esisession.cpp)

target_link_libraries(eveoverlay PUBLIC 
	Imgui Glad fmt glfw glm IconFontCppHeaders
	Boost::boost Threads::Threads OpenSSL::SSL OpenSSL::Crypto
	nlohmann_json::nlohmann_json SQLite::SQLite3)

# Executable for running the application
add_executable(eve-overlay eve-overlay.cpp)

target_link_libraries(eve-overlay PUBLIC eveoverlay)

if(NOT MSVC)
	target_compile_options(eveoverlay PUBLIC -Wall -Wextra)
	target_compile_options(eveoverlay PUBLIC $<$<CONFIG:DEBUG>:-fno-omit-frame-pointer -fsanitize=address>)
	target_link_libraries(eveoverlay PUBLIC $<$<CONFIG:DEBUG>:-fsanitize=address>)
endif()
