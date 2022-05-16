#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <filesystem>

struct file_table_item {
    std::filesystem::path filename;
    std::string name;
    uint64_t begin;
    uint64_t size;
};

void print_usage() {
    std::cout << "Usage: pack [-q] [-D root_path] output_file.pak input_files...\n";
}

int main(int argc, char **argv) {
    if (argc <= 1) {
        print_usage();
        return 1;
    }

    bool quiet = false;
    std::filesystem::path output_file, root_path;
    std::vector<std::filesystem::path> input_files;

    int i=1;
    if (argv[i] == std::string_view("-q")) {
        quiet = true;
        ++i;
    }
    if (argv[i] == std::string_view("-D")) {
        ++i;
        root_path = argv[i];
        ++i;
    } else {
        root_path = std::filesystem::path(argv[0]).parent_path();
    }
    if (i >= argc) {
        print_usage();
        return 1;
    }
    output_file = argv[i];
    for (++i; i < argc; ++i) {
        std::filesystem::path f = argv[i];
        if (std::filesystem::exists(f)) {
            input_files.push_back(f);
        } else {
            std::cout << "invalid input file: " << f.string() << '\n';
            return 1;
        }
    }

    std::vector<file_table_item> items;
    std::vector<char> data;

    std::filesystem::path in_dir(argv[1]);

    for (const auto &path : input_files) {
        file_table_item item;
        item.filename = path;
        item.name = std::filesystem::relative(path, root_path).replace_extension().string();
#ifdef _WIN32
        std::ranges::replace(item.name, '\\', '/');
#endif

        if (!quiet) {
            std::cout << item.name << '\n';
        }

        if (auto key = std::ranges::find(items, item.name, &file_table_item::name); key != items.end()) {
            if (key->filename == item.filename) {
                continue;
            } else {
                std::cerr << "duplicate file key: " << item.name << '\n';
                return 1;
            }
        }

        item.begin = data.size();
        std::ifstream ifs(path, std::ifstream::in | std::ifstream::ate | std::ifstream::binary);
        item.size = ifs.tellg();
        ifs.seekg(std::ios::beg);
        std::copy(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>(), std::back_inserter(data));

        items.push_back(item);
    }

    std::ofstream ofs(output_file, std::ofstream::out | std::ofstream::binary);

    uint64_t nitems = items.size();
    ofs.write(reinterpret_cast<char *>(&nitems), sizeof(nitems));

    for (const auto &item : items) {
        uint64_t len = item.name.size();
        ofs.write(reinterpret_cast<char *>(&len), sizeof(len));
        ofs.write(item.name.data(), item.name.size());
        ofs.write(reinterpret_cast<const char *>(&item.begin), sizeof(item.begin));
        ofs.write(reinterpret_cast<const char *>(&item.size), sizeof(item.size));
    }

    std::copy(std::begin(data), std::end(data), std::ostreambuf_iterator<char>(ofs));

    return 0;
}