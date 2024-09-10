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

/// @brief Contains the basic info from a video frame.
/// data contains the YUV info from the frame.
class VideoFrame {
public:
	uint8_t* data[AV_NUM_DATA_POINTERS] = {nullptr};
	int linesize[AV_NUM_DATA_POINTERS] = {0};
	int height;
	int width;
	double pts;

	/// @brief Default constructor.
	VideoFrame() : height(0), width(0), pts(0.0) {
			for (int i = 0; i < AV_NUM_DATA_POINTERS; i++) {
					data[i] = nullptr;
					linesize[i] = 0;
			}
	}
	/// @brief Deep copy constructor
	/// @param other 
	VideoFrame(const VideoFrame& other) {
        deep_copy(other);
    }
	/// @brief Deep copy asignation operator.
	/// @param other
	VideoFrame& operator = (const VideoFrame& other) {
			if (this != &other) {
					for (int i = 0; i < AV_NUM_DATA_POINTERS; i++) {
							delete[] data[i];
							data[i] = nullptr;
					}
					deep_copy(other);
			}
			return *this;
	}
	/// Default destructor to release dynamic memory.
	~VideoFrame() {
			for (int i = 0; i < AV_NUM_DATA_POINTERS; i++) {
					linesize[i] = 0;
					delete[] data[i];
					data[i] = nullptr;
			}
			height = 0;
			width = 0;
			pts = 0.0;
	}
	/// @brief Makes a deep copy from other instance.
	/// @param other 
	/// @return true if copy has been completed, false if there is an error.
	bool deep_copy(const VideoFrame& other){
		try{
			width = other.width;
			height = other.height;
			pts = other.pts;
			for (int i = 0; i < AV_NUM_DATA_POINTERS; i++) {
				if (other.data[i] != nullptr) {
						int plane_height = (i == 0) ? other.height : (other.height + 1) / 2;
						int num_bytes = other.linesize[i] * plane_height;
						data[i] = new uint8_t[num_bytes];
						if (num_bytes <= 0) {
							cout << "Invalid size for plane " << i << ": " << num_bytes << endl;
							continue;
						}
						memcpy(data[i], other.data[i], num_bytes);
						linesize[i] = other.linesize[i];
				} else {
						data[i] = nullptr;
						linesize[i] = 0;
				}
			}
			return true;
		}catch(exception& e){
			cerr << "Couldn't copy VideoFrame: " << e.what() << std::endl;
			return false;
		}
	}
};

/// @brief Class that contains basic audio chunk data.
class AudioData {

public:
	uint8_t* data[AV_NUM_DATA_POINTERS];
	int size[AV_NUM_DATA_POINTERS];
	double pts;
	int nb_samples;
	/// @brief Default constructor
	AudioData() : pts(0.0), nb_samples(0){
		for (int i = 0; i < AV_NUM_DATA_POINTERS; i++) {
				size[i] = 0;
				data[i] = nullptr;
		}
	}
	/// @brief Deep copy constructor
	/// @param other 
	AudioData(const AudioData& other) {
		deep_copy(other);
	}
	AudioData& operator=(const AudioData& other) {
		if (this != &other) {
				release_data();  // Liberar la memoria actual antes de copiar
				deep_copy(other);
		}
		return *this;
	}
	/// @brief Default destructor to release dynamic memory.
	~AudioData(){
		release_data();
	}

private:
	/// @brief Makes a deep copy from other instance.
	/// @param other 
	/// @return true if copy has been completed, false if there is an error.
	void deep_copy(const AudioData& other) {
		pts = other.pts;
		nb_samples = other.nb_samples;
		for (int i = 0; i < AV_NUM_DATA_POINTERS; ++i) {
				if (other.data[i] != nullptr && other.size[i] > 0) {
						size[i] = other.size[i];
						data[i] = new uint8_t[size[i]];
						memcpy(data[i], other.data[i], size[i]);
				} else {
						data[i] = nullptr;
						size[i] = 0;
				}
		}
	}
	/// @brief Releases data from deep object fields.
	void release_data() {
		pts = 0.0;
		nb_samples = 0;
		for (int i = 0; i < AV_NUM_DATA_POINTERS; ++i) {
				delete[] data[i];
				data[i] = nullptr;
				size[i] = 0;
		}
	}
};

