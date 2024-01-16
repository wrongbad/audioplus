#include "audioplus/audio.h"

#include "portaudio.h"
#include <stdexcept>
#include <iostream>

namespace {

void throw_pa_error(int err)
{
    if(!err) { return; }
    std::cerr << "err = " << Pa_GetErrorText(err) << std::endl;
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


static PaDeviceInfo const* get_info(int index)
{
    PaDeviceInfo const* info = Pa_GetDeviceInfo(index);
    if(!info) { throw std::runtime_error("device out of range"); }
    return info;
}

std::string AudioDevice::name() const
{
    return get_info(index)->name;
}
int AudioDevice::max_input_channels() const
{
    return get_info(index)->maxInputChannels;
}
int AudioDevice::max_output_channels() const
{
    return get_info(index)->maxOutputChannels;
}
double AudioDevice::default_sample_rate() const
{
    return get_info(index)->defaultSampleRate;
}


AudioSession::AudioSession()
{
    throw_pa_error( Pa_Initialize() );
}
AudioSession::~AudioSession()
{
    Pa_Terminate(); // errors in destructors are tricky
}
void AudioSession::restart()
{
    throw_pa_error( Pa_Terminate() );
    throw_pa_error( Pa_Initialize() );
}
AudioSession::Iter AudioSession::begin()
{
    return {0};
}
AudioSession::Iter AudioSession::end()
{
    return {Pa_GetDeviceCount()};
}
AudioDevice AudioSession::default_input()
{
    return {Pa_GetDefaultInputDevice()};
}
AudioDevice AudioSession::default_output()
{
    return {Pa_GetDefaultOutputDevice()};
}

AudioStream AudioSession::open(AudioStream::Config & cfg)
{
    AudioStream out;

    if(cfg.input_channels == 0 && cfg.output_channels == 0)
        throw std::runtime_error("AudioStream with no channels");

    if(cfg.input_device.index < 0)
        cfg.input_device.index = Pa_GetDefaultInputDevice();

    if(cfg.output_device.index < 0)
        cfg.output_device.index = Pa_GetDefaultOutputDevice();

    if(cfg.sample_rate == 0 && cfg.input_channels > 0)
        cfg.sample_rate = cfg.input_device.default_sample_rate();
    else if(cfg.sample_rate == 0)
        cfg.sample_rate = cfg.output_device.default_sample_rate();

    if(cfg.chunk_frames == 0)
        cfg.chunk_frames = 512; // TODO device default latency)

    PaStreamParameters in_params;
    in_params.device = cfg.input_device.index;
    in_params.channelCount = cfg.input_channels;
    in_params.sampleFormat = cfg.input_dtype;
    in_params.suggestedLatency = 0;
    in_params.hostApiSpecificStreamInfo = nullptr;

    PaStreamParameters out_params;
    out_params.device = cfg.output_device.index;
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
        &out.backend,
        cfg.input_channels ? &in_params : nullptr,
        cfg.output_channels ? &out_params : nullptr,
        cfg.sample_rate,
        cfg.chunk_frames,
        pa_flags,
        (PaStreamCallback *)cfg.on_audio_fn,
        cfg.on_audio_ctx
    ) );

    return out;
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

bool AudioStream::is_open() const
{
    return backend;
}
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
    throw_pa_error( close(std::nothrow_t{}) );
}
int AudioStream::close(std::nothrow_t)
{
    int stat = backend ? Pa_CloseStream(backend) : 0;
    backend = nullptr;
    return stat;
}
double AudioStream::clock_time()
{
    return backend ? Pa_GetStreamTime(backend) : 0;
}

AudioStream::Status::Status(PaStreamCallbackTimeInfo const* pa_time, unsigned long pa_flags)
{
    input_hw_time = pa_time->inputBufferAdcTime;
    callback_time = pa_time->currentTime;
    output_hw_time = pa_time->outputBufferDacTime;
    input_underflow = (pa_flags & paInputUnderflow);
    input_overflow = (pa_flags & paInputOverflow);
    output_underflow = (pa_flags & paOutputUnderflow);
    output_overflow = (pa_flags & paOutputOverflow);
    priming_output = (pa_flags & paPrimingOutput);
}

} // namespace audiopluss