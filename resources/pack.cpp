#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>

struct file_table_item {
    std::string name;
    size_t begin;
    size_t size;
};

int main(int argc, char **argv) {
    if (argc < 3) {
        std::cout << "Usage: pack input_dir output_file.pak\n";
        return 1;
    }

    bool quiet = false;
    if (argc >= 4 && argv[3] == std::string_view("-q")) {
        quiet = true;
    }

    std::vector<file_table_item> items;
    std::vector<char> data;

    std::filesystem::path in_dir(argv[1]);

    for (const auto &path : std::filesystem::recursive_directory_iterator(in_dir)) {
        if (path.is_regular_file()) {
            file_table_item item;
            item.name = path.path().stem().string();

            if (!quiet) {
                std::cout << item.name << '\n';
            }

            item.begin = data.size();
            std::ifstream ifs(path.path(), std::ifstream::in | std::ifstream::ate | std::ifstream::binary);
            item.size = ifs.tellg();
            ifs.seekg(std::ios::beg);
            std::copy(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>(), std::back_inserter(data));
            items.push_back(item);
        }
    }

    std::ofstream ofs(argv[2], std::ofstream::out | std::ofstream::binary);

    size_t nitems = items.size();
    ofs.write(reinterpret_cast<char *>(&nitems), sizeof(nitems));

    for (const auto &item : items) {
        size_t len = item.name.size();
        ofs.write(reinterpret_cast<char *>(&len), sizeof(len));
        ofs.write(item.name.data(), item.name.size());
        ofs.write(reinterpret_cast<const char *>(&item.begin), sizeof(item.begin));
        ofs.write(reinterpret_cast<const char *>(&item.size), sizeof(item.size));
    }

    std::copy(std::begin(data), std::end(data), std::ostreambuf_iterator<char>(ofs));

    return 0;
}