/// @brief Thread-safe queue with a maximum size limit.
/// This template class provides a thread-safe queue implementation using
/// mutexes and condition variables. The queue blocks enqueue operations if the
/// queue is full and blocks dequeue operations if the queue is empty.
template <typename T>
class SafeQueue {
private:
    /// @brief Internal queue to hold items.
    std::queue<T> q;

    /// @brief Mutex for thread-safe access to the queue.
    mutable std::mutex mtx;

    /// @brief Condition variable for signaling enqueue and dequeue operations.
    std::condition_variable cv;

    /// @brief Maximum size of the queue.
    size_t maxSize;

public:
    /// @brief Constructor to initialize the queue with a maximum size.
    /// @param maxSize Maximum number of items that can be enqueued.
    SafeQueue(size_t maxSize) : maxSize(maxSize) {}

    /// @brief Adds an item to the queue.
    /// Blocks if the queue is full until space is available.
    /// @param item The item to be enqueued.
    void enqueue(const T& item) {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [this] { return q.size() < maxSize; });
        q.push(item);
        cv.notify_one();
    }

    /// @brief Removes an item from the queue.
    /// Does not block. Returns `false` if the queue is empty.
    /// @param item Reference to store the dequeued item.
    /// @return `true` if the item was successfully dequeued, `false` otherwise.
    bool dequeue(T& item) {
        unique_lock<mutex> lock(mtx);
        if (q.empty()) return false;
        item = q.front();
        q.pop();
        cv.notify_one();
        return true;
    }

    /// @brief Waits until an item is available and dequeues it.
    /// Blocks until an item becomes available if the queue is empty.
    /// @param item Reference to store the dequeued item.
    void wait_dequeue(T& item) {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [this] { return !q.empty(); });
        item = q.front();
        q.pop();
        cv.notify_one();
    }

    /// @brief Checks if the queue is empty.
    /// @return `true` if the queue is empty, `false` otherwise.
    bool empty() const {
        lock_guard<mutex> lock(mtx);
        return q.empty();
    }

    /// @brief Checks if the queue is full.
    /// @return `true` if the queue is full, `false` otherwise.
    bool full() const {
        lock_guard<mutex> lock(mtx);
        return q.size() >= maxSize;
    }

    /// @brief Clears all items from the queue.
    void clear() {
        lock_guard<mutex> lock(mtx);
        while (!q.empty()) q.pop();
    }

    /// @brief Gets the current size of the queue.
    /// @return The number of items in the queue.
    size_t size() const {
        lock_guard<mutex> lock(mtx);
        return q.size();
    }
};

/// @brief This class has the main video player context.
class VideoState {
public:
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

    double audio_clock; // PTS from audio
    double video_clock; // PTS from video

		/// @brief Constructor to initialize queues with the max size.
    VideoState() : video_queue(2000), audio_queue(2000) {}

    /// @brief Disable video state copy.
    VideoState(const VideoState&) = delete;
    VideoState& operator=(const VideoState&) = delete;
};

/// @brief Open the source and setup the video player.
/// @param state 
/// @param filename 
/// @return `true` if video is ready to play or `false` otherwise.
bool video_reader_open(VideoState* state, const char* filename);

/// @brief Closes all state context fields that couldn't be automatically released.
/// @param state 
void video_reader_close(VideoState* state);

/// @brief Receive a video packet and decode it into a video frame. Then, relevant data from the frame is set into a `VideoFrame` referenced copy.
/// @param state 
/// @param packet 
/// @param vf 
void decode_video_packet(VideoState* state, AVPacket* packet, VideoFrame& vf);

/// @brief Receives an audio packet to decode and resample it into `AudioData` referenced copy.
/// @param state 
/// @param packet 
/// @param ad 
void decode_audio_packet(VideoState* state, AVPacket* packet, AudioData& ad);

/// @brief Refresh timer from video.
/// @param userdata 
/// @param timerID 
/// @param interval 
/// @return 
unsigned int video_refresh_timer(void* userdata, SDL_TimerID timerID, Uint32 interval);

/// @brief Receives raw video frame to process and then render it on the screen.
/// @param state 
/// @param vf 
void render_video_frame(VideoState* state, const VideoFrame& vf);

#endif