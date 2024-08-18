#ifndef __STATIC_MAP_H__
#define __STATIC_MAP_H__

#include <algorithm>
#include <array>

namespace utils {

    namespace impl {
        template<typename Key, typename Value, size_t Size>
        class static_map {
        private:
            using value_type = std::pair<Key, Value>;
            std::array<value_type, Size> m_data;

        public:
            constexpr static_map(value_type (&&data)[Size]) {
                std::ranges::move(data, m_data.begin());
                std::ranges::sort(m_data, {}, &value_type::first);
                if (!std::ranges::empty(std::ranges::unique(m_data, {}, &value_type::first))) {
                    throw "Keys must be unique";
                }
            }

            constexpr auto begin() const { return m_data.begin(); }
            constexpr auto end() const { return m_data.end(); }

            constexpr auto find(const auto &key) const {
                if (auto it = std::ranges::lower_bound(m_data, key, {}, &value_type::first); it != end() && it->first == key) {
                    return it;
                } else {
                    return end();
                }
            }
        };
    }

    template<typename Key, typename Value, size_t Size>
    constexpr auto static_map(std::pair<Key, Value> (&&data)[Size]) {
        return impl::static_map<Key, Value, Size>(std::move(data));
    }

}

#endif