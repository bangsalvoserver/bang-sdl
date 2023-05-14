#ifndef __STYLE_TRACKER_H__
#define __STYLE_TRACKER_H__

#include <list>
#include <optional>

namespace widgets {

    template<typename T>
    using style_iterator = typename std::list<T>::const_iterator; 

    template<typename T>
    class style_set {
    private:
        std::list<T> m_styles;

    public:
        std::optional<T> get_style() const {
            if (!m_styles.empty()) {
                return m_styles.back();
            } else {
                return std::nullopt;
            }
        }

        style_iterator<T> add_style(const T &value) {
            return m_styles.insert(m_styles.end(), value);
        }

        void remove_style(style_iterator<T> it) {
            m_styles.erase(it);
        }
    };

    template<typename T>
    class style_tracker {
    private:
        style_set<T> *m_set;
        style_iterator<T> m_it;

    public:
        style_tracker(style_set<T> *set, const T &value)
            : m_set{set}, m_it{set->add_style(value)} {}

        ~style_tracker() {
            if (m_set) {
                m_set->remove_style(m_it);
                m_set = nullptr;
            }
        }

        style_tracker(const style_tracker &other) = delete;
        style_tracker(style_tracker &&other) noexcept
            : m_set{std::exchange(other.m_set, nullptr)}
            , m_it{std::move(other.m_it)} {}

        style_tracker &operator = (const style_tracker &) = delete;
        style_tracker &operator = (style_tracker &&other) noexcept {
            if (this != &other) {
                std::swap(m_set, other.m_set);
                std::swap(m_it, other.m_it);
            }
            return *this;
        }
    };

}

#endif