cmake_minimum_required(VERSION 3.14)
project(video-player LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 14)

# Añadir subdirectorio de GLFW
add_subdirectory(lib/glfw)

# Añadir las rutas a los directorios de encabezados de FFmpeg
include_directories(
    ${CMAKE_SOURCE_DIR}/lib/FFmpeg
    ${CMAKE_SOURCE_DIR}/lib/FFmpeg/libavcodec
    ${CMAKE_SOURCE_DIR}/lib/FFmpeg/libavformat
    ${CMAKE_SOURCE_DIR}/lib/FFmpeg/libavutil
    ${CMAKE_SOURCE_DIR}/lib/FFmpeg/libswscale
    ${CMAKE_SOURCE_DIR}/lib/FFmpeg/libswresample
)

# Especificar las bibliotecas a enlazar
set(FFMPEG_LIBRARIES
    ${CMAKE_SOURCE_DIR}/lib/FFmpeg/libavcodec/libavcodec.a
    ${CMAKE_SOURCE_DIR}/lib/FFmpeg/libavformat/libavformat.a
    ${CMAKE_SOURCE_DIR}/lib/FFmpeg/libavutil/libavutil.a
    ${CMAKE_SOURCE_DIR}/lib/FFmpeg/libswscale/libswscale.a
    ${CMAKE_SOURCE_DIR}/lib/FFmpeg/libswresample/libswresample.a
)

if(APPLE)
    list(APPEND EXTRA_LIBS
    "-framework OpenGL")
elseif(WIN32)
    list(APPEND EXTRA_LIBS
    opengl32 glu32)
    set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++")
else()
    list(APPEND EXTRA_LIBS
    GL GLU X11)
endif()

# Crear el ejecutable
add_executable(video-player src/main.cpp)

# Enlazar las bibliotecas
target_link_libraries(video-player glfw ${FFMPEG_LIBRARIES} ${EXTRA_LIBS})

