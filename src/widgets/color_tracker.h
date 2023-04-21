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
            if (this != &other) {
                std::swap(m_colors, other.m_colors);
                m_counter = other.m_counter;
            }
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

    class color_tracker_lifetime {
    private:
        color_tracker *m_tracker;
        order_color_iterator m_iterator;
    
    public:
        color_tracker_lifetime(color_tracker &tracker, color &color_ref, color value)
            : m_tracker(&tracker)
            , m_iterator(tracker.add(&color_ref, value)) {}

        ~color_tracker_lifetime() {
            if (m_tracker) {
                m_tracker->remove(m_iterator);
                m_tracker = nullptr;
            }
        }
        
        color_tracker_lifetime(const color_tracker_lifetime &other) = delete;
        color_tracker_lifetime(color_tracker_lifetime &&other) noexcept
            : m_tracker(std::exchange(other.m_tracker, nullptr))
            , m_iterator(std::move(other.m_iterator)) {}
        
        color_tracker_lifetime &operator = (const color_tracker_lifetime &other) = delete;
        color_tracker_lifetime &operator = (color_tracker_lifetime &&other) noexcept {
            if (this != &other) {
                std::swap(m_tracker, other.m_tracker);
                std::swap(m_iterator, other.m_iterator);
            }
            return *this;
        }
    };

}

#endif