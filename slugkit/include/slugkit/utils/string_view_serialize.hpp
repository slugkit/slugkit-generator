#pragma once

#include <userver/formats/serialize/to.hpp>

#include <string>
#include <string_view>

namespace std {

template <typename Format>
Format Serialize(std::string_view str, userver::formats::serialize::To<Format>) {
    typename Format::Builder builder(std::string{str});
    return builder.ExtractValue();
}

}  // namespace std
