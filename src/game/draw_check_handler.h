#ifndef __DRAW_CHECK_HANDLER_H__
#define __DRAW_CHECK_HANDLER_H__

#include "player.h"

namespace banggame {

    class draw_check_handler {
    private:
        player *m_origin = nullptr;
        card *m_origin_card = nullptr;
        draw_check_function m_function;

    public:
        void set(player *origin, card *origin_card, draw_check_function &&function);
        void start();
        void select(card *drawn_card);
        void restart();
        void resolve(card *drawn_card);
    };

}

#endif