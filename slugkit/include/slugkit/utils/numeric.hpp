#pragma once

#include <boost/multiprecision/cpp_int.hpp>

#include <userver/formats/parse/to.hpp>
#include <userver/formats/serialize/to.hpp>
#include <userver/utils/strong_typedef.hpp>

namespace numeric {

using BigInt = boost::multiprecision::cpp_int;

BigInt lcm(const BigInt& a, const BigInt& b);
BigInt gcd(const BigInt& a, const BigInt& b);

}  // namespace numeric

namespace boost::multiprecision {

template <typename Format>
auto Serialize(const cpp_int& value, userver::formats::serialize::To<Format>) -> Format {
    typename Format::Builder builder;
    builder = value.str();
    return builder.ExtractValue();
}

template <typename Value>
auto Parse(const Value& value, userver::formats::parse::To<cpp_int>) -> cpp_int {
    return cpp_int(value.template As<std::string>());
}

}  // namespace boost::multiprecision