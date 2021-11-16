#ifndef __UNPACKER_H__
#define __UNPACKER_H__

#include "resource.h"

#include <map>
#include <vector>
#include <string>
#include <cstring>
#include <iostream>

template<typename T>
T read_data(const char **pos) {
    T buf;
    std::memcpy(&buf, *pos, sizeof(buf));
    *pos += sizeof(buf);
    return buf;
}

class unpacker {
public:
    unpacker(resource_view data) {
        struct packed_data {
            std::string name;
            size_t pos;
            size_t size;
        };

        std::vector<packed_data> items;

        const char *pos = data.data;
        size_t nitems = read_data<size_t>(&pos);

        for (; nitems!=0; --nitems) {
            packed_data item;

            size_t len = read_data<size_t>(&pos);
            item.name = std::string(pos, len);
            pos += len;

            item.pos = read_data<size_t>(&pos);
            item.size = read_data<size_t>(&pos);

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

#endif