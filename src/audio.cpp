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


char const* AudioDevice::name() const
{
    return ((PaDeviceInfo *)backend)->name;
}
int AudioDevice::max_input_channels() const
{
    return ((PaDeviceInfo *)backend)->maxInputChannels;
}
int AudioDevice::max_output_channels() const
{
    return ((PaDeviceInfo *)backend)->maxOutputChannels;
}
double AudioDevice::default_sample_rate() const
{
    return ((PaDeviceInfo *)backend)->defaultSampleRate;
}


AudioSystem::AudioSystem()
{
    throw_pa_error( Pa_Initialize() );

    for(int i=0 ; i<Pa_GetDeviceCount() ; i++)
    {
        devices.push_back(AudioDevice{(void const*)Pa_GetDeviceInfo(i), i});
    }
}

AudioSystem::~AudioSystem()
{
    // errors in destructors are tricky
    Pa_Terminate();
}

void AudioSystem::restart()
{
    throw_pa_error( Pa_Terminate() );
    throw_pa_error( Pa_Initialize() );

    devices.clear();
    for(int i=0 ; i<Pa_GetDeviceCount() ; i++)
    {
        devices.push_back(AudioDevice{(void const*)Pa_GetDeviceInfo(i), i});
    }
}



AudioStream::Context::~Context()
{
    // errors in destructors are tricky
    if(backend) { Pa_CloseStream(backend); }
}


bool AudioStream::running() const
{
    return Pa_IsStreamActive((PaStream *)context->backend);
}

static int audio_cb(
    void const* inp, void * outp,
    unsigned long frames,
    PaStreamCallbackTimeInfo const* time,
    PaStreamCallbackFlags status,
    void * vctx)
{
    auto * ctx = (AudioStream::Context *)vctx;
    // TODO time/status
    return ctx->on_audio(inp, outp, frames);
}

void AudioStream::connect(AudioDevice const& device)
{
    PaStreamParameters in_params;
    in_params.device = device.index;
    in_params.channelCount = input_channels;
    in_params.sampleFormat = input_dtype;
    in_params.suggestedLatency = 0;
    in_params.hostApiSpecificStreamInfo = nullptr;

    PaStreamParameters out_params;
    out_params.device = device.index;
    out_params.channelCount = output_channels;
    out_params.sampleFormat = output_dtype;
    out_params.suggestedLatency = 0;
    out_params.hostApiSpecificStreamInfo = nullptr;
    
    PaStreamFlags flags = 0;
    if(!clip) { flags |= paClipOff; }
    if(!dither) { flags |= paDitherOff; }
    if(never_drop_input) { flags |= paNeverDropInput; }
    if(prime_output_with_callback) { flags |= paPrimeOutputBuffersUsingStreamCallback; }

    throw_pa_error( Pa_OpenStream(
        &context->backend,
        input_channels ? &in_params : nullptr,
        output_channels ? &out_params : nullptr,
        sample_rate ? sample_rate : device.default_sample_rate(),
        chunk_frames ? chunk_frames : 512, // TODO device default latency
        flags,
        audio_cb,
        (void *)context.get()
    ) );

    if(context->on_finish)
    {
        // TODO 
    }
}
void AudioStream::start()
{
    throw_pa_error( Pa_StartStream((PaStream *)context->backend) );
}
void AudioStream::stop()
{
    throw_pa_error( Pa_StopStream((PaStream *)context->backend) );
}
void AudioStream::abort()
{
    throw_pa_error( Pa_AbortStream((PaStream *)context->backend) );
}
void AudioStream::disconnect()
{
    if(!context->backend) { return; }
    throw_pa_error( Pa_CloseStream((PaStream *)context->backend) );
}

} // namespace audioplus