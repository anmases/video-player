#include "video_reader.hpp"

bool video_reader_open(VideoState* state, const char* filename) {
    auto& width = state->width;
    auto& height = state->height;
    auto& video_time_base = state->video_time_base;
		auto& audio_time_base = state->audio_time_base;
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

		if (avformat_network_init() != 0) {
			cout << "Couldn't initialize network" << endl;
			return false;
		}

    // Abrir el archivo usando avformat
    format_context = avformat_alloc_context();
    if (!format_context) {
				cout << "Couldn't create AV format context" << endl;
        return false;
    }
		AVDictionary* options = nullptr;
    // Ajustes de red para buffering o retraso
    av_dict_set(&options, "buffer_size", "8192000", 0);  // Tamaño del buffer 8MB
    av_dict_set(&options, "max_delay", "500000", 0);     // Máxima demora 5s
		av_dict_set(&options, "rtbufsize", "104857600", 0);
		av_dict_set(&options, "hls_flags", "single_file", 0);
    if (avformat_open_input(&format_context, filename, NULL, &options) != 0) {
				cout << "Couldn't open video file" << endl;
				av_dict_free(&options);
        return false;
    }
		av_dict_free(&options);

		if (avformat_find_stream_info(format_context, nullptr) < 0) {
			cout << "Couldn't find stream information" << endl;
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
						audio_time_base = stream->time_base;
        }
    }

    if (video_stream_index == -1) {
				cout << "Couldn't find video stream" << endl;
        return false;
    }

    if (audio_stream_index == -1) {
				cout << "Couldn't find audio stream" << endl;
        return false;
    }

    // Configurar el contexto del códec de video
    video_codec_context = avcodec_alloc_context3(video_codec);
    if (!video_codec_context) {
				cout << "Couldn't create video codec context" << endl;
        return false;
    }
    if (avcodec_parameters_to_context(video_codec_context, format_context->streams[video_stream_index]->codecpar) < 0) {
				cout << "Couldn't initialize video codec context" << endl;
        return false;
    }
    if (avcodec_open2(video_codec_context, video_codec, nullptr) < 0) {
				cout << "Couldn't open video codec" << endl;
        return false;
    }

    // Configurar el contexto del códec de audio
    audio_codec_context = avcodec_alloc_context3(audio_codec);
    if (!audio_codec_context) {
				cout << "Couldn't create audio codec context" << endl;
        return false;
    }
    if (avcodec_parameters_to_context(audio_codec_context, format_context->streams[audio_stream_index]->codecpar) < 0) {
				cout << "Couldn't initialize audio codec context" << endl;
        return false;
    }
    if (avcodec_open2(audio_codec_context, audio_codec, nullptr) < 0) {
				cout << "Couldn't open audio codec" << endl;
        return false;
    }

		// Inicializar SwrContext para la conversión de formato de audio
		state->swr_context = swr_alloc();
		if (!state->swr_context) {
				cout << "Couldn't allocate SwrContext" << endl;
				return false;
		}
		// Configurar el SwrContext
		AVChannelLayout in_ch_layout = audio_codec_context->ch_layout;
		AVChannelLayout out_ch_layout;
		av_channel_layout_default(&out_ch_layout, audio_codec_context->ch_layout.nb_channels);  // Salida en estéreo
		int ret = av_opt_set_chlayout(state->swr_context, "in_chlayout", &in_ch_layout, 0);
		if (ret < 0) {
				char errbuf[AV_ERROR_MAX_STRING_SIZE];
				av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, ret);
				cout << "Error setting input channel layout: " << errbuf << endl;
				return false;
		}
		ret = av_opt_set_chlayout(state->swr_context, "out_chlayout", &out_ch_layout, 0);
		if (ret < 0) {
				char errbuf[AV_ERROR_MAX_STRING_SIZE];
				av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, ret);
				cout << "Error setting output channel layout: " << errbuf << endl;
				return false;
		}
		ret = av_opt_set_int(state->swr_context, "in_sample_rate", audio_codec_context->sample_rate, 0);
		if (ret < 0) {
				char errbuf[AV_ERROR_MAX_STRING_SIZE];
				av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, ret);
				cout << "Error setting input sample rate: " << errbuf << endl;
				return false;
		}
		ret = av_opt_set_int(state->swr_context, "out_sample_rate", audio_codec_context->sample_rate, 0);
		if (ret < 0) {
				char errbuf[AV_ERROR_MAX_STRING_SIZE];
				av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, ret);
				cout << "Error setting output sample rate: " << errbuf << endl;
				return false;
		}
		ret = av_opt_set_sample_fmt(state->swr_context, "in_sample_fmt", audio_codec_context->sample_fmt, 0);
		if (ret < 0) {
				char errbuf[AV_ERROR_MAX_STRING_SIZE];
				av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, ret);
				cout << "Error setting input sample format: " << errbuf << endl;
				return false;
		}
		ret = av_opt_set_sample_fmt(state->swr_context, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
		if (ret < 0) {
				char errbuf[AV_ERROR_MAX_STRING_SIZE];
				av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, ret);
				cout << "Error setting output sample format: " << errbuf << endl;
				return false;
		}
		if (swr_init(state->swr_context) < 0) {
				cout << "Couldn't initialize the SwrContext" << endl;
				swr_free(&state->swr_context);
				return false;
		}


    // Asignar memoria para frames y paquetes
    av_frame = av_frame_alloc();
    av_packet = av_packet_alloc();
    if (!av_packet || !av_frame) {
				cout << "Couldn't allocate memory for AV frame or AV packet" << endl;
        return false;
    }

    return true;
}

