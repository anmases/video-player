#ifndef video_reader_hpp
#define video_reader_hpp

extern "C" {
    #include <libavcodec/avcodec.h>
		#include <libavformat/avformat.h>
    #include <libavutil/avutil.h>
    #include <libswscale/swscale.h>
    #include <libswresample/swresample.h>
		#include <libavutil/opt.h>
		#include <inttypes.h>
}

struct VideoReaderState {
    int width;
    int height;

    AVRational time_base;
    AVFormatContext* format_context;
    AVCodecContext* video_codec_context;
    AVCodecContext* audio_codec_context;
    SwsContext* sws_context;
		SwrContext* swr_context;
    AVFrame* av_frame;
    AVPacket* av_packet;
    int video_stream_index;
    int audio_stream_index;
};

bool video_reader_open(VideoReaderState* state, const char* filename);
bool video_reader_read_frame(VideoReaderState* state, uint8_t* frame_buffer, int64_t* pts);
bool video_reader_read_audio(VideoReaderState* state, uint8_t** audio_data, int* audio_size);
void video_reader_close(VideoReaderState* state);

#endif