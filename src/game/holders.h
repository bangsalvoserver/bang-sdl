#ifndef __HOLDERS_H__
#define __HOLDERS_H__

#include <concepts>
#include <memory>

#include "utils/reflector.h"
#include "effects/card_effect.h"
#include "game_action.h"

namespace banggame {

    DEFINE_ENUM_FWD_TYPES_IN_NS(banggame, effect_type,
        (none)
        (play_card_action,      effect_play_card_action)
        (max_usages,            effect_max_usages)
        (pass_turn,             effect_pass_turn)
        (resolve,               effect_resolve)
        (mth_add,               effect_empty)
        (bang,                  effect_bang)
        (bangcard,              effect_bangcard)
        (banglimit,             effect_banglimit)
        (missed,                effect_missed)
        (missedcard,            effect_missed)
        (bangresponse,          effect_bangresponse)
        (barrel,                effect_barrel)
        (destroy,               effect_destroy)
        (choose_card,           effect_choose_card)
        (startofturn,           effect_startofturn)
        (repeatable,            effect_empty)
        (drawing,               effect_drawing)
        (steal,                 effect_steal)
        (duel,                  effect_duel)
        (beer,                  effect_beer)
        (heal,                  effect_heal)
        (heal_notfull,          effect_heal_notfull)
        (indians,               effect_indians)
        (draw,                  effect_draw)
        (draw_discard,          effect_draw_discard)
        (draw_one_less,         effect_draw_one_less)
        (generalstore,          effect_generalstore)
        (deathsave,             effect_deathsave)
        (backfire,              effect_backfire)
        (bandidos,              effect_bandidos)
        (aim,                   effect_aim)
        (poker,                 effect_poker)
        (tornado,               effect_tornado)
        (damage,                effect_damage)
        (saved,                 effect_saved)
        (escape,                effect_escape)
        (sell_beer,             effect_sell_beer)
        (discard_black,         effect_discard_black)
        (add_gold,              effect_add_gold)
        (pay_gold,              effect_pay_gold)
        (rum,                   effect_rum)
        (bottle,                effect_empty)
        (pardner,               effect_empty)
        (goldrush,              effect_goldrush)
        (select_cube,           effect_select_cube)
        (pay_cube,              effect_pay_cube)
        (add_cube,              effect_add_cube)
        (reload,                effect_reload)
        (rust,                  effect_rust)
        (bandolier,             effect_bandolier)
        (belltower,             effect_belltower)
        (doublebarrel,          effect_doublebarrel)
        (thunderer,             effect_thunderer)
        (buntlinespecial,       effect_buntlinespecial)
        (bigfifty,              effect_bigfifty)
        (flintlock,             effect_flintlock)
        (duck,                  effect_duck)
        (move_bomb,             effect_move_bomb)
        (tumbleweed,            effect_tumbleweed)
        (changewws,             effect_empty)
        (sniper,                effect_sniper)
        (ricochet,              effect_ricochet)
        (peyotechoice,          effect_empty)
        (handcuffschoice,       effect_empty)
        (teren_kill,            effect_teren_kill)
        (greygory_deck,         effect_greygory_deck)
        (lemonade_jim,          effect_lemonade_jim)
        (josh_mccloud,          effect_josh_mccloud)
        (frankie_canton,        effect_frankie_canton)
        (evelyn_shebang,        effect_evelyn_shebang)
        (red_ringo,             effect_red_ringo)
        (al_preacher,           effect_al_preacher)
        (ms_abigail,            effect_ms_abigail)
        (graverobber,           effect_graverobber)
        (mirage,                effect_mirage)
        (disarm,                effect_disarm)
        (sacrifice,             effect_sacrifice)
        (lastwill,              effect_lastwill)
    )

