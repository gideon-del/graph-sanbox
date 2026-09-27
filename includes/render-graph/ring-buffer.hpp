#pragma once
#include <vector>
template <typename T>
class RingBuffer
{
    std::vector<T> m_slots;
    size_t m_count = 0;

public:
    template <class F>
    void create(size_t count, F &&fn)
    {

        m_count = count;
        m_slots.clear();
        for (int i = 0; i < count; i++)
        {
            m_slots.push_back(std::move(fn(i)));
        }
    };

    T *get(uint32_t frameIdx = 0)
    {
        if (m_count == 0)
        {
            return nullptr;
        }

        auto idx = frameIdx % m_count;
        return &m_slots[idx];
    }

    ~RingBuffer()
    {
        m_slots.clear();
    }
};