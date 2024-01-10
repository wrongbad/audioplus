#pragma once

#include <string>

namespace audioplus {

struct MidiDevice
{
    int index;
    std::string name() const;
    bool has_input() const;
    bool has_output() const;
};

struct MidiMsg
{
    uint8_t data[4];
    int32_t timestamp;
};

struct MidiInputStream
{
    void * backend = nullptr;

    struct Config
    {
        MidiDevice device = {-1}; // -1 = default
        int buffer_size = 512;
        void * clock_time_fn = nullptr;
        void * clock_time_ctx = nullptr;

        template<class Obj>
        void clock_time(Obj * obj)
        {
            clock_time_fn = (void *) +[] (void * ctx) {
                return ((Obj *)ctx)->clock_time();
            };
            clock_time_ctx = (void *)obj;
        }
    };

    MidiInputStream() {}
    MidiInputStream(MidiInputStream const&) = delete;
    MidiInputStream(MidiInputStream && o);
    MidiInputStream & operator=(MidiInputStream && o);
    ~MidiInputStream() { close(std::nothrow_t{}); }

    bool is_open() const;
    void close();
    int close(std::nothrow_t); // return < 0 if error

    int read(MidiMsg * buf, int buf_size);
};

struct MidiSession
{
    struct Iter
    {
        int index;
        MidiDevice operator*() { return MidiDevice{index}; }
        Iter & operator++() { index++; return *this; }
        bool operator!=(Iter const& o) { return index!=o.index;}
        int operator-(Iter const& o) { return index-o.index; }
    };

    MidiSession();
    ~MidiSession();
    void restart();

    // cfg modified in place
    MidiInputStream open(MidiInputStream::Config & cfg);

    Iter begin();
    Iter end();

    MidiDevice default_input();
    MidiDevice default_output();
};

} // namespace audioplus