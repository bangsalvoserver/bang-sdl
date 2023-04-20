#ifndef __COLOR_TRACKER_H__
#define __COLOR_TRACKER_H__

#include "sdl_wrap.h"

#include <map>

namespace sdl {

    struct color_key {
        color *color_ptr;
        size_t counter;

        bool operator == (const color_key &other) const = default;

        auto operator <=> (const color_key &other) const {
            return color_ptr == other.color_ptr ?
                counter <=> other.counter :
                color_ptr <=> other.color_ptr;
        }

        auto operator <=> (color *other) const {
            return color_ptr <=> other;
        }
    };

    using order_color_multimap = std::multimap<color_key, color, std::greater<>>;
    using order_color_iterator = typename order_color_multimap::iterator;

    class color_tracker {
    private:
        order_color_multimap m_colors;
        size_t m_counter = 0;
    
    public:
        color_tracker() = default;
        ~color_tracker() {
            for (auto &[key, _] : m_colors) {
                *key.color_ptr = {};
            }
        }

        color_tracker(const color_tracker &other) = delete;
        color_tracker(color_tracker &&other) noexcept
            : m_colors(std::move(other.m_colors))
            , m_counter(other.m_counter)
        {
            other.m_colors.clear();
        }

        color_tracker &operator = (const color_tracker &other) = delete;
        color_tracker &operator = (color_tracker &&other) noexcept {
            std::swap(m_colors, other.m_colors);
            m_counter = other.m_counter;
            return *this;
        }
    
    public:
        order_color_iterator add(color *color_ptr, color value) {
            *color_ptr = value;
            auto it = m_colors.emplace(color_key{color_ptr, m_counter}, value);
            ++m_counter;
            return it;
        }

        void remove(order_color_iterator it) {
            sdl::color *color_ptr = it->first.color_ptr;
            m_colors.erase(it);
            auto lower = m_colors.lower_bound(color_ptr);
            if (m_colors.empty()) {
                m_counter = 0;
            }
            if (lower != m_colors.end() && lower->first.color_ptr == color_ptr) {
                *color_ptr = lower->second;
            } else {
                *color_ptr = {};
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
            : m_iterators(std::move(other.m_iterators))
        {
            other.m_iterators.clear();
        }
        
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