cmake_minimum_required(VERSION 3.14)
project(video-player LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 14)

add_subdirectory(lib/glfw)

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

add_executable(video-player src/main.cpp)
target_link_libraries(video-player glfw ${EXTRA_LIBS})
