#include "audioplus/midi.h"

#include "portmidi.h"

#include <stdexcept>


void throw_pm_error(int err)
{
    if(err >= 0) { return; }
    throw std::runtime_error(Pm_GetErrorText((PmError)err));
}


namespace audioplus {


char const* MidiDevice::name() const
{
    return Pm_GetDeviceInfo(index)->name;
}
bool MidiDevice::has_input() const
{
    return Pm_GetDeviceInfo(index)->input;
}
bool MidiDevice::has_output() const
{
    return Pm_GetDeviceInfo(index)->output;
}


MidiSystem::MidiSystem()
{
    throw_pm_error( Pm_Initialize() );
}
MidiSystem::~MidiSystem()
{
    Pm_Terminate(); // errors in destructors are tricky
}
void MidiSystem::restart()
{
    throw_pm_error( Pm_Terminate() );
    throw_pm_error( Pm_Initialize() );
}
MidiSystem::Iter MidiSystem::begin()
{
    return {0};
}
MidiSystem::Iter MidiSystem::end()
{
    return {Pm_CountDevices()};
}
MidiDevice MidiSystem::default_input()
{
    return {Pm_GetDefaultInputDeviceID()};
}
MidiDevice MidiSystem::default_output()
{
    return {Pm_GetDefaultOutputDeviceID()};
}


MidiInputStream::MidiInputStream(MidiDevice device, int buffer_size)
{
    throw_pm_error( Pm_OpenInput(
        &backend,
        device.index,
        nullptr, // SysDepInfo
        buffer_size,
        nullptr, // TimeFunction
        nullptr // TimeContext
    ) );
}
MidiInputStream::MidiInputStream(MidiInputStream && o)
{
    std::swap(backend, o.backend);
}
MidiInputStream & MidiInputStream::operator=(MidiInputStream && o)
{
    std::swap(backend, o.backend);
    return *this;
}

MidiInputStream::~MidiInputStream()
{
    if(backend) { Pm_Close(backend); }
}

void MidiInputStream::close()
{
    if(!backend) { return; }
    throw_pm_error( Pm_Close(backend) );
}

int MidiInputStream::read(MidiMsg * buf, int buf_size)
{
    static_assert(
        sizeof(PmEvent) == sizeof(MidiMsg), 
        "message struct mismatch");

    PmEvent * pbuf = (PmEvent *)buf;
    int count = Pm_Read(backend, pbuf, buf_size);
    throw_pm_error(count);

    // repack struct as a portability precaution.
    // is the compiler smart enough to no-op this?
    for(int i=0 ; i<count ; i++)
    {
        PmEvent pmsg = pbuf[i];
        buf[i].data[0] = Pm_MessageStatus(pmsg.message);
        buf[i].data[1] = Pm_MessageData1(pmsg.message);
        buf[i].data[2] = Pm_MessageData2(pmsg.message);
        buf[i].timestamp = pmsg.timestamp;
    }

    return count;
}




} // namespace audioplus