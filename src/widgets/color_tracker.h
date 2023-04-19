#ifndef __COLOR_TRACKER_H__
#define __COLOR_TRACKER_H__

#include "sdl_wrap.h"

#include <map>

namespace sdl {

    using color_key = std::pair<size_t, color *>;

    struct color_key_ordering {
        bool operator ()(const color_key &lhs, const color_key &rhs) const {
            return lhs.first < rhs.first;
        }
    };

    using order_color_multimap = std::multimap<color_key, color, color_key_ordering>;
    using order_color_iterator = typename order_color_multimap::iterator;

    class color_tracker {
    private:
        order_color_multimap m_colors;
        size_t m_counter = 0;
    
    public:
        color_tracker() = default;
        ~color_tracker() {
            for (auto &[key, _] : m_colors) {
                *key.second = {};
            }
        }

        color_tracker(const color_tracker &other) = delete;
        color_tracker(color_tracker &&other) noexcept
            : m_colors(std::move(other.m_colors))
            , m_counter(other.m_counter) {}

        color_tracker &operator = (const color_tracker &other) = delete;
        color_tracker &operator = (color_tracker &&other) noexcept {
            std::swap(m_colors, other.m_colors);
            m_counter = other.m_counter;
            return *this;
        }
    
    public:
        order_color_iterator add(color *color_ptr, color value) {
            *color_ptr = value;
            auto it = m_colors.emplace(std::make_pair(m_counter, color_ptr), value);
            ++m_counter;
            return it;
        }

        void remove(order_color_iterator it) {
            auto key = it->first;
            m_colors.erase(it);
            auto [lower, upper] = m_colors.equal_range(key);
            if (m_colors.empty()) {
                m_counter = 0;
            }
            if (lower != upper) {
                *key.second = std::prev(upper)->second;
            } else {
                *key.second = {};
            }
        }
    };

    class color_iterator_list {
    private:
        std::vector<std::pair<color_tracker *, order_color_iterator>> m_iterators;
    
    public:
        color_iterator_list() = default;
        ~color_iterator_list() { clear(); }

        color_iterator_list(const color_iterator_list &other) = delete;
        color_iterator_list(color_iterator_list &&other) noexcept
            : m_iterators(std::move(other.m_iterators)) {}
        
        color_iterator_list &operator = (const color_iterator_list &other) = delete;
        color_iterator_list &operator = (color_iterator_list &&other) noexcept {
            std::swap(m_iterators, other.m_iterators);
            return *this;
        }

        void add(color_tracker &tracker, color &color_ref, color value) {
            m_iterators.emplace_back(&tracker, tracker.add(&color_ref, value));
        }

        void clear() {
            for (auto &[tracker, it] : m_iterators) {
                tracker->remove(it);
            }
            m_iterators.clear();
        }
    };

}

#endif