function(compile_resources out_list)
    foreach(file IN LISTS ARGN)
        get_filename_component(varname "${file}" NAME)
        string(MAKE_C_IDENTIFIER "${varname}" varname)

        set(resource_c_file "${CMAKE_CURRENT_BINARY_DIR}/${file}.c")
        list(APPEND ret "${resource_c_file}")

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
    set(${out_list} ${ret} PARENT_SCOPE)
endfunction()

compile_resources(bang_cards bang_cards.json)
add_library(bang_cards OBJECT ${bang_cards})

compile_resources(resources_obj arial.ttf icons/icon_checkbox.png icons/bang.png)
add_library(resources_obj OBJECT ${resources_obj} icons/bang.rc)

add_executable(packer pack.cpp)
file(GLOB_RECURSE card_files "${CMAKE_CURRENT_SOURCE_DIR}/cards/*.png")
add_custom_command(
    OUTPUT "${CMAKE_BINARY_DIR}/cards.pak"
    COMMAND packer
        "${CMAKE_CURRENT_SOURCE_DIR}/cards/"
        "${CMAKE_BINARY_DIR}/cards.pak"
        -q
    DEPENDS ${card_files}
    VERBATIM
)
add_custom_target(cards_pak DEPENDS "${CMAKE_BINARY_DIR}/cards.pak")