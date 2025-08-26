#pragma once

#include <cstdint>

namespace slugkit::utils {

/// @brief Returns the next prime number after the given number.
/// @param number The number to get the next prime number after.
/// @return The next prime number after the given number.
std::uint64_t NextPrime(std::uint64_t number);

/// @brief Returns the previous prime number before the given number.
/// @param number The number to get the previous prime number before.
/// @return The previous prime number before the given number.
std::uint64_t PrevPrime(std::uint64_t number);

}  // namespace slugkit::utils
