#include <stdio.h>
extern "C" {
    #include <libavcodec/avcodec.h>
		#include <libavformat/avformat.h>
    #include <libavutil/avutil.h>
    #include <libswscale/swscale.h>
    #include <libswresample/swresample.h>
		#include <inttypes.h>
}

bool load_frame(const char* filename, int* width_out, int* height_out, unsigned char** data_out){
	//open the file using avformat
	AVFormatContext* format_context = avformat_alloc_context();
	if(!format_context){
		printf("Couldn't create AV format context\n");
		return false;
	}
	if(avformat_open_input(&format_context, filename, NULL, NULL) != 0){
		printf("couldn't open video file\n");
		return false;
	}

	//find the video stream.
	int video_stream_index = -1;
	const AVCodec* av_codec;
	AVCodecParameters* av_codec_params;
	for(int i=0; i< format_context->nb_streams; i++){
		auto stream = format_context->streams[i];
		av_codec_params = stream->codecpar;
		av_codec = avcodec_find_decoder(av_codec_params->codec_id);
		if(!av_codec){
			continue;
		}
		if(av_codec_params->codec_type == AVMEDIA_TYPE_VIDEO){
			video_stream_index = i;
			break;
		}
	}

	if(video_stream_index == -1){
		printf("couldn't find video stream index\n");
		return false;
	}

	//Set up codec context for the decoder.
	AVCodecContext* codec_context = avcodec_alloc_context3(av_codec);
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

	AVFrame* av_frame = av_frame_alloc();
	AVPacket* av_packet = av_packet_alloc();
	if(!av_packet || !av_frame){
		printf("Couldn't no longer allocate info.\n");
		return false;
	}

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
	//buffer for 4 bytes (RGBA)
	uint8_t* data = new uint8_t[av_frame->width * av_frame->height * 4];
	SwsContext* sws_context = sws_getContext(
			av_frame->width, //tamaño origen
			av_frame->height,
			codec_context->pix_fmt,
			av_frame->width, //tamaño destino
			av_frame->height,
			AV_PIX_FMT_RGB0,
			SWS_BILINEAR,
			nullptr, nullptr, nullptr
		);
	if(!sws_context){
		printf("couldn't initialize sw scaler\n");
		return false;
	}
	uint8_t* dest[4] = {data, NULL, NULL, NULL};
	int dest_linesize[4] = {av_frame->width * 4, 0, 0, 0};
	sws_scale(sws_context, av_frame->data, av_frame->linesize, 0, av_frame->height, dest, dest_linesize);

	*width_out = av_frame->width;
	*height_out = av_frame->height;
	*data_out = data;

	//Clean codec and format context
	sws_freeContext(sws_context);
	avformat_close_input(&format_context);
	avformat_free_context(format_context);
	av_frame_free(&av_frame);
	av_packet_free(&av_packet);
	avcodec_free_context(&codec_context);
	return true;
}