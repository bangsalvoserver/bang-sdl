#ifndef __ID_MAP_H__
#define __ID_MAP_H__

#include <memory>
#include <vector>
#include <functional>

namespace utils {

    template<typename T, typename IdGetter> class id_map;

    template<typename IDMap>
    class id_map_iterator {
    private:
        template<typename T, typename IdGetter> friend class id_map;
        template<typename T> friend class id_map_iterator;

        template<typename T> struct map_unwrapper {};
        template<typename T, typename IdGetter> struct map_unwrapper<id_map<T, IdGetter>> {
            using value_type = T;
            using base_iterator = typename std::vector<std::unique_ptr<T>>::iterator;
        };
        template<typename T, typename IdGetter> struct map_unwrapper<const id_map<T, IdGetter>> {
            using value_type = T;
            using base_iterator = typename std::vector<std::unique_ptr<T>>::const_iterator;
        };

    private:
        const IDMap *m_map = nullptr;
        using base_iterator = typename map_unwrapper<IDMap>::base_iterator;
        base_iterator m_it;

    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = typename map_unwrapper<IDMap>::value_type;
        using pointer = value_type*;
        using reference = value_type&;

        id_map_iterator() = default;

    private:
        id_map_iterator(const IDMap &map, base_iterator it)
            : m_map(&map), m_it(it) {}
            
        static id_map_iterator make_begin(const IDMap &map, base_iterator begin, base_iterator end) {
            id_map_iterator it(map, begin);
            while (it.m_it != end && *it.m_it == nullptr) {
                ++it.m_it;
            }
            return it;
        }

    public:
        reference operator *() { return **m_it; }
        const reference operator *() const { return **m_it; }

        pointer operator ->() { return m_it->get(); }
        const pointer operator ->() const { return m_it->get(); }

        id_map_iterator &operator ++() {
            do {
                ++m_it;
            } while (m_it != m_map->end().m_it && *m_it == nullptr);
            return *this;
        }

        id_map_iterator operator ++(int) {
            id_map_iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        id_map_iterator &operator --() {
            do {
                --m_it;
            } while (*m_it == nullptr);
            return *this;
        }

        id_map_iterator operator --(int) {
            id_map_iterator tmp = *this;
            --(*this);
            return tmp;
        }

        bool operator == (const id_map_iterator &other) const { return m_it == other.m_it; }
        auto operator <=> (const id_map_iterator &other) const { return m_it <=> other.m_it; }
    };

    template<typename T> requires requires (const T &value) {
        { value.id } -> std::convertible_to<size_t>;
    } struct default_id_getter {
        size_t operator()(const T &value) const {
            return value.id;
        }
    };

    template<typename T, typename IdGetter = default_id_getter<T>>
    class id_map : private IdGetter {
    private:
        using data_vector = std::vector<std::unique_ptr<T>>;
        data_vector m_data;
        size_t m_size = 0;
        size_t m_first_available_id = 1;

        size_t get_id(const T &value) {
            return std::invoke(static_cast<IdGetter &>(*this), value);
        }

    public:
        using value_type = T;
        using iterator = id_map_iterator<id_map<T, IdGetter>>;
        using const_iterator = id_map_iterator<const id_map<T, IdGetter>>;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        id_map() requires (std::is_default_constructible_v<IdGetter>) = default;

        template<std::convertible_to<IdGetter> U>
        requires (!std::is_default_constructible_v<IdGetter>)
        id_map(U &&id_getter) : IdGetter(std::forward<U>(id_getter)) {}

        iterator begin() { return iterator::make_begin(*this, m_data.begin(), m_data.end()); }
        const_iterator cbegin() const { return const_iterator::make_begin(*this, m_data.cbegin(), m_data.cend()); }
        const_iterator begin() const { return cbegin(); }

        iterator end() { return iterator(*this, m_data.end()); }
        const_iterator cend() const { return const_iterator(*this, m_data.cend()); }
        const_iterator end() const { return cend(); }

        reverse_iterator rbegin() { return reverse_iterator(end()); }
        const_reverse_iterator crbegin() const { return const_reverse_iterator(cend()); }
        const_reverse_iterator rbegin() const { return crbegin(); }

        reverse_iterator rend() { return reverse_iterator(begin()); }
        const_reverse_iterator crend() const { return const_reverse_iterator(end()); }
        const_reverse_iterator rend() const { return crend(); }

    public:
        std::unique_ptr<T> extract(size_t id) {
            m_first_available_id = std::min(id, m_first_available_id);
            --m_size;
            return std::move(m_data[id - 1]);
        }

        std::unique_ptr<T> &insert(std::unique_ptr<T> &&ptr) {
            size_t id = get_id(*ptr);
            if (id > m_data.size()) {
                m_data.resize(id);
            }
            if (id == m_first_available_id) {
                do {
                    ++m_first_available_id;
                } while (m_first_available_id <= m_data.size() && m_data[m_first_available_id - 1] != nullptr);
            }
            ++m_size;
            return m_data[id - 1] = std::move(ptr);
        }

        template<typename ... Ts>
        T &emplace(Ts && ... args) {
            return *insert(std::make_unique<T>(std::forward<Ts>(args) ... ));
        }

        std::pair<T &, bool> try_insert(std::unique_ptr<T> &&ptr) {
            size_t id = get_id(*ptr);
            if (id > m_data.size()) {
                m_data.resize(id);
            }
            if (id == m_first_available_id) {
                do {
                    ++m_first_available_id;
                } while (m_first_available_id <= m_data.size() && m_data[m_first_available_id - 1] != nullptr);
            }
            if (auto &val = m_data[id - 1]) {
                return {*val, false};
            } else {
                ++m_size;
                return {*(val = std::move(ptr)), true};
            }
        }

        template<typename ... Ts>
        std::pair<T &, bool> try_emplace(Ts && ... args) {
            return try_insert(std::make_unique<T>(std::forward<Ts ...>(args) ... ));
        }

        iterator find(size_t id) {
            if (id > m_data.size() || id == 0) return end();
            auto it = m_data.begin() + id - 1;
            if (*it) return iterator(*this, it);
            else return end();
        }

        const_iterator find(size_t id) const {
            if (id > m_data.size() || id == 0) return cend();
            auto it = m_data.cbegin() + id - 1;
            if (*it) return const_iterator(*this, it);
            else return cend();
        }

        void erase(iterator it) {
            m_first_available_id = std::min(get_id(*it), m_first_available_id);
            --m_size;
            it.m_it->reset();
        }

        void erase(size_t id) {
            erase(find(id));
        }

        size_t size() const {
            return m_size;
        }

        size_t first_available_id() const {
            return m_first_available_id;
        }

        void clear() {
            m_data.clear();
            m_size = 0;
            m_first_available_id = 1;
        }
    };
}

#endif