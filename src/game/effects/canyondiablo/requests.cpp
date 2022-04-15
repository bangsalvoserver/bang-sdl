#include "requests.h"
#include "effects.h"

#include "../../game.h"

namespace banggame {

    void request_card_sharper::on_resolve() {
        target->m_game->pop_request<request_card_sharper>();
        
        handler_card_sharper{}.on_resolve(origin_card, origin, target, chosen_card, target_card);
    }

    game_formatted_string request_card_sharper::status_text(player *owner) const {
        if (target == owner) {
            return {"STATUS_CARD_SHARPER", origin_card, target_card, chosen_card};
        } else {
            return {"STATUS_CARD_SHARPER_OTHER", target, origin_card, target_card, chosen_card};
        }
    }
}