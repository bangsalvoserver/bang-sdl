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
        COMMAND python "${CMAKE_CURRENT_SOURCE_DIR}/pack.py"
        -q
        -D
        "${CMAKE_CURRENT_SOURCE_DIR}/${root_path}"
        "${out_file}"
        ${abs_files}
        DEPENDS ${abs_files} "${CMAKE_CURRENT_SOURCE_DIR}/pack.py"
        VERBATIM
    )
    add_custom_target("${target_name}" DEPENDS "${out_file}")
endfunction()

file(GLOB_RECURSE card_files cards/*.png)
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

if (NOT MSVC)
set(sound_files
    sounds/bandidos.wav
    sounds/bang.wav
    sounds/death.wav
    sounds/draw.wav
    sounds/duel.wav
    sounds/dynamite.wav
    sounds/gamestart.wav
    sounds/gatling.wav
    sounds/generalstore.wav
    sounds/indians.wav
    sounds/invalid.wav
    sounds/shuffle.wav
    sounds/snake.wav
)
endif()

pack_resources(sounds_pak sounds "${CMAKE_BINARY_DIR}/sounds.pak" "${sound_files}")