void video_reader_close(VideoState* state) {
	sws_freeContext(state->sws_context);
	swr_free(&state->swr_context);
	avformat_close_input(&state->format_context);
	avformat_free_context(state->format_context);
	avformat_network_deinit();
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
			cout << "Error sending video packet, response: " << response << endl;
			av_frame_free(&frame);
			return;
	}
	response = avcodec_receive_frame(state->video_codec_context, frame);
	if (response < 0) {
			cout << "Error receiving video packet, response: " << response << endl;
			av_frame_free(&frame);
			return;
	}

	// Almacenar el PTS (Presentation Timestamp) del frame
	double valid_pts = (frame->pts != AV_NOPTS_VALUE) ? frame->pts : frame->best_effort_timestamp;
	vf.pts = valid_pts;
	vf.width = frame->width;
	vf.height = frame->height;

	// Copiar los datos del frame (deep copy)
	for (int i = 0; i < AV_NUM_DATA_POINTERS && frame->data[i]; i++) {
			if (frame->data[i] == nullptr) {
					cout << "Invalid data pointer for plane " << i << endl;
					continue;
			}
			int plane_height = (i == 0) ? frame->height : (frame->height + 1) / 2;
			int num_bytes = frame->linesize[i] * plane_height;
			if (num_bytes <= 0) {
					cout << "Invalid size for plane " << i << ": " << num_bytes << endl;
					continue;
			}
			vf.data[i] = new uint8_t[num_bytes];
			memcpy(vf.data[i], frame->data[i], num_bytes);
			vf.linesize[i] = frame->linesize[i];
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
	if(response < 0){
		cout << "Couldn't receive audio frame: " << response << endl;
		av_frame_free(&frame);
		return;
	}

	double valid_pts = (frame->pts != AV_NOPTS_VALUE) ? frame->pts : frame -> best_effort_timestamp;
	ad.pts = valid_pts * av_q2d(state->audio_codec_context->time_base);
	ad.nb_samples = frame->nb_samples;

	
    // Vamos a copiar todos los planos de audio
    for (int i = 0; i < AV_NUM_DATA_POINTERS && frame->data[i]; i++) {
        int num_bytes = av_samples_get_buffer_size(
            nullptr, state->audio_codec_context->ch_layout.nb_channels, 
            frame->nb_samples, 
            state->audio_codec_context->sample_fmt, 1
        );
        if (num_bytes <= 0) {
            cout << "Invalid size for audio plane: " << i << " : " << num_bytes << endl;
            continue;
        }
				ad.size[i] = num_bytes;
        ad.data[i] = new uint8_t[num_bytes];
        memcpy(ad.data[i], frame->data[i], num_bytes);
    }
	
	av_frame_free(&frame);
}

void render_video_frame(VideoState* state, const VideoFrame& vf) {
	// Si no se ha inicializado el contexto de escalado, inicializarlo ahora
	if (!state->sws_context) {
			state->sws_context = sws_getContext(
					vf.width, vf.height,
					state->video_codec_context->pix_fmt,
					vf.width, vf.height,
					AV_PIX_FMT_RGB0,  // Puedes ajustar el formato de destino
					SWS_BILINEAR,
					nullptr, nullptr, nullptr
			);
			if (!state->sws_context) {
					cout << "Couldn't initialize SW scaler" << endl;
					return;
			}
	}

	// Calcular el tamaño del buffer de salida (escalado)
	unsigned int num_bytes = vf.width * vf.height * 4; // 4 bytes por píxel (RGBA)
	uint8_t* scaled_data = new uint8_t[num_bytes];

	// Preparar los buffers para `sws_scale`
	uint8_t* dest[4] = { scaled_data, nullptr, nullptr, nullptr };
	int dest_linesize[4] = { vf.width * 4, 0, 0, 0 }; // Ancho de la imagen * 4 bytes por píxel (RGBA)

	// Realizar la conversión y escalado
	int result = sws_scale(state->sws_context, vf.data, vf.linesize, 0, vf.height, dest, dest_linesize);
	if (result <= 0) {
			cout << "sws_scale failed with error code: " << result << endl;
			delete[] scaled_data;
			return;
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, vf.width, vf.height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, vf.width, vf.height, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);

	// Renderizar el frame escalado usando OpenGL
	glBindTexture(GL_TEXTURE_2D, state->texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, vf.width, vf.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled_data);

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

	delete[] scaled_data;
}

unsigned int video_refresh_timer(void* userdata, SDL_TimerID timerID, Uint32 interval) {
    VideoState* state = (VideoState*)userdata;

    if (!state->quit) {
        VideoFrame vf;
        if (state->video_queue.dequeue(vf)) {
            render_video_frame(state, vf);
            for (int i = 0; i < AV_NUM_DATA_POINTERS; i++) {
							if (vf.data[i]) {
									delete[] vf.data[i]; 
									vf.data[i] = nullptr;
							}
						}

            SDL_AddTimer(interval, video_refresh_timer, state);
        }
    }
    return interval;
}
