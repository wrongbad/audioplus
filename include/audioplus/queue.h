#pragma once

#include <atomic>

namespace audioplus {

template<class T, int N, 
    class Index = uint32_t, 
    class Counter = std::atomic<Index> >
struct Queue
{
    T m_buf[N] {};
    Counter m_write {0};
    Counter m_read {0};
    Index m_head {0};
    Index m_tail {0};

    Index write_ready() 
    {
        return N - ( m_write.load(std::memory_order_relaxed)
            - m_read.load(std::memory_order_acquire) );
    }

    T & write_slot(Index offset = 0)
    {
        return m_buf[(m_tail + offset) % N];
    }

    void write_commit(Index count = 1)
    {
        m_tail = (m_tail + count) % N;
        m_write.fetch_add(count, std::memory_order_release);
    }

    Index read_ready()
    {
        return m_write.load(std::memory_order_acquire)
            - m_read.load(std::memory_order_relaxed);
    }

    T & read_slot(Index offset = 0)
    {
        return m_buf[(m_head + offset) % N];
    }

    void read_commit(Index count = 1)
    {
        m_head = (m_head + count) % N;
        m_read.fetch_add(count, std::memory_order_release);
    }
};

} // namespace audioplus