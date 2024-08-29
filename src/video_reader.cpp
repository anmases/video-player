#include "video_reader.hpp"

bool video_reader_open(VideoReaderState* state, const char* filename) {
    auto& width = state->width;
    auto& height = state->height;
    auto& time_base = state->time_base;
    auto& format_context = state->format_context;
    auto& video_codec_context = state->video_codec_context;
    auto& audio_codec_context = state->audio_codec_context;
    auto& av_frame = state->av_frame;
    auto& av_packet = state->av_packet;
    auto& audio_frame = state->audio_frame;
    auto& audio_packet = state->audio_packet;
    auto& sws_context = state->sws_context;
    auto& swr_context = state->swr_context;
    auto& video_stream_index = state->video_stream_index;
    auto& audio_stream_index = state->audio_stream_index;

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
            time_base = stream->time_base;
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
    audio_frame = av_frame_alloc();
    audio_packet = av_packet_alloc();
    if (!av_packet || !av_frame) {
        printf("Couldn't allocate memory for audio frame or audio packet\n");
        return false;
    }
    

    return true;
}

bool video_reader_read_frame(VideoReaderState* state, uint8_t* frame_buffer, int64_t* video_pts) {
    auto& width = state->width;
    auto& height = state->height;
    auto& format_context = state->format_context;
    auto& video_codec_context = state->video_codec_context;
    auto& av_frame = state->av_frame;
    auto& av_packet = state->av_packet;
    auto& sws_context = state->sws_context;
    auto& video_stream_index = state->video_stream_index;

    // Decode one frame
    int response;
    while (av_read_frame(format_context, av_packet) >= 0) {
        if (av_packet->stream_index != video_stream_index) {
            av_packet_unref(av_packet);
            continue;
        }
        response = avcodec_send_packet(video_codec_context, av_packet);
        if (response < 0) {
            char errbuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(response, errbuf, sizeof(errbuf));
            printf("Failed to decode video packet: %s\n", errbuf);
            return false;
        }
        response = avcodec_receive_frame(video_codec_context, av_frame);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
            av_packet_unref(av_packet);
            continue;
        } else if (response < 0) {
            char errbuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(response, errbuf, sizeof(errbuf));
            printf("Failed to decode video packet: %s\n", errbuf);
            return false;
        }
        av_packet_unref(av_packet);
        break;
    }

    // Presentation timestamp
    *video_pts = av_frame->pts;

    sws_context = sws_getContext(
        width, // Tamaño origen
        height,
        video_codec_context->pix_fmt,
        width, // Tamaño destino
        height,
        AV_PIX_FMT_RGB0,
        SWS_BILINEAR,
        nullptr, nullptr, nullptr
    );
    if (!sws_context) {
        printf("Couldn't initialize SW scaler\n");
        return false;
    }

    uint8_t* dest[4] = { frame_buffer, NULL, NULL, NULL };
    int dest_linesize[4] = { av_frame->width * 4, 0, 0, 0 };
    sws_scale(sws_context, av_frame->data, av_frame->linesize, 0, av_frame->height, dest, dest_linesize);
    return true;
}

bool video_reader_read_audio(VideoReaderState* state, uint8_t** audio_data, int* audio_size, int64_t* audio_pts) {
    auto& audio_codec_context = state->audio_codec_context;
    auto& audio_frame = state->audio_frame;
    auto& audio_packet = state->audio_packet;
    auto& swr_context = state->swr_context;

    int response;
    while (av_read_frame(state->format_context, audio_packet) >= 0) {
        if (audio_packet->stream_index == state->audio_stream_index) {
            response = avcodec_send_packet(audio_codec_context, audio_packet);
            if (response < 0) {
                char errbuf[AV_ERROR_MAX_STRING_SIZE];
                av_strerror(response, errbuf, sizeof(errbuf));
                printf("Failed to decode audio packet: %s\n", errbuf);
                return false;
            }
            response = avcodec_receive_frame(audio_codec_context, audio_frame);
            if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
                av_packet_unref(audio_packet);
                continue;
            } else if (response < 0) {
                char errbuf[AV_ERROR_MAX_STRING_SIZE];
                av_strerror(response, errbuf, sizeof(errbuf));
                printf("Failed to decode audio frame: %s\n", errbuf);
                return false;
            }

						*audio_pts = audio_frame->pts;

            int dst_nb_samples = av_rescale_rnd(
                swr_get_delay(swr_context, audio_codec_context->sample_rate) + audio_frame->nb_samples,
                audio_codec_context->sample_rate, audio_codec_context->sample_rate, AV_ROUND_UP);

            int data_size = av_samples_get_buffer_size(
                nullptr, audio_frame->ch_layout.nb_channels, dst_nb_samples,
                AV_SAMPLE_FMT_S16, 1
            );

            *audio_data = new uint8_t[data_size];
            if (swr_convert(swr_context, audio_data, dst_nb_samples, (const uint8_t**)audio_frame->data, audio_frame->nb_samples) < 0) {
                printf("Error converting audio\n");
                return false;
            }

            *audio_size = data_size;
            av_packet_unref(audio_packet);
            return true;
        } else {
            av_packet_unref(audio_packet);
        }
    }

    return false;
}

void video_reader_close(VideoReaderState* state) {
    // Clean codec and format context
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