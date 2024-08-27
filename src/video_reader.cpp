#include "video_reader.hpp"

bool video_reader_open(VideoReaderState* state, const char* filename){
	auto& width = state->width;
	auto& height = state->height;
	auto& format_context = state->format_context;
	auto& codec_context = state->codec_context;
	auto& av_frame = state->av_frame;
	auto& av_packet = state->av_packet;
	auto& sws_context = state->sws_context;
	auto& video_stream_index = state->video_stream_index;
//open the file using avformat
	format_context = avformat_alloc_context();
	if(!format_context){
		printf("Couldn't create AV format context\n");
		return false;
	}
	if(avformat_open_input(&format_context, filename, NULL, NULL) != 0){
		printf("couldn't open video file\n");
		return false;
	}

	//find the video stream.
	video_stream_index = -1;
	const AVCodec* av_codec;
	AVCodecParameters* av_codec_params;
	for(int i=0; i< format_context->nb_streams; i++){
		AVStream* stream = format_context->streams[i];
		av_codec_params = stream->codecpar;
		av_codec = avcodec_find_decoder(av_codec_params->codec_id);
		if(!av_codec){
			continue;
		}
		if(av_codec_params->codec_type == AVMEDIA_TYPE_VIDEO){
			video_stream_index = i;
			width = av_codec_params->width;
			height = av_codec_params->height;
			break;
		}
	}

	if(video_stream_index == -1){
		printf("couldn't find video stream index\n");
		return false;
	}

	//Set up codec context for the decoder.
	codec_context = avcodec_alloc_context3(av_codec);
	if(!codec_context){
		printf("Couldn't create codec context\n");
		return false;
	}
	if(avcodec_parameters_to_context(codec_context, av_codec_params) < 0){
		printf("Couldn't initialize codec context\n");
		return false;
	}
	if(!avcodec_open2(codec_context, av_codec, nullptr) < 0){
		printf("Couldn't open codec\n");
		return false;
	}

	av_frame = av_frame_alloc();
	av_packet = av_packet_alloc();
	if(!av_packet || !av_frame){
		printf("Couldn't no longer allocate info.\n");
		return false;
	}
	
	

		return true;	
}

bool video_reader_read_frame(VideoReaderState* state, uint8_t* frame_buffer){
	auto& width = state->width;
	auto& height = state->height;
	auto& format_context = state->format_context;
	auto& codec_context = state->codec_context;
	auto& av_frame = state->av_frame;
	auto& av_packet = state->av_packet;
	auto& sws_context = state->sws_context;
	auto& video_stream_index = state->video_stream_index;
	// decode one frame
	int response;
	while(av_read_frame(format_context, av_packet) >= 0){
		if(av_packet->stream_index != video_stream_index){
			continue;
		}
		response = avcodec_send_packet(codec_context, av_packet);
		if(response < 0){
				char errbuf[AV_ERROR_MAX_STRING_SIZE];
				av_strerror(response, errbuf, sizeof(errbuf));
				printf("failed to decode packet: %s\n", errbuf);
				return false;
		}
		response = avcodec_receive_frame(codec_context, av_frame);
		if(response == AVERROR(EAGAIN) || response == AVERROR_EOF){
				continue;
		} else if(response < 0){
				char errbuf[AV_ERROR_MAX_STRING_SIZE];
				av_strerror(response, errbuf, sizeof(errbuf));
				printf("failed to decode packet: %s\n", errbuf);
				return false;
		}
		av_packet_unref(av_packet);
		break;
	}
	sws_context = sws_getContext(
		width, //tamaño origen
		height,
		codec_context->pix_fmt,
		width, //tamaño destino
		height,
		AV_PIX_FMT_RGB0,
		SWS_BILINEAR,
		nullptr, nullptr, nullptr
	);
	if(!sws_context){
		printf("couldn't initialize sw scaler\n");
		return false;
	}

	uint8_t* dest[4] = {frame_buffer, NULL, NULL, NULL};
	int dest_linesize[4] = {av_frame->width * 4, 0, 0, 0};
	sws_scale(sws_context, av_frame->data, av_frame->linesize, 0, av_frame->height, dest, dest_linesize);


	//*width_out = av_frame->width;
	//*height_out = av_frame->height;
	//*data_out = data;

	return true;
}

void video_reader_close(VideoReaderState* state){
	//Clean codec and format context
	sws_freeContext(state->sws_context);
	avformat_close_input(&state->format_context);
	avformat_free_context(state->format_context);
	av_frame_free(&state->av_frame);
	av_packet_free(&state->av_packet);
	avcodec_free_context(&state->codec_context);
}