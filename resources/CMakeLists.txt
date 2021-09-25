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

compile_resources(fonts arial.ttf)
add_library(fonts OBJECT ${fonts})

compile_resources(card_images cards.pak characters.pak misc.pak)
add_library(card_images OBJECT ${card_images})