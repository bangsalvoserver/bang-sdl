set(bang_resources
bang_cards.json
)

foreach(file ${bang_resources})
    get_filename_component(varname "${file}" NAME)
    string(MAKE_C_IDENTIFIER "${varname}" varname)

    set(resource_c_file "${CMAKE_CURRENT_BINARY_DIR}/${file}.c")
    list(APPEND resource_c_files "${resource_c_file}")

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

add_library(resources OBJECT ${resource_c_files})