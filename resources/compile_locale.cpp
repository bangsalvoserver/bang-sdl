#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <map>

int main(int argc, char **argv) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " input_file output_file identifier_name\n";
        return 1;
    }

    std::string input_file = argv[1];
    std::string output_file = argv[2];
    std::string identifier_name = argv[3];

    std::map<std::string, std::string> strings;

    std::ifstream ifs(input_file);
    if (ifs.fail()) {
        std::cerr << "Could not open " << input_file << " for input\n";
        return 1;
    }

    std::string line;
    while (std::getline(ifs, line)) {
        if (line.empty()) continue;
        size_t space_pos = line.find_first_of(" \t");
        size_t not_space_pos = line.find_first_not_of(" \t", space_pos);
        std::string key = line.substr(0, space_pos);
        if (strings.find(key) != strings.end()) {
            std::cerr << "Duplicated string " << key << '\n';
            return 1;
        }
        strings.emplace(std::move(key), line.substr(not_space_pos));
    }

    std::ofstream ofs(output_file);
    if (ofs.fail()) {
        std::cerr << "Could not open " << output_file << " for output\n";
        return 1;
    }

    ofs << "// Auto generated file.\n"
        "#include <string>\n"
        "#include \"utils/static_map.h\"\n\n"
        "constexpr auto " << identifier_name << " = util::static_map<std::string_view, std::string_view>({\n";
    
    for (const auto &[key, value] : strings) {
        ofs << "\t{" << std::quoted(key) << ", " << std::quoted(value) << "},\n";
    }

    ofs << "});\n";

    return 0;
}