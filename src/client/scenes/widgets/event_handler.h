#ifndef __EVENT_HANDLER_H__
#define __EVENT_HANDLER_H__

#include "common/sdl.h"

#include <vector>
#include <algorithm>
#include <functional>

namespace sdl {

    using button_callback_fun = std::function<void()>;

    class event_handler {
    private:
        static inline std::vector<event_handler *> s_handlers;
        static inline event_handler *s_focus = nullptr;
        bool m_enabled = true;

    protected:
        virtual bool handle_event(const sdl::event &event) = 0;

        virtual void on_gain_focus() {}
        virtual void on_lose_focus() {}

    public:
        event_handler() {
            s_handlers.push_back(this);
        }

        virtual ~event_handler() {
            if (focused()) {
                set_focus(nullptr);
            }
            s_handlers.erase(std::ranges::find(s_handlers, this));
        }

        static bool handle_events(const sdl::event &event) {
            for (auto &h : s_handlers) {
                if (h->enabled() && h->handle_event(event)) return true;
            }
            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                set_focus(nullptr);
            }
            return false;
        }

        static void set_focus(event_handler *focus) {
            if (s_focus != focus) {
                if (s_focus) {
                    s_focus->on_lose_focus();
                }
                s_focus = focus;
                if (s_focus) {
                    s_focus->on_gain_focus();
                }
            }
        }

        bool focused() const {
            return s_focus == this;
        }

        static bool is_focused(event_handler *e) {
            return s_focus == e;
        }

        void disable() noexcept { m_enabled = false; }
        void enable() noexcept { m_enabled = true; }
        void set_enabled(bool value) { m_enabled = value; }
        bool enabled() const noexcept { return m_enabled; }
    };

}

#endif