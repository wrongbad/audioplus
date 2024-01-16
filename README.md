# audioplus

modern c++ api for audio io and file io

this repo is a thin wrapper for:
- [dr_libs](https://github.com/mackron/dr_libs)
- [portaudio](https://github.com/PortAudio/portaudio)
- [portmidi](https://github.com/PortMidi/portmidi)

a very simple cmake file is included for easy integration  
it may aslo serve as a reference impl for other build systems

the cmake file will check if the dependencies are pre-configured  
and if not, it will download them from github automatically

this library is split into 3 mini libraries  
so you don't need the dependencies you don't use

---

# read wav

```cpp
#include "audioplus/wav.h"
#include <fstream>

// construct an istream wrapper
auto wav = audioplus::make_wav_stream(
    std::ifstream("sick_beats.wav", std::ios::binary));

// read metadata (optional)
audioplus::WavHeader header;
wav >> header;

// read interleaved samples (may shrink, but won't grow)
std::vector<float> samples(header.frames * header.channels);
wav >> samples;

// or stream interleaved samples
std::vector<float> samples(4096);
while(wav >> samples)
{
    // process samples
}

// or read raw array
int count = wav.read(samples.data(), samples.size());
```

# write wav

```cpp
// ostream wrapper the same way
auto wav = audioplus::make_wav_stream(
    std::ofstream("sick_beats.wav", std::ios::binary));

// writing header first is required
wav << audioplus::WavHeader {
    sample_rate,
    channels,
    frames,
    audioplus::WavHeader::Int16
};

// write samples (chunk stream friendly)
std::vector<int16_t> samples = ...
wav << samples;

// write raw array
int count = wav.write(samples.data(), samples.size());
```

# process live audio and midi

```cpp
#include "audioplus/audio.h"
#include "audioplus/midi.h"

using namespace audioplus;

struct MyAudio
{
    AudioSession audio_session;
    AudioStream audio_stream;
    
    MidiSession midi_session;
    MidiInputStream midi_stream;

    MyAudio()
    {
        AudioStream::Config audio_config;
        // configures data format based on this->on_audio() signature
        audio_config.on_audio(this);
        audio_config.input_channels = 1;
        audio_config.output_channels = 2;

        audio_stream = audio_session.open(audio_config);
        audio_stream.start();

        // unset values in config are populated by stream.open()
        std::cout << "sr=" << audio_config.sample_rate << std::endl;

        MidiInputStream::Config midi_config;
        // default device + settings
        midi_stream = midi_session.open(midi_config);
    }

    int on_audio(float const* input, int16_t * output, int frames)
    {
        MidiMsg msg[16];
        int count = midi_stream.read(msg, 16);
        // handle messages, process audio
        return 0; // no error
    }
};
```