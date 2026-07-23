#pragma once
#include <atomic>
#include <array>
#include <optional>

template <typename T, size_t N>
class SPSCQueue
{
    static_assert((N & (N - 1)) == 0, "Capacity must be a power of 2");

    std::array<T, N> m_buffer;
    alignas(64) std::atomic<size_t> m_head{0};
    alignas(64) std::atomic<size_t> m_tail{0};

public:
    bool push(T item)
    {
        auto tail = m_tail.load(std::memory_order_relaxed);
        auto head = m_head.load(std::memory_order_acquire);

        if ((tail - head) >= N)
            return false;
        m_buffer[tail & (N - 1)] = std::move(item);

        m_tail.store(tail + 1, std::memory_order_release);
        return true;
    };

    std::optional<T> pop()
    {
        auto head = m_head.load(std::memory_order_relaxed);
        auto tail = m_tail.load(std::memory_order_acquire);

        if (head == tail)
        {
            return std::nullopt;
        }
        T item = std::move(m_buffer[head & (N - 1)]);

        m_head.store(head + 1, std::memory_order_release);
        return item;
    };

    size_t size_approx() const
    {
        size_t h = m_head.load(std::memory_order_relaxed);
        size_t t = m_tail.load(std::memory_order_relaxed);
        return t - h;
    }

    bool empty() const
    {
        return m_head.load(std::memory_order_relaxed) ==
               m_tail.load(std::memory_order_relaxed);
    }

    static constexpr size_t capacity() { return N; }
};