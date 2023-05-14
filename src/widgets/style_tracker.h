#ifndef __STYLE_TRACKER_H__
#define __STYLE_TRACKER_H__

#include <set>
#include <vector>
#include <optional>

namespace widgets {

    template<typename T>
    using style_pair = std::pair<T, size_t>;

    template<typename T>
    struct style_pair_ordering {
        bool operator ()(const style_pair<T> &lhs, const style_pair<T> &rhs) const {
            return lhs.second > rhs.second;
        }
    };

    template<typename T>
    using style_pair_set = std::set<style_pair<T>, style_pair_ordering<T>>;

    template<typename T>
    using style_set_iterator = typename style_pair_set<T>::iterator;

    template<typename T>
    class style_set {
    private:
        style_pair_set<T> m_styles;
        size_t m_counter = 0;
    
    protected:
        virtual void update_style() = 0;

    public:
        std::optional<T> get_style() const {
            if (!m_styles.empty()) {
                return m_styles.begin()->first;
            } else {
                return std::nullopt;
            }
        }

        style_set_iterator<T> add_style(const T &value) {
            auto it = m_styles.emplace(value, m_counter);
            ++m_counter;
            update_style();
            return it.first;
        }

        void remove_style(style_set_iterator<T> it) {
            m_styles.erase(it);
            if (m_styles.empty()) {
                m_counter = 0;
            }
            update_style();
        }
    };

    template<typename T>
    class style_tracker {
    private:
        style_set<T> *m_set;
        style_set_iterator<T> m_it;

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