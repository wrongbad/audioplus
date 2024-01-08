#pragma once

#include <string>
#include <vector>

namespace audioplus {

struct AudioDevice
{
    char const* name() const;
    int max_input_channels() const;
    int max_output_channels() const;
    double default_sample_rate() const;

    void const* backend;
    int const index;
};

struct AudioSystem
{
    std::vector<AudioDevice> devices;

    AudioSystem();
    ~AudioSystem();
    void restart();
};

template<class T>
uint32_t get_audio_dtype() { return 0; }
template<>
inline uint32_t get_audio_dtype<float>() { return 1; }
template<>
inline uint32_t get_audio_dtype<int32_t>() { return 2; }
template<>
inline uint32_t get_audio_dtype<int16_t>() { return 8; }


struct AudioStream
{
    using AudioFunc = std::function<int(void const*, void*, int)>;
    using FinishFunc = std::function<void()>;

    struct RealtimeStatus
    {
        double input_hw_time = 0;
        double callback_time = 0;
        double output_hw_time = 0;
        bool input_underflow = 0;
        bool input_overflow = 0;
        bool output_underflow = 0;
        bool output_overflow = 0;
        bool priming_output = 0;
    };
    struct Context
    {
        void * backend = nullptr;
        AudioFunc on_audio;
        FinishFunc on_finish;
        RealtimeStatus realtime_status;

        ~Context();
    };

    bool running() const;

    void connect(AudioDevice const& device);
    void start();
    void stop();
    void abort();
    void disconnect();

    // for use only inside audio callback
    RealtimeStatus const& realtime_status() const { return context->realtime_status; }

    AudioStream & on_finish(FinishFunc fn)
    {
        context->on_finish = std::move(fn);
        return *this;
    }
    AudioStream & on_audio(AudioFunc fn)
    {
        context->on_audio = std::move(fn);
        return *this;
    }

    template<class Obj, class T1, class T2>
    AudioStream & on_audio(Obj * obj, int(Obj::*fn)(T1 const*, T2*, int))
    {
        context->on_audio = [=] (void const* i, void * o, int n) {
            return (obj->*fn)((T1 const*)i, (T2 *)o, n);
        };
        input_dtype = get_audio_dtype<T1>();
        output_dtype = get_audio_dtype<T2>();
        return *this;
    }

    int input_channels = 0;
    uint32_t input_dtype = 0;
    int output_channels = 0;
    uint32_t output_dtype = 0;
    double sample_rate = 0;
    int chunk_frames = 0;
    bool clip = true;
    bool dither = true;
    bool never_drop_input = false;
    bool prime_output_with_callback = false;

private:
    // AudioStream can be copied / moved
    // but Context is referenced by user-data pointer
    // in the c API for for callbacks
    std::unique_ptr<Context> context {new Context()};
};




} // namespace audioplus