#pragma once

#include <string>
#include <vector>

struct PaStreamCallbackTimeInfo;

namespace audioplus {

template<class T>
uint32_t get_audio_dtype();

struct AudioDevice
{
    int const index;
    char const* name() const;
    int max_input_channels() const;
    int max_output_channels() const;
    double default_sample_rate() const;
};

struct AudioSystem
{
    struct Iter
    {
        int index;
        AudioDevice operator*() { return AudioDevice{index}; }
        Iter & operator++() { index++; return *this; }
        bool operator!=(Iter const& o) { return index!=o.index;}
    };

    AudioSystem();
    ~AudioSystem();
    void restart();

    Iter begin();
    Iter end();

    AudioDevice default_input();
    AudioDevice default_output();
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
        void on_audio(Obj * obj, int(Obj::*fn)(T1 const*, T2*, int, Status))
        {
            on_audio_ctx = obj;
            on_audio_fn = (void *) +[] (
                void const* i, void * o,
                unsigned long n,
                PaStreamCallbackTimeInfo const* time,
                unsigned long pa_status,
                void * pa_ctx)
            {
                Status status; // TODO
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
    AudioStream(AudioDevice device, Config cfg);
    AudioStream(AudioStream const&) = delete;
    AudioStream(AudioStream &&);
    AudioStream & operator=(AudioStream &&);
    ~AudioStream();

    bool running() const;

    void start();
    void stop();
    void abort();
    void close();
};


} // namespace audioplus