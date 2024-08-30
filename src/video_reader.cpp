#include "video_reader.hpp"

bool video_reader_open(VideoState* state, const char* filename) {
    auto& width = state->width;
    auto& height = state->height;
    auto& video_time_base = state->video_time_base;
    auto& format_context = state->format_context;
    auto& video_codec_context = state->video_codec_context;
    auto& audio_codec_context = state->audio_codec_context;
    auto& av_frame = state->av_frame;
    auto& av_packet = state->av_packet;
    auto& sws_context = state->sws_context;
		auto& swr_context = state->swr_context;
    auto& video_stream_index = state->video_stream_index;
    auto& audio_stream_index = state->audio_stream_index;

		state->quit = false;

    // Abrir el archivo usando avformat
    format_context = avformat_alloc_context();
    if (!format_context) {
        printf("Couldn't create AV format context\n");
        return false;
    }
    if (avformat_open_input(&format_context, filename, NULL, NULL) != 0) {
        printf("Couldn't open video file\n");
        return false;
    }

    // Inicializar variables
    video_stream_index = -1;
    audio_stream_index = -1;

    const AVCodec* video_codec = nullptr;
    const AVCodec* audio_codec = nullptr;
    AVCodecParameters* av_codec_params = nullptr;

    // Encontrar el flujo de video y audio
    for (int i = 0; i < format_context->nb_streams; i++) {
        AVStream* stream = format_context->streams[i];
        av_codec_params = stream->codecpar;
        const AVCodec* av_codec = avcodec_find_decoder(av_codec_params->codec_id);
        if (!av_codec) {
            continue;
        }
        if (av_codec_params->codec_type == AVMEDIA_TYPE_VIDEO && video_stream_index == -1) {
            video_stream_index = i;
            video_codec = av_codec;
            width = av_codec_params->width;
            height = av_codec_params->height;
            video_time_base = stream->time_base;
        }
        if (av_codec_params->codec_type == AVMEDIA_TYPE_AUDIO && audio_stream_index == -1) {
            audio_stream_index = i;
            audio_codec = av_codec;
        }
    }

    if (video_stream_index == -1) {
        printf("Couldn't find video stream\n");
        return false;
    }

    if (audio_stream_index == -1) {
        printf("Couldn't find audio stream\n");
        return false;
    }

    // Configurar el contexto del códec de video
    video_codec_context = avcodec_alloc_context3(video_codec);
    if (!video_codec_context) {
        printf("Couldn't create video codec context\n");
        return false;
    }
    if (avcodec_parameters_to_context(video_codec_context, format_context->streams[video_stream_index]->codecpar) < 0) {
        printf("Couldn't initialize video codec context\n");
        return false;
    }
    if (avcodec_open2(video_codec_context, video_codec, nullptr) < 0) {
        printf("Couldn't open video codec\n");
        return false;
    }

    // Configurar el contexto del códec de audio
    audio_codec_context = avcodec_alloc_context3(audio_codec);
    if (!audio_codec_context) {
        printf("Couldn't create audio codec context\n");
        return false;
    }
    if (avcodec_parameters_to_context(audio_codec_context, format_context->streams[audio_stream_index]->codecpar) < 0) {
        printf("Couldn't initialize audio codec context\n");
        return false;
    }
    if (avcodec_open2(audio_codec_context, audio_codec, nullptr) < 0) {
        printf("Couldn't open audio codec\n");
        return false;
    }

	// Inicializar SwrContext para la conversión de formato de audio
state->swr_context = swr_alloc();
if (!state->swr_context) {
    printf("Couldn't allocate SwrContext\n");
    return false;
}
// Configurar el SwrContext
AVChannelLayout in_ch_layout = audio_codec_context->ch_layout;
AVChannelLayout out_ch_layout;
av_channel_layout_default(&out_ch_layout, 2);  // Salida en estéreo
int ret = av_opt_set_chlayout(state->swr_context, "in_chlayout", &in_ch_layout, 0);
if (ret < 0) {
    char errbuf[AV_ERROR_MAX_STRING_SIZE];
    av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, ret);
    printf("Error setting input channel layout: %s\n", errbuf);
    return false;
}
ret = av_opt_set_chlayout(state->swr_context, "out_chlayout", &out_ch_layout, 0);
if (ret < 0) {
    char errbuf[AV_ERROR_MAX_STRING_SIZE];
    av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, ret);
    printf("Error setting output channel layout: %s\n", errbuf);
    return false;
}
ret = av_opt_set_int(state->swr_context, "in_sample_rate", audio_codec_context->sample_rate, 0);
if (ret < 0) {
    char errbuf[AV_ERROR_MAX_STRING_SIZE];
    av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, ret);
    printf("Error setting input sample rate: %s\n", errbuf);
    return false;
}
ret = av_opt_set_int(state->swr_context, "out_sample_rate", audio_codec_context->sample_rate, 0);
if (ret < 0) {
    char errbuf[AV_ERROR_MAX_STRING_SIZE];
    av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, ret);
    printf("Error setting output sample rate: %s\n", errbuf);
    return false;
}
ret = av_opt_set_sample_fmt(state->swr_context, "in_sample_fmt", audio_codec_context->sample_fmt, 0);
if (ret < 0) {
    char errbuf[AV_ERROR_MAX_STRING_SIZE];
    av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, ret);
    printf("Error setting input sample format: %s\n", errbuf);
    return false;
}
ret = av_opt_set_sample_fmt(state->swr_context, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
if (ret < 0) {
    char errbuf[AV_ERROR_MAX_STRING_SIZE];
    av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, ret);
    printf("Error setting output sample format: %s\n", errbuf);
    return false;
}
if (swr_init(state->swr_context) < 0) {
    printf("Couldn't initialize the SwrContext\n");
    swr_free(&state->swr_context);
    return false;
}


    // Asignar memoria para frames y paquetes
    av_frame = av_frame_alloc();
    av_packet = av_packet_alloc();
    if (!av_packet || !av_frame) {
        printf("Couldn't allocate memory for AV frame or AV packet\n");
        return false;
    }

    return true;
}


