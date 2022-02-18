#include "events.h"

namespace banggame {
    void event_handler_map::handle_event(event_args &event) {
        if (event.index() == enums::indexof(event_type::delayed_action)) {
            std::invoke(std::get<0>(std::get<enums::indexof(event_type::delayed_action)>(event)));
        } else {
            for (auto &[card_id, e] : m_event_handlers) {
                if (e.index() == event.index()) {
                    enums::visit_indexed([&]<event_type T>(enums::enum_constant<T>, auto &fun) {
                        std::apply(fun, std::get<enums::indexof(T)>(event));
                    }, e);
                }
            }
        }
    }
}