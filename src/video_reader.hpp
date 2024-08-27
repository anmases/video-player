#ifndef video_reader_hpp
#define video_reader_hpp

extern "C" {
    #include <libavcodec/avcodec.h>
		#include <libavformat/avformat.h>
    #include <libavutil/avutil.h>
    #include <libswscale/swscale.h>
    #include <libswresample/swresample.h>
		#include <inttypes.h>
}

struct VideoReaderState {
	int width;
	int height;


	AVFormatContext* format_context;
	AVCodecContext* codec_context;
	SwsContext* sws_context;
	AVFrame* av_frame;
	AVPacket* av_packet;
	int video_stream_index;
};

bool video_reader_open(VideoReaderState* state, const char* filename);
bool video_reader_read_frame(VideoReaderState* state, uint8_t* frame_buffer);
void video_reader_close(VideoReaderState* state);

#endif