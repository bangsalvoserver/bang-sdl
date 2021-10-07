#ifndef __EVENT_HANDLER_H__
#define __EVENT_HANDLER_H__

#include "utils/sdl.h"

#include <vector>
#include <algorithm>

namespace sdl {

    class event_handler {
    private:
        static inline std::vector<event_handler *> s_handlers;

    protected:
        virtual bool handle_event(const sdl::event &event) = 0;

    public:
        event_handler() {
            s_handlers.push_back(this);
        }

        virtual ~event_handler() {
            s_handlers.erase(std::ranges::find(s_handlers, this));
        }

        static bool handle_events(const sdl::event &event) {
            for (auto &h : s_handlers) {
                if (h->handle_event(event)) return true;
            }
            return false;
        }
    };

}

#endif