void video_reader_close(VideoState* state) {
    // Limpiar el contexto de códec y formato
    sws_freeContext(state->sws_context);
    swr_free(&state->swr_context);
    avformat_close_input(&state->format_context);
    avformat_free_context(state->format_context);
    av_frame_free(&state->av_frame);
    av_packet_free(&state->av_packet);
    av_frame_free(&state->audio_frame);
    av_packet_free(&state->audio_packet);
    avcodec_free_context(&state->video_codec_context);
    avcodec_free_context(&state->audio_codec_context);
}

void decode_video_packet(VideoState* state, AVPacket* packet, VideoFrame& vf) {
    AVFrame* frame = av_frame_alloc();
    int response = avcodec_send_packet(state->video_codec_context, packet);
    if (response < 0) {
        av_frame_free(&frame);
        return;
    }
    response = avcodec_receive_frame(state->video_codec_context, frame);
    if (response >= 0) {
        vf.width = frame->width;
        vf.height = frame->height;
        vf.pts = frame->pts;

        // Calcular el tamaño de la imagen
        unsigned int num_bytes = vf.width * vf.height * 4;
        vf.data = new uint8_t[num_bytes];

        uint8_t* dest[4] = { vf.data, nullptr, nullptr, nullptr };
        int dest_linesize[4] = { frame->width * 4, 0, 0, 0 };

        // Inicializar o reutilizar el sws_context si no está inicializado
        if (!state->sws_context || state->sws_context == 0) {
						printf("initialize scaler");
            state->sws_context = sws_getContext(
                frame->width,
                frame->height,
                state->video_codec_context->pix_fmt,
                frame->width,
                frame->height,
                AV_PIX_FMT_RGB0,
                SWS_BILINEAR,
                nullptr, nullptr, nullptr
            );

            if (!state->sws_context) {
                printf("Couldn't initialize SW scaler\n");
                av_frame_free(&frame);
                delete[] vf.data; // Liberar si se asignó
                vf.data = nullptr; // Evitar doble liberación
                return;
            }
        }

        // Realizar la conversión de escalado
        int result = sws_scale(state->sws_context, frame->data, frame->linesize, 0, frame->height, dest, dest_linesize);
        if (result <= 0) {
            printf("sws_scale failed with error code: %d\n", result);
            delete[] vf.data; // Liberar si falla la conversión
            vf.data = nullptr; // Evitar doble liberación
        }
    }
    av_frame_free(&frame);
}




void decode_audio_packet(VideoState* state, AVPacket* packet, AudioData& ad) {
    AVFrame* frame = av_frame_alloc();
    int response = avcodec_send_packet(state->audio_codec_context, packet);
    if (response < 0) {
        av_frame_free(&frame);
        return;
    }
    response = avcodec_receive_frame(state->audio_codec_context, frame);
    if (response >= 0) {
        int dst_nb_samples = av_rescale_rnd(
            swr_get_delay(state->swr_context, state->audio_codec_context->sample_rate) + frame->nb_samples,
            state->audio_codec_context->sample_rate,
            state->audio_codec_context->sample_rate,
            AV_ROUND_UP
        );
        int data_size = av_samples_get_buffer_size(nullptr, state->audio_codec_context->ch_layout.nb_channels, dst_nb_samples, AV_SAMPLE_FMT_S16, 1);
        ad.data = new uint8_t[data_size];
        ad.size = data_size;
        ad.pts = frame->pts * av_q2d(state->audio_codec_context->time_base);
        uint8_t* out[] = { ad.data };
        swr_convert(state->swr_context, out, dst_nb_samples, (const uint8_t**)frame->data, frame->nb_samples);
    }
    av_frame_free(&frame);
}


void render_video_frame(VideoState* state, const VideoFrame& vf) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, vf.width, vf.height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, vf.width, vf.height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);

    glBindTexture(GL_TEXTURE_2D, state->texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, vf.width, vf.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, vf.data);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, state->texture);
    glBegin(GL_QUADS);
    glTexCoord2d(0, 0); glVertex2i(0, 0);
    glTexCoord2d(1, 0); glVertex2i(vf.width, 0);
    glTexCoord2d(1, 1); glVertex2i(vf.width, vf.height);
    glTexCoord2d(0, 1); glVertex2i(0, vf.height);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    SDL_GL_SwapWindow(state->window);
}

unsigned int video_refresh_timer(void* userdata, SDL_TimerID timerID, Uint32 interval) {
    VideoState* state = (VideoState*)userdata;

    if (!state->quit) {
        VideoFrame vf;
        if (state->video_queue.dequeue(vf)) {
            render_video_frame(state, vf);
            delete[] vf.data;

            SDL_AddTimer(interval, video_refresh_timer, state);
        }
    }
    return interval;
}
