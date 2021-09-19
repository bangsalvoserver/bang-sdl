# Arguments: resource_file_name source_file_name variable_name

file(READ ${resource_file_name} hex_content HEX)
file(SIZE ${resource_file_name} content_size)

string(REPEAT "[0-9a-f]" 32 column_pattern)
string(REGEX REPLACE "(${column_pattern})" "\\1\n" content "${hex_content}")

string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1, " content "${content}")

string(REGEX REPLACE ", \n?$" "" content "${content}")

set(array_definition "const char ${variable_name}[${content_size}] = {\n${content}\n};")
set(length_definition "const int ${variable_name}_length = ${content_size};")

set(source "// Auto generated file.\n${array_definition}\n${length_definition}\n")

file(WRITE "${source_file_name}" "${source}")