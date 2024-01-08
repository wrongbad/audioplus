#include "audioplus/wav.h"

#define DR_WAV_IMPLEMENTATION
#include "dr_libs/dr_wav.h"

namespace audioplus {

static size_t read_callback(void * ctx, void * buf, size_t count)
{
    std::istream * stream = (std::istream *)ctx;
    stream->read((char *)buf, count);
    return stream->gcount();
}

static uint32_t seek_callback(void * ctx, int offset, drwav_seek_origin drway)
{
    std::istream * stream = (std::istream *)ctx;
    std::ios_base::seekdir way = 
        (drway == drwav_seek_origin_start) ?
        std::ios_base::beg : std::ios_base::cur;
    stream->seekg(offset, way);
    return stream->fail() ? 0 : 1;
}

static size_t write_callback(void * ctx, void const* buf, size_t count)
{
    std::ostream * stream = (std::ostream *)ctx;
    stream->write((char *)buf, count);
    return stream->fail() ? 0 : count;
};


struct WavImpl::WavIO
{
    drwav m_wav;
    WavHeader m_header;

    WavIO(void * ctx) // read mode
    {
        if(drwav_init_ex(
            &m_wav, 
            read_callback, 
            seek_callback, 
            nullptr, // onChunk
            ctx,
            nullptr, // chunk_ctx
            DRWAV_SEQUENTIAL, 
            nullptr // allocator
        )) 
        { 
            m_header.sample_rate = m_wav.sampleRate;
            m_header.channels = m_wav.channels;
            m_header.frames = m_wav.totalPCMFrameCount;
            int format = (m_wav.translatedFormatTag << 16) + m_wav.bitsPerSample;
            constexpr int type_int = DR_WAVE_FORMAT_PCM << 16;
            constexpr int type_float = DR_WAVE_FORMAT_IEEE_FLOAT << 16;
            switch(format)
            {
                case type_float+64: m_header.dtype = WavHeader::Float64; break;
                case type_float+32: m_header.dtype = WavHeader::Float32; break;
                case type_int+32: m_header.dtype = WavHeader::Int32; break;
                case type_int+16: m_header.dtype = WavHeader::Int16; break;
                default: m_header.dtype = WavHeader::OTHER;
            }
        }
    }

    WavIO(void * ctx, WavHeader header) // write mode
    {
        m_header = header;

        drwav_data_format format;
        format.container = drwav_container_riff;
        format.channels = header.channels;
        format.sampleRate = header.sample_rate;
        format.bitsPerSample = 0;

        switch(header.dtype)
        {
            case WavHeader::Float64:
                format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
                format.bitsPerSample = 64;
                break;
            case WavHeader::Float32:
                format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
                format.bitsPerSample = 32;
                break;
            case WavHeader::Int32:
                format.format = DR_WAVE_FORMAT_PCM;
                format.bitsPerSample = 32;
                break;
            case WavHeader::Int16:
                format.format = DR_WAVE_FORMAT_PCM;
                format.bitsPerSample = 16;
                break;
            default:
                break;
        }

        if( format.bitsPerSample != 0
            && 
            drwav_init_write_sequential(
            &m_wav, 
            &format,
            header.frames * header.channels,
            write_callback,
            ctx,
            nullptr
        ))
        {
            m_header = header;
        }
    }
    bool valid() const { return m_header.channels != 0; }
    ~WavIO() { drwav_uninit(&m_wav); }
};

WavImpl::WavImpl()
{   
}
WavImpl::~WavImpl()
{
}

static bool prep_read(WavImpl * w, std::istream & stream)
{
    if(!w->m_impl)
    {
        w->m_impl.reset(new WavImpl::WavIO((void *)&stream));
    }
    if(!w->m_impl->valid())
    { 
        w->error_message = "could not read wav header";
        return false;
    }
    return true;
}

int WavImpl::read(std::istream & stream, WavHeader * header)
{
    if(!prep_read(this, stream)) { return -1; }
    *header = m_impl->m_header;
    return 0;
}

int WavImpl::read(std::istream & stream, float * samples, int count)
{
    if(!prep_read(this, stream)) { return -1; }
    return drwav_read_pcm_frames_f32(&m_impl->m_wav, count, samples);
}

int WavImpl::read(std::istream & stream, int32_t * samples, int count)
{
    if(!prep_read(this, stream)) { return -1; }
    return drwav_read_pcm_frames_s32(&m_impl->m_wav, count, samples);
}

int WavImpl::read(std::istream & stream, int16_t * samples, int count)
{
    if(!prep_read(this, stream)) { return -1; }
    return drwav_read_pcm_frames_s16(&m_impl->m_wav, count, samples);
}

int WavImpl::write(std::ostream & stream, WavHeader const* header)
{
    if(m_impl)
    {
        error_message = "wav header already set";
        return -1;
    }
    m_impl.reset(new WavIO((void *)&stream, *header));
    if(!m_impl->valid())
    {
        error_message = "wav header write failed";
        return -1;
    }
    return 0;
}

static bool check_write(WavImpl * w, WavHeader::DType dtype)
{
    if(!w->m_impl || !w->m_impl->valid())
    { 
        w->error_message = "wav header not written yet";
        return false;
    }
    if(w->m_impl->m_header.dtype != dtype)
    {
        w->error_message = "wav write data type mismatch";
        return false;
    }
    return true;
}

int WavImpl::write(std::ostream & stream, float const* samples, int count)
{
    if(!check_write(this, WavHeader::Float32)) { return -1; }
    int frames = drwav_write_pcm_frames(&m_impl->m_wav,
        count / m_impl->m_wav.channels, samples);
    return frames * m_impl->m_wav.channels;
}

int WavImpl::write(std::ostream & stream, int32_t const* samples, int count)
{
    if(!check_write(this, WavHeader::Int32)) { return -1; }
    int frames = drwav_write_pcm_frames(&m_impl->m_wav,
        count / m_impl->m_wav.channels, samples);
    return frames * m_impl->m_wav.channels;
}

int WavImpl::write(std::ostream & stream, int16_t const* samples, int count)
{
    if(!check_write(this, WavHeader::Int16)) { return -1; }
    int frames = drwav_write_pcm_frames(&m_impl->m_wav,
        count / m_impl->m_wav.channels, samples);
    return frames * m_impl->m_wav.channels;
}

void WavImpl::finish()
{
    // triggers dr_wav write completion
    // should be called before allowing the ostream to be destroyed
    m_impl.reset();
}

} // namespace audioplus




