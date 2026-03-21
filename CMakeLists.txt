cmake_minimum_required(VERSION 3.16)
project(pin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- GLFW (system-installed) ---
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW REQUIRED glfw3)

# --- OpenGL ---
find_package(OpenGL REQUIRED)

# --- Source files ---
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/lib/imgui)

set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/imgui_impl_opengl3.cpp
)

add_executable(pin
    src/main.cpp
    ${IMGUI_SOURCES}
)

target_include_directories(pin PRIVATE
    ${IMGUI_DIR}
    ${CMAKE_SOURCE_DIR}/lib
    ${GLFW_INCLUDE_DIRS}
)

target_link_libraries(pin PRIVATE
    ${GLFW_LIBRARIES}
    OpenGL::GL
)

target_compile_options(pin PRIVATE ${GLFW_CFLAGS_OTHER})
