#include <stdio.h>
#include <iostream>
#include <SDL3/SDL.h>
#include <GL/gl.h>
#include "video_reader.hpp"

int main(int argc, char* argv[]) {
    // Inicializar SDL con subsistemas de video y audio
    if (SDL_Init(SDL_INIT_TIMER) != 0) {
        const char* error = SDL_GetError();
        if (error && *error != '\0') {
            printf("Error al inicializar SDL_VIDEO: %s\n", error);
        } else {
            printf("Error al inicializar SDL_VIDEO: No se proporcionó un mensaje de error.\n");
        }
        return 1;
    }

    // Crear una ventana con SDL3
    SDL_Window* window = SDL_CreateWindow("Video Player SDL3", 640, 480, SDL_WINDOW_OPENGL);
    if (!window) {
        printf("Error al crear la ventana: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Crear un contexto OpenGL directamente desde la ventana
    if (SDL_SetWindowResizable(window, SDL_TRUE) != 0) {
        printf("Error al hacer la ventana redimensionable: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Inicializar el lector de video y manejo de texturas
    VideoReaderState vr_state;
    if (!video_reader_open(&vr_state, "C:\\Users\\antonio\\Desktop\\test.mp4")) {
        printf("Couldn't open video file\n");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    const int frame_width = vr_state.width;
    const int frame_height = vr_state.height;
    uint8_t* frame_data = new uint8_t[frame_width * frame_height * 4];

    // Ajustar el tamaño de la ventana al tamaño del frame de video
    SDL_SetWindowSize(window, frame_width, frame_height);

    // Crear una textura OpenGL
    GLuint tex_handle;
    glGenTextures(1, &tex_handle);
    glBindTexture(GL_TEXTURE_2D, tex_handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // Bucle principal
    bool running = true;
    SDL_Event event;
    while (running) {
        // Manejo de eventos
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        // Leer un frame y cargarlo en la textura
        int64_t pts;
        if (!video_reader_read_frame(&vr_state, frame_data, &pts)) {
            printf("Couldn't load video frame\n");
            break;
        }

        // Renderizar el frame
        glClear(GL_COLOR_BUFFER_BIT);
        glBindTexture(GL_TEXTURE_2D, tex_handle);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, frame_width, frame_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, frame_data);

        // Configurar la proyección ortogonal y dibujar el quad
        glBegin(GL_QUADS);
        glTexCoord2d(0, 0); glVertex2i(0, 0);
        glTexCoord2d(1, 0); glVertex2i(frame_width, 0);
        glTexCoord2d(1, 1); glVertex2i(frame_width, frame_height);
        glTexCoord2d(0, 1); glVertex2i(0, frame_height);
        glEnd();

        // Presentar en la ventana
        SDL_GL_SwapWindow(window);
    }

    // Limpiar
    video_reader_close(&vr_state);
    delete[] frame_data;
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