    DEFINE_ENUM_FWD_TYPES_IN_NS(banggame, equip_type,
        (none)
        (max_hp,                effect_max_hp)
        (mustang,               effect_mustang)
        (scope,                 effect_scope)
        (jail,                  effect_jail)
        (dynamite,              effect_dynamite)
        (horse,                 effect_horse)
        (weapon,                effect_weapon)
        (volcanic,              effect_volcanic)
        (pickaxe,               effect_pickaxe)
        (calumet,               effect_calumet)
        (boots,                 effect_boots)
        (ghost,                 effect_ghost)
        (snake,                 effect_snake)
        (shotgun,               effect_shotgun)
        (bounty,                effect_bounty)
        (el_gringo,             effect_el_gringo)
        (buy_cost,              effect_empty)
        (horsecharm,            effect_horsecharm)
        (luckycharm,            effect_luckycharm)
        (gunbelt,               effect_gunbelt)
        (wanted,                effect_wanted)
        (bomb,                  effect_bomb)
        (tumbleweed,            effect_tumbleweed)
        (bronco,                effect_bronco)
        (calamity_janet,        effect_calamity_janet)
        (black_jack,            effect_black_jack)
        (kit_carlson,           effect_kit_carlson)
        (claus_the_saint,       effect_claus_the_saint)
        (bill_noface,           effect_bill_noface)
        (slab_the_killer,       effect_slab_the_killer)
        (suzy_lafayette,        effect_suzy_lafayette)
        (vulture_sam,           effect_vulture_sam)
        (johnny_kisch,          effect_johnny_kisch)
        (bellestar,             effect_bellestar)
        (greg_digger,           effect_greg_digger)
        (herb_hunter,           effect_herb_hunter)
        (molly_stark,           effect_molly_stark)
        (tequila_joe,           effect_tequila_joe)
        (vera_custer,           effect_vera_custer)
        (tuco_franziskaner,     effect_tuco_franziskaner)
        (colorado_bill,         effect_colorado_bill)
        (henry_block,           effect_henry_block)
        (big_spencer,           effect_big_spencer)
        (gary_looter,           effect_gary_looter)
        (john_pain,             effect_john_pain)
        (youl_grinner,          effect_youl_grinner)
        (don_bell,              effect_don_bell)
        (dutch_will,            effect_dutch_will)
        (madam_yto,             effect_madam_yto)
        (greygory_deck,         effect_greygory_deck)
        (lemonade_jim,          effect_lemonade_jim)
        (evelyn_shebang,        effect_evelyn_shebang)
        (julie_cutter,          effect_julie_cutter)
        (bloody_mary,           effect_bloody_mary)
        (red_ringo,             effect_red_ringo)
        (al_preacher,           effect_al_preacher)
        (ms_abigail,            effect_ms_abigail)
        (blessing,              effect_blessing)
        (curse,                 effect_curse)
        (thedaltons,            effect_thedaltons)
        (thedoctor,             effect_thedoctor)
        (trainarrival,          effect_trainarrival)
        (thirst,                effect_thirst)
        (highnoon,              effect_highnoon)
        (shootout,              effect_shootout)
        (invert_rotation,       effect_invert_rotation)
        (reverend,              effect_reverend)
        (hangover,              effect_hangover)
        (sermon,                effect_sermon)
        (ghosttown,             effect_ghosttown)
        (handcuffs,             effect_handcuffs)
        (ambush,                effect_ambush)
        (lasso,                 effect_lasso)
        (judge,                 effect_judge)
        (peyote,                effect_peyote)
        (russianroulette,       effect_russianroulette)
        (abandonedmine,         effect_abandonedmine)
        (deadman,               effect_deadman)
        (fistfulofcards,        effect_fistfulofcards)
        (packmule,              effect_packmule)
        (indianguide,           effect_indianguide)
        (taxman,                effect_taxman)
        (lastwill,              effect_lastwill)
        (brothel,               effect_brothel)
        (newidentity,           effect_newidentity)
        (lawofthewest,          effect_lawofthewest)
        (vendetta,              effect_vendetta)
    )

    DEFINE_ENUM_FWD_TYPES_IN_NS(banggame, mth_type,
        (none)
        (doc_holyday,           handler_doc_holyday)
        (flint_westwood,        handler_flint_westwood)
        (draw_atend,            handler_draw_atend)
        (fanning,               handler_fanning)
        (move_bomb,             handler_move_bomb)
        (squaw,                 handler_squaw)
        (card_sharper,          handler_card_sharper)
        (lastwill,              handler_lastwill)
    )

    void handle_multitarget(card *origin_card, player *origin, mth_target_list targets);

    template<enums::reflected_enum E>
    struct effect_base {
        using enum_type = E;

        REFLECTABLE(
            (play_card_target_type) target,
            (target_player_filter) player_filter,
            (target_card_filter) card_filter,
            (short) effect_value,
            (enum_type) type
        )

        bool is(enum_type value) const { return type == value; }
    };

    struct effect_holder : effect_base<effect_type> {
        using effect_base<effect_type>::effect_base;

        bool can_respond(card *origin_card, player *target) const;

        void verify(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin, effect_flags flags);
        
        void verify(card *origin_card, player *origin, player *target) const;
        void on_play(card *origin_card, player *origin, player *target, effect_flags flags);
        
        void verify(card *origin_card, player *origin, player *target, card *target_card) const;
        void on_play(card *origin_card, player *origin, player *target, card *target_card, effect_flags flags);
    };
    
    struct equip_holder : effect_base<equip_type> {
        using effect_base<equip_type>::effect_base;

        void on_pre_equip(card *target_card, player *target);
        void on_equip(card *target_card, player *target);
        void on_unequip(card *target_card, player *target);
    };

    class request_holder {
    public:
        template<std::derived_from<request_base> T, typename ... Ts>
        request_holder(std::type_identity<T>, Ts && ... args)
            : m_value(std::make_shared<T>(std::forward<Ts>(args) ... )) {}

        card *origin_card() const {
            return m_value->origin_card;
        }
        player *origin() const {
            return m_value->origin;
        }
        player *target() const {
            return m_value->target;
        }
        effect_flags flags() const {
            return m_value->flags;
        }
        game_formatted_string status_text(player *owner) const {
            return m_value->status_text(owner);
        }

        bool can_pick(card_pile_type pile, player *target, card *target_card) const {
            return m_value->can_pick(pile, target, target_card);
        }
        void on_pick(card_pile_type pile, player *target, card *target_card) {
            auto copy = m_value;
            copy->on_pick(pile, target, target_card);
        }

        template<typename T> auto &get() {
            return dynamic_cast<T &>(*m_value);
        }

        template<typename T> const auto &get() const {
            return dynamic_cast<const T &>(*m_value);
        }

        template<typename T> auto *get_if() {
            return dynamic_cast<T *>(m_value.get());
        }

        template<typename T> const auto *get_if() const {
            return dynamic_cast<const T *>(m_value.get());
        }

        template<typename T> bool is() const {
            return get_if<T>() != nullptr;
        }

    private:
        std::shared_ptr<request_base> m_value;
    };
}

#endif