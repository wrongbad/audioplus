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


template<class Obj>
class AudioTraits
{

};




// template<class Obj>
// decltype(declval<Obj>().on_audio())
// audio_cb(
//     void const* inp, void * outp,
//     unsigned long frames,
//     PaStreamCallbackTimeInfo const* time,
//     unsigned long status,
//     void * vctx)
// {
//     auto * ctx = (AudioStream::Context *)vctx;
//     // TODO time/status
//     return ctx->on_audio(inp, outp, frames);
// }



// template<class Obj, >


struct AudioStream
{
    // using AudioFunc = std::function<int(void const*, void*, int)>;
    // using FinishFunc = std::function<void()>;

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
    // struct Context
    // {
    //     void * backend = nullptr;
    //     AudioFunc on_audio;
    //     FinishFunc on_finish;
    //     RealtimeStatus realtime_status;

    //     ~Context();
    // };

    void * backend = nullptr;

    // template<class Obj, class T1, class T2>
    // AudioStream(StreamParams params, Obj * ctx, 
    //     int(Obj::*fn)(T1 const*, T2*, int, Status))


    // template<class Obj, class T1, class T2>
    // AudioStream(StreamParams params, Obj * ctx, 
    //     int(Obj::*fn)(T1 const*, T2*, int, Status))
    // {
    //     (void const* inp, void * outp,
    //         unsigned long frames,
    //         PaStreamCallbackTimeInfo const* time,
    //         unsigned long status,
    //         void * vctx)
    // }

    // template<class Obj, class T1, class T2>
    // AudioStream(StreamParams params, Obj * ctx, int(Obj::*fn)(T1 const*, T2*, int))
    // {
        
    // }

    // template<class Obj>
    // AudioStream(StreamParams params, Obj * ctx)
    // :   AudioStream(params, ctx, &Obj::on_audio)
    // {
    // }

    AudioStream() {}
    AudioStream(AudioDevice device, Config cfg);
    AudioStream(AudioStream const&) = delete;
    AudioStream(AudioStream &&);
    AudioStream & operator=(AudioStream &&);
    ~AudioStream();

    bool running() const;

    // void connect(AudioDevice device);
    void start();
    void stop();
    void abort();
    void close();

    // // for use only inside audio callback
    // RealtimeStatus const& realtime_status() const 
    // {
    //     return context->realtime_status; 
    // }






    // AudioStream & on_finish(FinishFunc fn)
    // {
    //     context->on_finish = std::move(fn);
    //     return *this;
    // }
    // AudioStream & on_audio(AudioFunc fn)
    // {
    //     context->on_audio = std::move(fn);
    //     return *this;
    // }

    // template<class Obj, class T1, class T2>
    // AudioStream & on_audio(Obj * obj, int(Obj::*fn)(T1 const*, T2*, int))
    // {
    //     context->on_audio = [=] (void const* i, void * o, int n) {
    //         return (obj->*fn)((T1 const*)i, (T2 *)o, n);
    //     };
    //     input_dtype = get_audio_dtype<T1>();
    //     output_dtype = get_audio_dtype<T2>();
    //     return *this;
    // }
    // uint32_t input_dtype = 0;
    // uint32_t output_dtype = 0;


// private:
    // AudioStream can be copied / moved
    // but Context is referenced by user-data pointer
    // in the c API for for callbacks
    // std::unique_ptr<Context> context {new Context()};
};


} // namespace audioplus