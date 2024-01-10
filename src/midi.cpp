#include "audioplus/midi.h"

#include "portmidi.h"

#include <stdexcept>


void throw_pm_error(int err)
{
    if(err >= 0) { return; }
    throw std::runtime_error(Pm_GetErrorText((PmError)err));
}


namespace audioplus {


static PmDeviceInfo const* get_info(int index)
{
    PmDeviceInfo const* info = Pm_GetDeviceInfo(index);
    if(!info) { throw std::runtime_error("device out of range"); }
    return info;
}

std::string MidiDevice::name() const
{
    return get_info(index)->name;
}
bool MidiDevice::has_input() const
{
    return get_info(index)->input;
}
bool MidiDevice::has_output() const
{
    return get_info(index)->output;
}


MidiSession::MidiSession()
{
    throw_pm_error( Pm_Initialize() );
}
MidiSession::~MidiSession()
{
    Pm_Terminate(); // errors in destructors are tricky
}
void MidiSession::restart()
{
    throw_pm_error( Pm_Terminate() );
    throw_pm_error( Pm_Initialize() );
}
MidiSession::Iter MidiSession::begin()
{
    return {0};
}
MidiSession::Iter MidiSession::end()
{
    return {Pm_CountDevices()};
}
MidiDevice MidiSession::default_input()
{
    return {Pm_GetDefaultInputDeviceID()};
}
MidiDevice MidiSession::default_output()
{
    return {Pm_GetDefaultOutputDeviceID()};
}
MidiInputStream MidiSession::open(MidiInputStream::Config & cfg)
{
    MidiInputStream out;

    if(cfg.device.index < 0)
        cfg.device.index = Pm_GetDefaultInputDeviceID();

    throw_pm_error( Pm_OpenInput(
        &out.backend,
        cfg.device.index,
        nullptr, // SysDepInfo
        cfg.buffer_size,
        (PmTimeProcPtr)cfg.clock_time_fn,
        cfg.clock_time_ctx
    ) );

    return out;
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

bool MidiInputStream::is_open() const
{
    return backend;
}
void MidiInputStream::close()
{
    throw_pm_error( close(std::nothrow_t{}) );
}
int MidiInputStream::close(std::nothrow_t)
{
    int stat = backend ? Pm_Close(backend) : 0;
    backend = nullptr;
    return stat;
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