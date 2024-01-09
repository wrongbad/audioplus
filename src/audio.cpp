#include "audioplus/audio.h"

#include "portaudio.h"
#include <stdexcept>

namespace {

void throw_pa_error(int err)
{
    if(!err) { return; }
    throw std::runtime_error(Pa_GetErrorText(err));
}

} // namespace


namespace audioplus {


template<>
uint32_t get_audio_dtype<float>() { return paFloat32; }
template<>
uint32_t get_audio_dtype<int32_t>() { return paInt32; }
template<>
uint32_t get_audio_dtype<int16_t>() { return paInt16; }


char const* AudioDevice::name() const
{
    return Pa_GetDeviceInfo(index)->name;
}
int AudioDevice::max_input_channels() const
{
    return Pa_GetDeviceInfo(index)->maxInputChannels;
}
int AudioDevice::max_output_channels() const
{
    return Pa_GetDeviceInfo(index)->maxOutputChannels;
}
double AudioDevice::default_sample_rate() const
{
    return Pa_GetDeviceInfo(index)->defaultSampleRate;
}


AudioSystem::AudioSystem()
{
    throw_pa_error( Pa_Initialize() );
}
AudioSystem::~AudioSystem()
{
    Pa_Terminate(); // errors in destructors are tricky
}
void AudioSystem::restart()
{
    throw_pa_error( Pa_Terminate() );
    throw_pa_error( Pa_Initialize() );
}
AudioSystem::Iter AudioSystem::begin()
{
    return {0};
}
AudioSystem::Iter AudioSystem::end()
{
    return {Pa_GetDeviceCount()};
}
AudioDevice AudioSystem::default_input()
{
    return {Pa_GetDefaultInputDevice()};
}
AudioDevice AudioSystem::default_output()
{
    return {Pa_GetDefaultOutputDevice()};
}


AudioStream::AudioStream(AudioStream && o)
{
    std::swap(backend, o.backend);
}
AudioStream & AudioStream::operator=(AudioStream && o)
{
    std::swap(backend, o.backend);
    return *this;
}

AudioStream::~AudioStream()
{
    Pa_CloseStream(backend);
}

AudioStream::AudioStream(AudioDevice device, Config cfg)
{
    PaStreamParameters in_params;
    in_params.device = device.index;
    in_params.channelCount = cfg.input_channels;
    in_params.sampleFormat = cfg.input_dtype;
    in_params.suggestedLatency = 0;
    in_params.hostApiSpecificStreamInfo = nullptr;

    PaStreamParameters out_params;
    out_params.device = device.index;
    out_params.channelCount = cfg.output_channels;
    out_params.sampleFormat = cfg.output_dtype;
    out_params.suggestedLatency = 0;
    out_params.hostApiSpecificStreamInfo = nullptr;
    
    PaStreamFlags pa_flags = 0;
    if(!cfg.clip) { pa_flags |= paClipOff; }
    if(!cfg.dither) { pa_flags |= paDitherOff; }
    if(cfg.never_drop_input) { pa_flags |= paNeverDropInput; }
    if(cfg.prime_output_with_callback) { pa_flags |= paPrimeOutputBuffersUsingStreamCallback; }

    throw_pa_error( Pa_OpenStream(
        &backend,
        cfg.input_channels ? &in_params : nullptr,
        cfg.output_channels ? &out_params : nullptr,
        cfg.sample_rate ? cfg.sample_rate : device.default_sample_rate(),
        cfg.chunk_frames ? cfg.chunk_frames : 512, // TODO device default latency
        pa_flags,
        (PaStreamCallback *)cfg.on_audio_fn,
        cfg.on_audio_ctx
    ) );
}



// AudioStream::Context::~Context()
// {
//     // errors in destructors are tricky
//     if(backend) { Pa_CloseStream(backend); }
// }



// static int audio_cb(
//     void const* inp, void * outp,
//     unsigned long frames,
//     PaStreamCallbackTimeInfo const* time,
//     PaStreamCallbackFlags status,
//     void * vctx)
// {
//     auto * ctx = (AudioStream::Context *)vctx;
//     // TODO time/status
//     return ctx->on_audio(inp, outp, frames);
// }

// void AudioStream::connect(AudioDevice const& device)
// {
//     PaStreamParameters in_params;
//     in_params.device = device.index;
//     in_params.channelCount = input_channels;
//     in_params.sampleFormat = input_dtype;
//     in_params.suggestedLatency = 0;
//     in_params.hostApiSpecificStreamInfo = nullptr;

//     PaStreamParameters out_params;
//     out_params.device = device.index;
//     out_params.channelCount = output_channels;
//     out_params.sampleFormat = output_dtype;
//     out_params.suggestedLatency = 0;
//     out_params.hostApiSpecificStreamInfo = nullptr;
    
//     PaStreamFlags flags = 0;
//     if(!clip) { flags |= paClipOff; }
//     if(!dither) { flags |= paDitherOff; }
//     if(never_drop_input) { flags |= paNeverDropInput; }
//     if(prime_output_with_callback) { flags |= paPrimeOutputBuffersUsingStreamCallback; }

//     throw_pa_error( Pa_OpenStream(
//         &context->backend,
//         input_channels ? &in_params : nullptr,
//         output_channels ? &out_params : nullptr,
//         sample_rate ? sample_rate : device.default_sample_rate(),
//         chunk_frames ? chunk_frames : 512, // TODO device default latency
//         flags,
//         audio_cb,
//         (void *)context.get()
//     ) );

//     if(context->on_finish)
//     {
//         // TODO 
//     }
// }
bool AudioStream::running() const
{
    return Pa_IsStreamActive(backend) > 0;
}
void AudioStream::start()
{
    throw_pa_error( Pa_StartStream(backend) );
}
void AudioStream::stop()
{
    throw_pa_error( Pa_StopStream(backend) );
}
void AudioStream::abort()
{
    throw_pa_error( Pa_AbortStream(backend) );
}
void AudioStream::close()
{
    if(!backend) { return; }
    throw_pa_error( Pa_CloseStream(backend) );
}

} // namespace audiopluss