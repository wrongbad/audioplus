#include "audioplus/wav.h"

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

namespace audioplus {

static size_t read_callback(void * ctx, void * buf, size_t count)
{
    std::istream * stream = (std::istream *)ctx;
    stream->read((char *)buf, count);
    return stream->gcount();
}

static uint32_t iseek_callback(void * ctx, int offset, drwav_seek_origin drway)
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

static uint32_t oseek_callback(void * ctx, int offset, drwav_seek_origin drway)
{
    std::ostream * stream = (std::ostream *)ctx;
    std::ios_base::seekdir way = 
        (drway == drwav_seek_origin_start) ?
        std::ios_base::beg : std::ios_base::cur;
    stream->seekp(offset, way);
    return stream->fail() ? 0 : 1;
}


struct WavStreamBase::Impl
{
    drwav m_wav;
    WavHeader m_header;

    Impl(void * ctx) // read mode
    {
        if(drwav_init_ex(
            &m_wav, 
            read_callback, 
            iseek_callback, 
            nullptr, // onChunk
            ctx,
            nullptr, // onChunk ctx
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

    Impl(void * ctx, WavHeader header) // write mode
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
                return;
        }

        if(drwav_init_write(
                &m_wav, 
                &format,
                write_callback,
                oseek_callback,
                ctx,
                nullptr // allocator
        ))
        {
            m_header = header;
        }
    }
    bool valid() const { return m_header.channels != 0; }
    ~Impl() { drwav_uninit(&m_wav); }
};

WavStreamBase::WavStreamBase()
{   
}
WavStreamBase::~WavStreamBase()
{
}

static bool prep_read(WavStreamBase * w, std::istream & stream)
{
    if(!w->m_impl)
    {
        w->m_impl.reset(new WavStreamBase::Impl((void *)&stream));
    }
    if(!w->m_impl->valid())
    { 
        w->error_message = "could not read wav header";
        return false;
    }
    return true;
}

int WavStreamBase::read(std::istream & stream, WavHeader * header)
{
    if(!prep_read(this, stream)) { return -1; }
    *header = m_impl->m_header;
    return 0;
}

int WavStreamBase::read(std::istream & stream, float * samples, int count)
{
    if(!prep_read(this, stream)) { return -1; }
    return drwav_read_pcm_frames_f32(&m_impl->m_wav, count, samples);
}

int WavStreamBase::read(std::istream & stream, int32_t * samples, int count)
{
    if(!prep_read(this, stream)) { return -1; }
    return drwav_read_pcm_frames_s32(&m_impl->m_wav, count, samples);
}

int WavStreamBase::read(std::istream & stream, int16_t * samples, int count)
{
    if(!prep_read(this, stream)) { return -1; }
    return drwav_read_pcm_frames_s16(&m_impl->m_wav, count, samples);
}

int WavStreamBase::write(std::ostream & stream, WavHeader const* header)
{
    if(m_impl)
    {
        error_message = "wav header already set";
        return -1;
    }
    m_impl.reset(new Impl((void *)&stream, *header));
    if(!m_impl->valid())
    {
        error_message = "wav header write failed";
        return -1;
    }
    return 0;
}

static bool check_write(WavStreamBase * w, WavHeader::DType dtype)
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

int WavStreamBase::write(std::ostream & stream, float const* samples, int count)
{
    if(!check_write(this, WavHeader::Float32)) { return -1; }
    int frames = drwav_write_pcm_frames(&m_impl->m_wav,
        count / m_impl->m_wav.channels, samples);
    return frames * m_impl->m_wav.channels;
}

int WavStreamBase::write(std::ostream & stream, int32_t const* samples, int count)
{
    if(!check_write(this, WavHeader::Int32)) { return -1; }
    int frames = drwav_write_pcm_frames(&m_impl->m_wav,
        count / m_impl->m_wav.channels, samples);
    return frames * m_impl->m_wav.channels;
}

int WavStreamBase::write(std::ostream & stream, int16_t const* samples, int count)
{
    if(!check_write(this, WavHeader::Int16)) { return -1; }
    int frames = drwav_write_pcm_frames(&m_impl->m_wav,
        count / m_impl->m_wav.channels, samples);
    return frames * m_impl->m_wav.channels;
}

void WavStreamBase::finish()
{
    // triggers dr_wav write completion
    // should be called before allowing the ostream to be destroyed
    m_impl.reset();
}

} // namespace audioplus

