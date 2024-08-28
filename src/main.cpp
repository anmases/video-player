#include <SDL3/SDL.h>
#include <chrono>
#include <iostream>
#include "video_reader.hpp"
#include <GL/gl.h>

int main(int argc, const char** argv) {
    // Inicializar SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) == SDL_FALSE) {
        printf("Couldn't initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    // Crear una ventana SDL con contexto OpenGL
    SDL_Window* window = SDL_CreateWindow("Hello World", 640, 480, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) {
        printf("Couldn't create SDL window: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Crear el contexto OpenGL
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        printf("Couldn't create OpenGL context: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Configurar VSync (opcional)
    SDL_GL_SetSwapInterval(1);

    // Abrir el archivo de video
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

    // Actualizar el tamaño de la ventana a las dimensiones del frame
    SDL_SetWindowSize(window, frame_width, frame_height);

    // Crear una textura OpenGL
    GLuint tex_handle;
    glGenTextures(1, &tex_handle);
    glBindTexture(GL_TEXTURE_2D, tex_handle);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    // Configuración del audio con SDL3
    SDL_AudioSpec spec = {
        .format = SDL_AUDIO_S16,
        .channels = vr_state.audio_codec_context->ch_layout.nb_channels,
        .freq = vr_state.audio_codec_context->sample_rate
    };

    SDL_AudioStream* audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
    if (!audio_stream) {
        printf("Failed to open audio stream: %s\n", SDL_GetError());
        video_reader_close(&vr_state);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(audio_stream));

    // Variables para el tiempo de reproducción usando std::chrono
    using Clock = std::chrono::high_resolution_clock;
    Clock::time_point start_time = Clock::now();

    uint8_t* audio_data = nullptr;
    int audio_size = 0;

    // Bucle principal de la aplicación
    bool running = true;
    SDL_Event event;
    while (running) {
        // Manejar eventos SDL
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        // Leer el frame de video y sincronizar con el tiempo de reproducción
        int64_t pts;
        if (video_reader_read_frame(&vr_state, frame_data, &pts)) {
            double pt_seconds = pts * (double)vr_state.time_base.num / (double)vr_state.time_base.den;

            // Sincronizar con el tiempo de reproducción
            Clock::time_point current_time = Clock::now();
            std::chrono::duration<double> elapsed_seconds = current_time - start_time;

            while (pt_seconds > elapsed_seconds.count()) {
                SDL_Delay(1);
                current_time = Clock::now();
                elapsed_seconds = current_time - start_time;
            }

            // Renderizar el frame de video
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glViewport(0, 0, frame_width, frame_height);
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glOrtho(0, frame_width, frame_height, 0, -1, 1);
            glMatrixMode(GL_MODELVIEW);

            glBindTexture(GL_TEXTURE_2D, tex_handle);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, frame_width, frame_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, frame_data);

            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, tex_handle);
            glBegin(GL_QUADS);
                glTexCoord2d(0, 0); glVertex2i(0, 0);
                glTexCoord2d(1, 0); glVertex2i(frame_width, 0);
                glTexCoord2d(1, 1); glVertex2i(frame_width, frame_height);
                glTexCoord2d(0, 1); glVertex2i(0, frame_height);
            glEnd();
            glDisable(GL_TEXTURE_2D);
            SDL_GL_SwapWindow(window);
        }

        // Leer y reproducir el audio cuando esté sincronizado
        if (video_reader_read_audio(&vr_state, &audio_data, &audio_size)) {
            if (SDL_PutAudioStreamData(audio_stream, audio_data, audio_size) < 0) {
                printf("Error while putting audio data: %s\n", SDL_GetError());
            }
            delete[] audio_data;
        }
    }

    // Limpiar recursos
    SDL_DestroyAudioStream(audio_stream);
    video_reader_close(&vr_state);
    SDL_DestroyWindow(window);
    SDL_Quit();
    delete[] frame_data;

    return 0;
}
