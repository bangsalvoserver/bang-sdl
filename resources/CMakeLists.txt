include("${CMAKE_SOURCE_DIR}/external/banggame/external/cpputils/resources/CMakeLists.txt")

compile_resources(game_options game_options.json)

add_executable(packer pack.cpp)
function(pack_resources target_name root_path out_file in_files)
    foreach(file ${in_files})
        if (IS_ABSOLUTE "${file}")
            list(APPEND abs_files "${file}")
        else()
            list(APPEND abs_files "${CMAKE_CURRENT_SOURCE_DIR}/${file}")
        endif()
    endforeach()
    add_custom_command(
        OUTPUT "${out_file}"
        COMMAND packer
        -q
        -D
        "${CMAKE_CURRENT_SOURCE_DIR}/${root_path}"
        "${out_file}"
        ${abs_files}
        DEPENDS ${abs_files} packer
        VERBATIM
    )
    add_custom_target("${target_name}" DEPENDS "${out_file}")
endfunction()

set(bang_cards_json "${CMAKE_SOURCE_DIR}/external/banggame/resources/bang_cards.json")
file(READ "${bang_cards_json}" bang_cards_json_content)
string(JSON num_of_keys LENGTH "${bang_cards_json_content}")
math(EXPR max_index "${num_of_keys} - 1")
foreach(IDX RANGE 0 ${max_index})
    string(JSON member_name MEMBER "${bang_cards_json_content}" ${IDX})
    string(JSON current_member GET "${bang_cards_json_content}" "${member_name}")
    string(JSON num_of_values LENGTH "${current_member}")
    math(EXPR max_value_index "${num_of_values} - 1")
    foreach(IDX RANGE 0 ${max_value_index})
        string(JSON current_card GET "${current_member}" ${IDX})
        string(JSON card_image ERROR_VARIABLE image_not_found GET "${current_card}" "image")
        if (NOT image_not_found)
            list(APPEND card_files "cards/${member_name}/${card_image}.png")
        endif()
    endforeach()
endforeach()

list(APPEND card_files
    cards/backface/character.png
    cards/backface/goldrush.png
    cards/backface/main_deck.png
    cards/backface/role.png
    cards/role/deputy.png
    cards/role/outlaw.png
    cards/role/renegade.png
    cards/role/sheriff.png
    cards/role/deputy_3p.png
    cards/role/outlaw_3p.png
    cards/role/renegade_3p.png
    cards/misc/card_border.png
    cards/misc/card_mask.png
    cards/misc/suit_clubs.png
    cards/misc/suit_diamonds.png
    cards/misc/suit_hearts.png
    cards/misc/suit_spades.png
    cards/misc/rank_2.png
    cards/misc/rank_3.png
    cards/misc/rank_4.png
    cards/misc/rank_5.png
    cards/misc/rank_6.png
    cards/misc/rank_7.png
    cards/misc/rank_8.png
    cards/misc/rank_9.png
    cards/misc/rank_10.png
    cards/misc/rank_J.png
    cards/misc/rank_Q.png
    cards/misc/rank_K.png
    cards/misc/rank_A.png
)

pack_resources(cards_pak cards "${CMAKE_BINARY_DIR}/cards.pak" "${card_files}")

set(media_files
    media/background.png
    media/icon_bang.png
    media/icon_checkbox.png
    media/icon_default_user.png
    media/icon_disconnected.png
    media/icon_loading.png
    media/icon_owner.png
    media/icon_gold.png
    media/icon_turn.png
    media/icon_origin.png
    media/icon_target.png
    media/icon_winner.png
    media/icon_dead_players.png
    media/sprite_cube.png
    media/sprite_cube_border.png
    media/fonts/arial.ttf
    media/fonts/bkant_bold.ttf
    media/fonts/perdido.ttf
)

pack_resources(media_pak media "${CMAKE_BINARY_DIR}/media.pak" "${media_files}")