#include <SDL3/SDL.h>
#include <chrono>
#include <iostream>
#include "video_reader.hpp"
#include <GL/gl.h>
#include <thread>
#include <atomic>
#include <mutex>

std::mutex render_mutex;

void decode_thread(VideoState* state) {
    AVPacket* packet = av_packet_alloc();
    bool end_of_stream = false;

    while (!state->quit) {
        if (!end_of_stream && av_read_frame(state->format_context, packet) < 0) {
            end_of_stream = true;
        }

        if (end_of_stream) {
            if (state->video_queue.empty() && state->audio_queue.empty()) {
                state->quit = true;
                break;
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(0));
                continue;
            }
        }

        if (packet->stream_index == state->video_stream_index && !state->video_queue.full()) {
            VideoFrame vf;
            decode_video_packet(state, packet, vf);
            if (vf.data != nullptr) {
                state->video_queue.enqueue(vf);
                std::cout << "Video frame enqueued, PTS: " << vf.pts << std::endl;
            }
        } else if (packet->stream_index == state->audio_stream_index && !state->audio_queue.full()) {
            AudioData ad;
            decode_audio_packet(state, packet, ad);
            if (ad.data != nullptr) {
                state->audio_queue.enqueue(ad);
                std::cout << "Audio data enqueued, PTS: " << ad.pts << std::endl;
            }
        }

        av_packet_unref(packet);
    }

    av_packet_free(&packet);
}

void audio_thread(VideoState* state, SDL_AudioStream* audio_stream) {
    while (!state->quit || !state->audio_queue.empty()) {
        AudioData ad;
        if (state->audio_queue.dequeue(ad)) {
            double audio_pts = ad.pts;

            std::lock_guard<std::mutex> lock(render_mutex);
            if (SDL_PutAudioStreamData(audio_stream, ad.data, ad.size) < 0) {
                printf("Error while putting audio data: %s\n", SDL_GetError());
            }
            state->audio_clock = audio_pts;
            delete[] ad.data; // Liberar la memoria de los datos de audio
            std::cout << "Audio data played, PTS: " << audio_pts << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Pequeña espera para evitar uso excesivo de CPU
    }
}


int main(int argc, const char** argv) {
    VideoState state;

    // Inicializar SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) == SDL_FALSE) {
        printf("Couldn't initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    // Crear una ventana SDL con contexto OpenGL
    state.window = SDL_CreateWindow("Video Player", 640, 480, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!state.window) {
        printf("Couldn't create SDL window: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Crear el contexto OpenGL
    state.gl_context = SDL_GL_CreateContext(state.window);
    if (!state.gl_context) {
        printf("Couldn't create OpenGL context: %s\n", SDL_GetError());
        SDL_DestroyWindow(state.window);
        SDL_Quit();
        return 1;
    }

    // Crear textura
    glGenTextures(1, &state.texture);
    glBindTexture(GL_TEXTURE_2D, state.texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    SDL_GL_SetSwapInterval(1);

    // Abrir el archivo de video
    if (!video_reader_open(&state, "C:\\Users\\antonio\\Desktop\\test.mp4")) {
        printf("Couldn't open video file\n");
        SDL_DestroyWindow(state.window);
        SDL_Quit();
        return 1;
    }

		// Actualizar el tamaño de la ventana a las dimensiones del frame
    SDL_SetWindowSize(state.window, state.width, state.height);

    // Configuración del audio con SDL3
    SDL_AudioSpec spec = {
        .format = SDL_AUDIO_S16,
        .channels = state.audio_codec_context->ch_layout.nb_channels,
        .freq = state.audio_codec_context->sample_rate
    };

    SDL_AudioStream* audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
    if (!audio_stream) {
        printf("Failed to open audio stream: %s\n", SDL_GetError());
        video_reader_close(&state);
        SDL_DestroyWindow(state.window);
        SDL_Quit();
        return 1;
    }

    SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(audio_stream));

    state.quit = false;
    state.audio_clock = 0.0;
    state.video_clock = 0.0;

		// Variables para el tiempo de reproducción usando std::chrono
    using Clock = std::chrono::high_resolution_clock;
    Clock::time_point start_time = Clock::now();

    state.decode_thread = new std::thread(decode_thread, &state);
    state.audio_thread = new std::thread(audio_thread, &state, audio_stream);
		double pt_seconds;
    SDL_Event event;
    while (!state.quit || !state.video_queue.empty()) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                state.quit = true;
            }
        }

        VideoFrame vf;
        if (state.video_queue.dequeue(vf)) {
						render_video_frame(&state, vf);
						std::cout << "Video data played, PTS: " << vf.pts << std::endl;
						pt_seconds = vf.pts * (double)state.video_time_base.num / (double)state.video_time_base.den;
						// Sincronizar con el tiempo de reproducción.
            Clock::time_point current_time = Clock::now();
            std::chrono::duration<double> elapsed_seconds = current_time - start_time;
            while (pt_seconds > elapsed_seconds.count()) {
							SDL_Delay(0);
							current_time = Clock::now();
							elapsed_seconds = current_time - start_time;
            }
        }
    }

    if (state.decode_thread->joinable()) state.decode_thread->join();
    if (state.audio_thread->joinable()) state.audio_thread->join();

    SDL_DestroyAudioStream(audio_stream);
    video_reader_close(&state);
    SDL_DestroyWindow(state.window);
    SDL_Quit();

    return 0;
}
