#include "events.h"

namespace banggame {
    void event_handler_map::handle_event(event_args &event) {
        for (auto &[card_id, e] : m_event_handlers) {
            if (e.index() == event.index()) {
                enums::visit_indexed([&]<event_type T>(enums::enum_constant<T>, auto &fun) {
                    std::apply(fun, std::get<enums::indexof(T)>(event));
                }, e);
            }
        }
    }
}