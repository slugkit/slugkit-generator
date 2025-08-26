#include <userver/utest/utest.hpp>

#include <slugkit/utils/text.hpp>

namespace slugkit::utils::text {

UTEST(MaskedCase, Basic) {
    // bits go from lsb to msb, chars from left to right
    EXPECT_EQ(MixedCase("Hello", kEnUsLocale, CaseMask{0b01010}), "hElLo");
    EXPECT_EQ(MixedCase("Hello", kEnUsLocale, CaseMask{0b10001}), "HellO");
    EXPECT_EQ(MixedCase("Hello", kEnUsLocale, CaseMask{0b11110}), "hELLO");
    EXPECT_EQ(MixedCase("Hello", kEnUsLocale, CaseMask{0b11111}), "HELLO");
}

}  // namespace slugkit::utils::text
