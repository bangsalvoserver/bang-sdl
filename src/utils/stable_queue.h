#ifndef __STABLE_QUEUE_H__
#define __STABLE_QUEUE_H__

#include <queue>

namespace utils {

    template<typename T>
    using stable_element = std::pair<T, size_t>;

    template<typename T, typename Compare>
    struct stable_compare : private Compare {
        stable_compare() requires std::is_default_constructible_v<Compare> = default;

        template<std::convertible_to<Compare> U>
        stable_compare(U &&value) : Compare(std::forward<U>(value)) {}

        bool operator()(const stable_element<T> &lhs, const stable_element<T> &rhs) const {
            return Compare::operator()(lhs.first, rhs.first)
                || !Compare::operator()(rhs.first, lhs.first)
                && rhs.second < lhs.second;
        }
    };

    template <typename T, typename Compare = std::less<T>>
    class stable_priority_queue : public std::priority_queue<stable_element<T>, std::vector<stable_element<T>>, stable_compare<T, Compare>> {
        using base = std::priority_queue<stable_element<T>, std::vector<stable_element<T>>, stable_compare<T, Compare>>;

    public:
        using base::priority_queue;

        const T &top() const { return base::c.front().first; }
        T &top() { return base::c.front().first; }

        void push(const T& value) {
            emplace(value);
        }

        void push(T&& value) {
            emplace(std::move(value));
        }
        
        template<typename ... Args>
        void emplace(Args&&... args) {
            base::emplace(std::piecewise_construct,
                std::make_tuple(std::forward<Args>(args) ...),
                std::make_tuple(m_counter));
            ++m_counter;
        }

        void pop() {
            base::pop();
            if (base::empty()) m_counter = 0;
        }

    protected:
        std::size_t m_counter = 0;
    };
    
}

#endif