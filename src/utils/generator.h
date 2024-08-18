#ifndef __GENERATOR_H__
#define __GENERATOR_H__

#include <coroutine>
#include <variant>
#include <ranges>

namespace utils {
    struct suspend_maybe {
        bool ready;
        explicit suspend_maybe(bool ready) : ready(ready) { }
        bool await_ready() const noexcept { return ready; }
        void await_suspend(std::coroutine_handle<>) const noexcept { }
        void await_resume() const noexcept { }
    };

    template<typename T>
    class [[nodiscard]] generator {
    public:
        struct promise_type;
        class iterator;

        using handle_type = std::coroutine_handle<promise_type>;
        using range_type = std::ranges::subrange<iterator, std::default_sentinel_t>;

    private:
        handle_type handle;

        explicit generator(handle_type handle) : handle(std::move(handle)) { }
    public:
        class iterator {
        private:
            handle_type handle;
            friend generator;

            explicit iterator(handle_type handle) noexcept : handle(handle) { }

        public:
            using iterator_category = std::input_iterator_tag;
            using value_type = std::remove_cvref_t<T>;
            using difference_type = std::ptrdiff_t;
            
            explicit iterator() = default;

            bool operator==(std::default_sentinel_t) const noexcept { return handle.done(); }

            inline iterator &operator++();
            void operator++(int) { operator++(); }

            inline T const *operator->() const;
            T const &operator*() const { return *operator->(); }
        };

        iterator begin() {    
            if (auto *ex = std::get_if<std::exception_ptr>(&handle.promise().value)) {
                std::rethrow_exception(*ex);
            } else {
                return iterator{handle};
            }
        }

        std::default_sentinel_t end() const noexcept { return std::default_sentinel; }

        struct promise_type {
            std::variant<T const*, std::exception_ptr, range_type> value = nullptr;

            generator get_return_object() {
                return generator(handle_type::from_promise(*this));
            }

            std::suspend_never initial_suspend() { return {}; }
            std::suspend_always final_suspend() noexcept { return {}; }
            void unhandled_exception() {
                value = std::current_exception();
            }
            std::suspend_always yield_value(T const &x) noexcept {
                value = std::addressof(x);
                return {};
            }
            suspend_maybe await_transform(generator &&source) {
                range_type range(source);
                value = range;
                return suspend_maybe(range.empty());
            }
            void return_void() { }
        };

        generator(generator const&) = delete;
        generator(generator &&other) noexcept : handle(std::move(other.handle)) {
            other.handle = nullptr;
        }
        ~generator() { if(handle) handle.destroy(); }
        generator& operator=(generator const&) = delete;
        generator& operator=(generator &&other) noexcept {
            std::swap(handle, other.handle);
            return *this;
        }
    };

    template<typename T>
    inline auto generator<T>::iterator::operator++() -> iterator& {
        auto *value = &handle.promise().value;
        auto *range = std::get_if<range_type>(value);
        if (!range || range->advance(1).empty()) {
            handle.resume();
        }
        if (auto *ex = std::get_if<std::exception_ptr>(value)) {
            std::rethrow_exception(*ex);
        }
        return *this;
    }

    template<typename T>
    inline auto generator<T>::iterator::operator->() const -> T const* {
        auto *value = &handle.promise().value;
        auto *range = std::get_if<range_type>(value);
        if (range) {
            return range->begin().operator->();
        } else {
            return std::get<T const *>(*value);
        }
    }
}

#endif