#ifndef __RAII_EDITOR__
#define __RAII_EDITOR__

#include <utility>

template<typename T>
class raii_editor {
private:
    T *m_ptr = nullptr;
    T m_prev_value;

public:
    raii_editor() = default;
    
    raii_editor(T &value, const T &new_value)
        : m_ptr(&value)
        , m_prev_value(value)
    {
        value = new_value;
    }
    
    ~raii_editor() {
        if (m_ptr) {
            *m_ptr = std::move(m_prev_value);
        }
    }

    raii_editor(raii_editor &&other) noexcept
        : m_ptr(std::exchange(other.m_ptr, nullptr))
        , m_prev_value(std::move(other.m_prev_value)) {}

    raii_editor(const raii_editor &other) = delete;

    raii_editor &operator = (raii_editor &&other) noexcept {
        if (m_ptr) {
            *m_ptr = std::move(m_prev_value);
        }
        m_ptr = std::exchange(other.m_ptr, nullptr);
        m_prev_value = std::move(other.m_prev_value);
        return *this;
    }

    raii_editor &operator = (const raii_editor &other) = delete;
};

#endif