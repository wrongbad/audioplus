#pragma once

#include <vector>

namespace audioplus {

struct MidiDevice
{
    int const index;
    char const* name() const;
    bool has_input() const;
    bool has_output() const;
};

struct MidiSystem
{
    struct Iter
    {
        int index;
        MidiDevice operator*() { return MidiDevice{index}; }
        Iter & operator++() { index++; return *this; }
        bool operator!=(Iter const& o) { return index!=o.index;}
    };

    MidiSystem();
    ~MidiSystem();
    void restart();

    Iter begin();
    Iter end();

    MidiDevice default_input();
    MidiDevice default_output();
};

struct MidiMsg
{
    uint8_t data[4];
    int32_t timestamp;
};

struct MidiInputStream
{
    void * backend = nullptr;

    MidiInputStream() {}
    MidiInputStream(MidiDevice device, int buffer_size=512);
    MidiInputStream(MidiInputStream const&) = delete;
    MidiInputStream(MidiInputStream && o);
    MidiInputStream & operator=(MidiInputStream && o);
    ~MidiInputStream();
    void close();

    int read(MidiMsg * buf, int buf_size);

};

} // namespace audioplus