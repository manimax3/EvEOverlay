# Library
find_package(glfw3 3.3 REQUIRED)

add_library(eveoverlay STATIC lib.cpp)

target_link_libraries(eveoverlay PUBLIC Imgui Glad Spdlog glfw)

# Executable for running the application
add_executable(eve-overlay eve-overlay.cpp)

target_link_libraries(eve-overlay PUBLIC eveoverlay)
