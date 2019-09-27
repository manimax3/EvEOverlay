add_library(Imgui STATIC 
	imgui/imgui.cpp
	imgui/imgui_demo.cpp
	imgui/imgui_draw.cpp
	imgui/imgui_widgets.cpp)
target_include_directories(Imgui PUBLIC imgui)
target_compile_options(Imgui PRIVATE -g -fno-omit-frame-pointer)

add_library(Glad STATIC glad/glad.c)
target_include_directories(Glad PUBLIC glad)

add_library(IconFontCppHeaders INTERFACE)
target_include_directories(IconFontCppHeaders INTERFACE IconFontCppHeaders)

add_subdirectory(fmt)
