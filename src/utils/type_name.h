#ifndef __TYPE_NAME_H__
#define __TYPE_NAME_H__

#include <string>
#include <typeinfo>

#ifdef __GNUG__
#include <cxxabi.h>
#include <memory>

namespace utils {

    inline std::string demangle(const char *name) {
        int status = -4;
        std::unique_ptr<char, void(*)(void*)> res {
            abi::__cxa_demangle(name, nullptr, nullptr, &status),
            std::free
        };
        return status == 0 ? res.get() : name;
    }

}

#else

namespace utils {

    inline std::string demangle(const char *name) {
        return name;
    }

}

#endif

#endif