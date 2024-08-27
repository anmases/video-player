cmake_minimum_required(VERSION 3.14)
project(video-player LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 14)

# Añadir subdirectorio de GLFW
add_subdirectory(lib/glfw)

# Encontrar y enlazar las bibliotecas de FFmpeg proporcionadas por vcpkg
find_package(FFmpeg REQUIRED)

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

# Añadir los directorios de encabezados (headers) de FFmpeg
target_include_directories(video-player PRIVATE ${FFMPEG_INCLUDE_DIRS})

# Enlazar las bibliotecas
target_link_libraries(video-player glfw 
    ${FFMPEG_LIBRARIES}
    ${EXTRA_LIBS})

