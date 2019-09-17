# Library
find_package(glfw3 3.3 REQUIRED)
find_package(glm REQUIRED)

add_library(eveoverlay STATIC 
	gfx/displaywindow.cpp
	gfx/imguiwindow.cpp)

target_link_libraries(eveoverlay PUBLIC Imgui Glad Spdlog glfw glm)

# Executable for running the application
add_executable(eve-overlay eve-overlay.cpp)

target_link_libraries(eve-overlay PUBLIC eveoverlay)
