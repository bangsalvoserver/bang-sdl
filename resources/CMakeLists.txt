function(compile_resources target_name)
    foreach(file IN LISTS ARGN)
        get_filename_component(varname "${file}" NAME)
        string(MAKE_C_IDENTIFIER "${varname}" varname)

        set(resource_c_file "${CMAKE_CURRENT_BINARY_DIR}/${file}.c")
        list(APPEND out_list "${resource_c_file}")

        add_custom_command(
            OUTPUT "${resource_c_file}"
            COMMAND ${CMAKE_COMMAND}
                -D "resource_file_name=${CMAKE_CURRENT_SOURCE_DIR}/${file}"
                -D "source_file_name=${resource_c_file}"
                -D "variable_name=__resource__${varname}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/EmbedResource.cmake"
            DEPENDS "${file}"
            VERBATIM
        )
    endforeach()
    add_library(${target_name} OBJECT ${out_list})
endfunction()

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    compile_resources(bang_cards bang_cards.json)
endif()

compile_resources(resources_obj
    icons/icon_checkbox.png
    icons/icon_default_user.png
    icons/icon_loading.png
    icons/icon_disconnected.png
    icons/icon_owner.png
    icons/bang.png
    background.png)

compile_resources(fonts_obj
    fonts/arial.ttf
    fonts/bkant_bold.ttf
    fonts/perdido.ttf
)

add_executable(compile_locale compile_locale.cpp)
function(compile_locales target_name)
    string(APPEND all_locales_content "// Auto generated file.\n")
    string(APPEND all_locales_content "#ifndef __LOCALES_H__\n")
    string(APPEND all_locales_content "#define __LOCALES_H__\n\n")
    foreach(file IN LISTS ARGN)
        get_filename_component(basename "${file}" NAME)
        string(MAKE_C_IDENTIFIER "${basename}" varname)

        set(locale_h_file "${CMAKE_CURRENT_BINARY_DIR}/locales/${file}.h")
        list(APPEND out_list "${locale_h_file}")

        add_custom_command(
            OUTPUT "${locale_h_file}"
            COMMAND compile_locale
                "${CMAKE_CURRENT_SOURCE_DIR}/${file}"
                "${locale_h_file}"
                "${varname}"
            DEPENDS "${file}" compile_locale
            VERBATIM
        )
        string(APPEND all_locales_content "#include \"${basename}.h\"\n")
    endforeach()
    set(all_locales_header "${CMAKE_CURRENT_BINARY_DIR}/locales/locales.h")
    string(APPEND all_locales_content "\n#endif")
    file(WRITE "${all_locales_header}" "${all_locales_content}")
    add_library(${target_name} INTERFACE ${out_list} ${all_locales_header})
endfunction()

compile_locales(locale_headers locale_it.txt locale_en.txt)
target_include_directories(locale_headers INTERFACE "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(packer pack.cpp)
file(GLOB_RECURSE card_files "${CMAKE_CURRENT_SOURCE_DIR}/cards/*.png")
add_custom_command(
    OUTPUT "${CMAKE_BINARY_DIR}/cards.pak"
    COMMAND packer
        "${CMAKE_CURRENT_SOURCE_DIR}/cards/"
        "${CMAKE_BINARY_DIR}/cards.pak"
        -q
    DEPENDS ${card_files} packer
    VERBATIM
)
add_custom_target(cards_pak DEPENDS "${CMAKE_BINARY_DIR}/cards.pak")