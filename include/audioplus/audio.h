#pragma once

#include <string>

struct PaStreamCallbackTimeInfo;

namespace audioplus {

template<class T>
uint32_t get_audio_dtype();

struct AudioDevice
{
    int index;
    std::string name() const;
    int max_input_channels() const;
    int max_output_channels() const;
    double default_sample_rate() const;
};

struct AudioStream
{
    struct Status
    {
        double input_hw_time = 0;
        double callback_time = 0;
        double output_hw_time = 0;
        bool input_underflow = 0;
        bool input_overflow = 0;
        bool output_underflow = 0;
        bool output_overflow = 0;
        bool priming_output = 0;
        Status() {}
        Status(PaStreamCallbackTimeInfo const* pa_time, unsigned long pa_flags);
    };

    struct Config
    {
        void * on_audio_fn = nullptr;
        void * on_audio_ctx = nullptr;
        void * on_finish_fn = nullptr;
        void * on_finish_ctx = nullptr;
        uint32_t input_dtype = 0;
        uint32_t output_dtype = 0;
        int input_channels = 0;
        int output_channels = 0;
        AudioDevice input_device = {-1}; // -1 = default
        AudioDevice output_device = {-1}; // -1 = default
        double sample_rate = 0;
        int chunk_frames = 0;
        bool clip = true;
        bool dither = true;
        bool never_drop_input = false;
        bool prime_output_with_callback = false;

        template<class Obj>
        void on_audio(Obj * obj) { on_audio(obj, &Obj::on_audio); }

        template<class Obj>
        void on_finish(Obj * obj)
        { 
            on_finish_ctx = obj;
            on_finish_fn = (void *) +[] (void * pa_ctx) {
                ((Obj*)pa_ctx)->on_finish();
            };
        }

      private:
        template<class Obj, class T1, class T2>
        void on_audio(Obj * obj, int(Obj::*fn)(T1 const*, T2*, int, Status const&))
        {
            on_audio_ctx = obj;
            on_audio_fn = (void *) +[] (
                void const* i, void * o,
                unsigned long n,
                PaStreamCallbackTimeInfo const* pa_time,
                unsigned long pa_flags,
                void * pa_ctx)
            {
                Status status(pa_time, pa_flags);
                return ((Obj *)pa_ctx)->on_audio((T1 const*)i, (T2 *)o, n, status);
            };
            input_dtype = get_audio_dtype<T1>();
            output_dtype = get_audio_dtype<T2>();
        }
        template<class Obj, class T1, class T2>
        void on_audio(Obj * obj, int(Obj::*fn)(T1 const*, T2*, int))
        {
            on_audio_ctx = obj;
            on_audio_fn = (void *) +[] (
                void const* i, void * o,
                unsigned long n,
                PaStreamCallbackTimeInfo const* time,
                unsigned long pa_status,
                void * pa_ctx)
            {
                return ((Obj *)pa_ctx)->on_audio((T1 const*)i, (T2 *)o, n);
            };
            input_dtype = get_audio_dtype<T1>();
            output_dtype = get_audio_dtype<T2>();
        }
    };

    void * backend = nullptr;

    AudioStream() {}
    AudioStream(AudioStream const&) = delete;
    AudioStream(AudioStream &&);
    AudioStream & operator=(AudioStream &&);
    ~AudioStream() { close(std::nothrow_t{}); }

    bool is_open() const;
    bool running() const;
    void start();
    void stop();
    void abort();
    void close();
    int close(std::nothrow_t); // return < 0 if error
    double clock_time();
};

struct AudioSession
{
    struct Iter
    {
        int index;
        AudioDevice operator*() { return AudioDevice{index}; }
        Iter & operator++() { index++; return *this; }
        bool operator!=(Iter const& o) { return index!=o.index;}
        int operator-(Iter const& o) { return index-o.index; }
    };

    AudioSession();
    ~AudioSession();
    void restart();
    
    // cfg modified in place
    AudioStream open(AudioStream::Config & cfg);

    Iter begin();
    Iter end();

    AudioDevice default_input();
    AudioDevice default_output();
};

} // namespace audioplus