#include <slugkit/utils/numeric.hpp>

namespace numeric {

BigInt lcm(const BigInt& a, const BigInt& b) {
    const auto& a_value = a;
    const auto& b_value = b;
    return BigInt(a_value * b_value / gcd(a_value, b_value));
}

BigInt gcd(const BigInt& a, const BigInt& b) {
    const auto& a_value = a;
    const auto& b_value = b;
    return b_value == 0 ? a : BigInt(gcd(b_value, a_value % b_value));
}

}  // namespace numeric
