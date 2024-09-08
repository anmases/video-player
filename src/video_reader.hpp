#ifndef video_reader_hpp
#define video_reader_hpp

#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <atomic>
#include <chrono>
#include <GL/gl.h>
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

using namespace std;

struct VideoFrame {
    uint8_t* data = nullptr;
    int width;
    int height;
    double pts;
};

struct AudioData {
    uint8_t* data = nullptr;
    int size;
    double pts;
};

template <typename T>
class SafeQueue {
private:
    std::queue<T> q;
    mutable std::mutex mtx;
    std::condition_variable cv;
    size_t maxSize;
		
public:
    SafeQueue(size_t maxSize) : maxSize(maxSize) {}

    void enqueue(const T& item) {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [this] { return q.size() < maxSize; });
        q.push(item);
        cv.notify_one();
    }

    bool dequeue(T& item) {
        unique_lock<mutex> lock(mtx);
        if (q.empty()) return false;
        item = q.front();
        q.pop();
        cv.notify_one();
        return true;
    }

    void wait_dequeue(T& item) {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [this] { return !q.empty(); });
        item = q.front();
        q.pop();
        cv.notify_one();
    }

    bool empty() {
        lock_guard<mutex> lock(mtx);
        return q.empty();
    }

    bool full() {
        lock_guard<mutex> lock(mtx);
        return q.size() >= maxSize;
    }

    void clear() {
        lock_guard<mutex> lock(mtx);
        while (!q.empty()) q.pop();
    }

		size_t size() const {
			lock_guard<mutex> lock(mtx);
			return q.size();
    }
};




struct VideoState {
    int width;
    int height;
    AVRational video_time_base;
		AVRational audio_time_base;
    AVFormatContext* format_context = nullptr;
    AVCodecContext* video_codec_context = nullptr;
    AVCodecContext* audio_codec_context = nullptr;
    SwsContext* sws_context = nullptr;
    SwrContext* swr_context = nullptr;
    AVFrame* av_frame = nullptr;
    AVPacket* av_packet = nullptr;
    AVFrame* audio_frame = nullptr;
    AVPacket* audio_packet = nullptr;
    int video_stream_index;
    int audio_stream_index;

    SDL_AudioDeviceID audio_device;
    SDL_Window* window = nullptr;
    SDL_GLContext gl_context;

    SafeQueue<VideoFrame> video_queue;
    SafeQueue<AudioData> audio_queue;

    atomic<bool> quit;
    thread* decode_thread = nullptr;
    thread* video_thread = nullptr;
    thread* audio_thread = nullptr;

    GLuint texture;

    double audio_clock; // PTS del audio
    double video_clock; // PTS del video

		// Constructor para inicializar las colas con un tamaño máximo
    VideoState() : video_queue(2000), audio_queue(2000) {}

    // Deshabilitar la copia de VideoState
    VideoState(const VideoState&) = delete;
    VideoState& operator=(const VideoState&) = delete;
};


bool video_reader_open(VideoState* state, const char* filename);
void video_reader_close(VideoState* state);

void decode_video_packet(VideoState* state, AVPacket* packet, VideoFrame& vf);
void decode_audio_packet(VideoState* state, AVPacket* packet, AudioData& ad);

unsigned int video_refresh_timer(void* userdata, SDL_TimerID timerID, Uint32 interval);
void render_video_frame(VideoState* state, const VideoFrame& vf);

#endif