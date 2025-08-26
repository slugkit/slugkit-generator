#include <slugkit/utils/roman.hpp>

#include <slugkit/utils/text.hpp>

#include <userver/utest/utest.hpp>

namespace slugkit::utils::roman {

UTEST(Roman, WholeRange) {
    for (int i = 1; i <= 3999; ++i) {
        EXPECT_EQ(ParseRoman(ToRoman(i)), i);
    }
}

UTEST(Roman, Lowercase) {
    for (int i = 1; i <= 3999; ++i) {
        auto roman = ToRoman(i, true);
        EXPECT_EQ(ParseRoman(roman), i);
        auto roman_upper = text::ToUpper(roman, text::kEnUsLocale);
        EXPECT_EQ(roman_upper, ToRoman(i));
    }
}

UTEST(Roman, Invalid) {
    EXPECT_THROW(ParseRoman("XiX"), std::invalid_argument);
    EXPECT_THROW(ParseRoman("IIII"), std::invalid_argument);
    EXPECT_THROW(ParseRoman("IVI"), std::invalid_argument);
    EXPECT_THROW(ParseRoman("IXC"), std::invalid_argument);
    EXPECT_THROW(ParseRoman("XIXC"), std::invalid_argument);
    EXPECT_THROW(ParseRoman("XIXC"), std::invalid_argument);
}

}  // namespace slugkit::utils::roman
