cmake_minimum_required(VERSION 3.14)
project(video-player LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 14)

# Añadir subdirectorio de SDL3
add_subdirectory(lib/SDL3)

# FFmpeg desde VCPKG
find_package(FFmpeg REQUIRED)
include_directories(${FFMPEG_INCLUDE_DIRS})

# Archivos fuente
list(APPEND SOURCES
    src/main.cpp
    src/video_reader.cpp
    src/video_reader.hpp
)

# Crear el ejecutable
add_executable(video-player ${SOURCES})

# Incluir directorios para SDL3 después de la creación del ejecutable
target_include_directories(video-player PRIVATE ${CMAKE_SOURCE_DIR}/lib/SDL3/include)

# Otras configuraciones del sistema
if(APPLE)
    list(APPEND EXTRA_LIBS "-framework OpenGL")
elseif(WIN32)
    list(APPEND EXTRA_LIBS opengl32 glu32)
    set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++")
else()
    list(APPEND EXTRA_LIBS GL GLU X11)
endif()

# Enlazar las bibliotecas
target_link_libraries(video-player PRIVATE ${FFMPEG_LIBRARIES} SDL3::SDL3 ${EXTRA_LIBS})

#copia el fochero SDL3 junto al ejecutable
add_custom_command(TARGET video-player POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_BINARY_DIR}/lib/SDL3/SDL3.dll"
    $<TARGET_FILE_DIR:video-player>)
