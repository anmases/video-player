#include <stdio.h>
extern "C" {
    #include <libavcodec/avcodec.h>
		#include <libavformat/avformat.h>
    #include <libavutil/avutil.h>
    #include <libswscale/swscale.h>
    #include <libswresample/swresample.h>
		#include <inttypes.h>
}

bool load_frame(const char* filename, int* width, int* height, unsigned char** data){
	AVFormatContext* format_context = avformat_alloc_context();
	if(!format_context){
		printf("Couldn't create AV format context\n");
		return false;
	}
	if(avformat_open_input(&format_context, filename, NULL, NULL) != 0){
		printf("couldn't open video file\n");
		return false;
	}

	int video_stream_index = -1;
	const AVCodec* av_codec;
	for(int i=0; i< format_context->nb_streams; i++){
		auto stream = format_context->streams[i];
		AVCodecParameters* av_codec_params = stream->codecpar;
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
		printf("couldn't find video stream index");
		return false;
	}
	return true;
}