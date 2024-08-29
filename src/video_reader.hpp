#ifndef video_reader_hpp
#define video_reader_hpp

#include <condition_variable>
#include <mutex>
extern "C" {
    #include <libavcodec/avcodec.h>
		#include <libavformat/avformat.h>
    #include <libavutil/avutil.h>
    #include <libswscale/swscale.h>
    #include <libswresample/swresample.h>
		#include <libavutil/opt.h>
		#include <inttypes.h>
		#include <SDL3/SDL.h>
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
		AVFrame* audio_frame;
    AVPacket* audio_packet;
    int video_stream_index;
    int audio_stream_index;
		//Para usar multihilo.
		std::mutex mutex;
    std::condition_variable cond;
};

bool video_reader_open(VideoReaderState* state, const char* filename);
bool video_reader_read_frame(VideoReaderState* state, uint8_t* frame_buffer, int64_t* video_pts);
bool video_reader_read_audio(VideoReaderState* state, uint8_t** audio_data, int* audio_size, int64_t* audio_pts);
void video_reader_close(VideoReaderState* state);

#endif