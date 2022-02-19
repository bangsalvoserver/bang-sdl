#ifndef __TSQUEUE_H__
#define __TSQUEUE_H__

#include <mutex>
#include <deque>

namespace util {
    template<typename T>
    class tsqueue {
    public:
        tsqueue() = default;
        tsqueue(const tsqueue &) = delete;
        tsqueue(tsqueue &&) = default;

        tsqueue &operator = (const tsqueue &) = delete;
        tsqueue &operator = (tsqueue &&) = delete;

    public:
        void push_back(const T &value) {
            std::scoped_lock lock(m_mutex);
            m_queue.push_back(value);
        }

        void push_back(T &&value) {
            std::scoped_lock lock(m_mutex);
            m_queue.push_back(std::move(value));
        }

        template<typename ... Ts>
        T &emplace_back(Ts && ... args) {
            std::scoped_lock lock(m_mutex);
            return m_queue.emplace_back(std::forward<Ts>(args) ... );
        }

        T pop_front() {
            std::scoped_lock lock(m_mutex);
            T value = std::move(m_queue.front());
            m_queue.pop_front();
            return value;
        }

        const T &front() const {
            std::scoped_lock lock(m_mutex);
            return m_queue.front();
        }

        T &front() {
            std::scoped_lock lock(m_mutex);
            return m_queue.front();
        }

        const T &back() const {
            std::scoped_lock lock(m_mutex);
            return m_queue.back();
        }

        T &back() {
            std::scoped_lock lock(m_mutex);
            return m_queue.back();
        }

        bool empty() const {
            std::scoped_lock lock(m_mutex);
            return m_queue.empty();
        }

        size_t size() const {
            std::scoped_lock lock(m_mutex);
            return m_queue.size();
        }

        void clear() {
            std::scoped_lock lock(m_mutex);
            m_queue.clear();
        }

    private:
        mutable std::mutex m_mutex;
        std::deque<T> m_queue;
    };
}

#endif