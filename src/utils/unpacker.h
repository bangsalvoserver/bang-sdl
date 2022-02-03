#ifndef __UNPACKER_H__
#define __UNPACKER_H__

#include "resource.h"

#include <map>
#include <vector>
#include <string>
#include <cstring>
#include <iostream>
#include <fstream>

template<typename T>
T read_data(const char **pos) {
    T buf;
    std::memcpy(&buf, *pos, sizeof(buf));
    *pos += sizeof(buf);
    return buf;
}

template<typename T>
T read_data(std::ifstream &ifs) {
    T buf;
    ifs.read(reinterpret_cast<char *>(&buf), sizeof(T));
    return buf;
}

template<typename Resource>
class unpacker {};

template<std::convertible_to<resource_view> T> class unpacker<T> {
public:
    unpacker(resource_view data) {
        struct packed_data {
            std::string name;
            uint64_t pos;
            uint64_t size;
        };

        std::vector<packed_data> items;

        const char *pos = data.data;
        uint64_t nitems = read_data<uint64_t>(&pos);

        for (; nitems!=0; --nitems) {
            packed_data item;

            uint64_t len = read_data<uint64_t>(&pos);
            item.name = std::string(pos, len);
            pos += len;

            item.pos = read_data<uint64_t>(&pos);
            item.size = read_data<uint64_t>(&pos);

            items.push_back(item);
        }

        for (const auto &item : items) {
            m_data.emplace(item.name, resource_view{pos + item.pos, item.size});
        }
    }

    const resource_view &operator[](std::string_view key) const {
        auto it = m_data.find(key);
        if (it == m_data.end()) {
            std::string str_err = "Impossibile trovare risorsa: ";
            str_err += key;
            throw std::out_of_range(str_err);
        }
        return it->second;
    }

private:
    std::map<std::string, resource_view, std::less<>> m_data;
};

template<> class unpacker<std::ifstream> {
private:
    struct data_offset {
        uint64_t pos;
        uint64_t size;
    };
    std::map<std::string, data_offset, std::less<>> m_data;
    std::ifstream &m_stream;

public:
    explicit unpacker(std::ifstream &ifs) : m_stream(ifs) {
        struct packed_data {
            std::string name;
            uint64_t pos;
            uint64_t size;
        };

        std::vector<packed_data> items;

        uint64_t nitems = read_data<uint64_t>(ifs);
        for (; nitems!=0; --nitems) {
            packed_data item;

            uint64_t len = read_data<uint64_t>(ifs);
            item.name.assign(len, '\0');
            ifs.read(item.name.data(), len);

            item.pos = read_data<uint64_t>(ifs);
            item.size = read_data<uint64_t>(ifs);

            items.push_back(item);
        }

        for (const auto &item : items) {
            m_data.try_emplace(item.name, (uint64_t)ifs.tellg() + item.pos, item.size);
        }
    }

    resource operator[](std::string_view key) const {
        auto it = m_data.find(key);
        if (it == m_data.end()) {
            std::string str_err = "Impossibile trovare risorsa: ";
            str_err += key;
            throw std::out_of_range(str_err);
        }
        m_stream.seekg(it->second.pos, std::ios::beg);
        resource ret;
        ret.assign(it->second.size, '\0');
        m_stream.read(ret.data(), ret.size());
        return ret;
    }
};

template<typename Resource> unpacker(Resource &&) -> unpacker<std::remove_reference_t<Resource>>;

#endif