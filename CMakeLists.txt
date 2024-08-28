cmake_minimum_required(VERSION 3.14)
project(video-player LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 14)

# Añadir subdirectorio de GLFW y SDL3
add_subdirectory(lib/glfw)
add_subdirectory(lib/SDL3)

# SDL3 Include y Lib directories
set(SDL3_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/lib/SDL3/include")
include_directories(${SDL3_INCLUDE_DIR})

# FFmpeg desde VCPKG
find_package(FFmpeg REQUIRED)
include_directories(${FFMPEG_INCLUDE_DIRS})
link_directories(${FFMPEG_LIBRARY_DIRS})

# Otras configuraciones del sistema
if(APPLE)
    list(APPEND EXTRA_LIBS "-framework OpenGL")
elseif(WIN32)
    list(APPEND EXTRA_LIBS opengl32 glu32)
    set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++")
else()
    list(APPEND EXTRA_LIBS GL GLU X11)
endif()

# Archivos fuente
list(APPEND SOURCES
    src/main.cpp
    src/video_reader.cpp
    src/video_reader.hpp
)

# Crear el ejecutable
add_executable(video-player ${SOURCES})

# Enlazar las bibliotecas
target_link_libraries(video-player PRIVATE glfw ${FFMPEG_LIBRARIES} SDL3::SDL3 ${EXTRA_LIBS})
