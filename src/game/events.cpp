#include "events.h"

namespace banggame {
    void event_handler_map::handle_event(event_args &event) {
        if (event.index() == enums::indexof(event_type::delayed_action)) {
            std::invoke(std::get<0>(std::get<enums::indexof(event_type::delayed_action)>(event)));
        } else {
            std::vector<event_function *> handlers;

            for (event_function &handler : m_event_handlers | std::views::values) {
                if (handler.index() == event.index()) {
                    handlers.push_back(&handler);
                }
            }

            for (event_function *h : handlers) {
                enums::visit_indexed([&]<event_type T>(enums::enum_constant<T>, auto &fun) {
                    std::apply(fun, std::get<enums::indexof(T)>(event));
                }, *h);
            }
        }
    }
}