// struct wav
// {
//     std::vector<float> interleaved_samples;
//     int sample_rate = 0;
//     int channels = 0;
// };

// template<class Stream>
// Stream & operator>>(Stream & in, wav & audiodata)
// {
//     auto read = +[] (void * ctx, void * buf, size_t count) {
//         Stream & in = *(Stream*)ctx;
//         in.read((char*)buf, count);
//         return (size_t)in.gcount();
//     };
//     auto seek = +[] (void * ctx, int off, drwav_seek_origin drway) {
//         Stream & in = *(Stream*)ctx;
//         std::ios_base::seekdir way = (drway == drwav_seek_origin_start) ?
//             std::ios_base::beg : std::ios_base::cur;
//         in.seekg(off, way);
//         return (uint32_t)(bool)in;
//     };
//     drwav reader;
//     if(!drwav_init_ex(&reader, read, seek, nullptr, (void*)&in,
//         nullptr, DRWAV_SEQUENTIAL, nullptr))
//     {   in.setstate(std::ios::failbit);
//         return in;
//     }
//     audiodata.frameSize = reader.channels;
//     audiodata.frameRate = reader.sampleRate;
//     size_t frames = reader.totalPCMFrameCount;
//     audiodata.samples.resize(frames * audiodata.frameSize);

//     frames = drwav_read_pcm_frames_f32(&reader, frames, audiodata.samples.data());
//     audiodata.samples.resize(frames * audiodata.frameSize); // handle truncated files

//     drwav_uninit(&reader);

//     return in;
// }

// template<class Stream>
// Stream & operator<<(Stream & out, wav const& audiodata)
// {
//     auto write = +[] (void * ctx, void const* buf, size_t count) {
//         Stream & out = *(Stream*)ctx;
//         out.write((char*)buf, count);
//         return bool(out) ? count : size_t(0); // approx
//     };
//     drwav_data_format format;
//     format.container = drwav_container_riff;
//     format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
//     format.channels = audiodata.frameSize;
//     format.sampleRate = audiodata.frameRate;
//     format.bitsPerSample = 32;
//     drwav writer;
//     if(!drwav_init_write_sequential(&writer, &format,
//         audiodata.samples.size(), write, (void*)&out, nullptr))
//     {   out.setstate(std::ios::failbit);
//         return out;
//     }
//     drwav_write_pcm_frames(&writer,
//         audiodata.samples.size()/audiodata.frameSize,
//         audiodata.samples.data());
//     drwav_uninit(&writer);
//     return out;
// }
