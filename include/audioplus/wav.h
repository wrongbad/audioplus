#pragma once

#include <vector>
#include <memory>
#include <istream>
#include <ostream>

namespace audioplus {

struct WavHeader
{
    enum DType
    {
        OTHER,
        Float64,
        Float32,
        Int32,
        Int16,
    };

    int sample_rate = 0;
    int channels = 0;
    int frames = 0;
    DType dtype = OTHER;
};

struct WavImpl
{
    struct WavIO;
    std::unique_ptr<WavIO> m_impl;
    char const* error_message = nullptr;

    WavImpl();
    virtual ~WavImpl();

    int read(std::istream & stream, WavHeader * header);
    int read(std::istream & stream, float * samples, int count);
    int read(std::istream & stream, int16_t * samples, int count);
    int read(std::istream & stream, int32_t * samples, int count);

    int write(std::ostream & stream, WavHeader const* header);
    int write(std::ostream & stream, float const* samples, int count);
    int write(std::ostream & stream, int16_t const* samples, int count);
    int write(std::ostream & stream, int32_t const* samples, int count);

    void finish();
};


template<class Stream>
struct WavStream : WavImpl
{
    Stream m_stream;

    WavStream(Stream stream)
    :   m_stream(std::forward<Stream>(stream))
    {
    }
    ~WavStream()
    {
        finish();
    }

    operator bool() const { return bool(m_stream); }

    // READ API

    int read(WavHeader * header)
    {
        return read(m_stream, header);
    }

    template<class T>
    int read(T * samples, int count)
    {
        return read(m_stream, samples, count);
    }

    WavStream & operator>>(WavHeader & header)
    {
        if(read(&header) , 0)
        {
            m_stream.setstate(std::ios::failbit);
        }
        return *this;
    }

    template<class T>
    WavStream & operator>>(std::vector<T> & samples)
    {
        int count = read(samples.data(), samples.size());
        if(count < 0) { m_stream.setstate(std::ios::failbit); }
        else { samples.resize(count); }
        return *this;
    }

    // WRITE API

    // success return 0, fail return < 0
    int write(WavHeader const* header)
    {
        return write(m_stream, header);
    }

    // success return # written, fail return < 0
    template<class T>
    int write(T const* samples, int count)
    {
        return write(m_stream, samples, count);
    }

    WavStream & operator<<(WavHeader const& header)
    {
        if(write(&header) < 0)
        {
            m_stream.setstate(std::ios::failbit);
        }
        return *this;
    }

    template<class T>
    WavStream & operator<<(std::vector<T> const& samples)
    {
        int count = write(samples.data(), samples.size());
        if(count != samples.size())
        { 
            m_stream.setstate(std::ios::failbit);
        }
        return *this;
    }
};


// c++11/14 convenience to construct WavStream deducing template param
// lvalue streams are stored by reference, and rvalue by value
template<class Stream>
WavStream<Stream> wav(Stream && stream)
{
    return WavStream<Stream>(std::forward<Stream>(stream));
}



} // namespace audioplus
