#pragma once

#include <boost/functional/hash.hpp>
#include <cstdint>

namespace slugkit::generator {

inline std::size_t StrHash(const char* str, std::size_t len) {
    auto seed = len;
    boost::hash_range(seed, str, str + len);
    return seed;
}

}  // namespace slugkit::generator
