#include <slugkit/utils/text.hpp>

#include <utf8proc.h>

#include <cctype>
#include <cstring>

namespace slugkit::utils::text {

const std::string kEnUsLocale{"en_US.UTF-8"};

namespace {

// Apply a per-codepoint case mapping (utf8proc_tolower/utf8proc_toupper) over the whole string.
// The string is decoded from UTF-8 to codepoints, each is mapped, then re-encoded back to UTF-8.
template <typename Map>
auto MapCodepoints(std::string_view str, Map&& map) -> std::string {
    std::string result;
    result.reserve(str.size());
    const auto* data = reinterpret_cast<const utf8proc_uint8_t*>(str.data());
    auto remaining = static_cast<utf8proc_ssize_t>(str.size());
    utf8proc_uint8_t buffer[4];
    while (remaining > 0) {
        utf8proc_int32_t cp = -1;
        auto consumed = utf8proc_iterate(data, remaining, &cp);
        if (consumed < 1 || cp < 0) {
            // Invalid byte: pass it through verbatim so we never corrupt non-UTF-8 input.
            result.push_back(static_cast<char>(*data));
            ++data;
            --remaining;
            continue;
        }
        utf8proc_int32_t mapped = map(cp);
        auto written = utf8proc_encode_char(mapped, buffer);
        result.append(reinterpret_cast<const char*>(buffer), static_cast<std::size_t>(written));
        data += consumed;
        remaining -= consumed;
    }
    return result;
}

}  // namespace

// TODO: locale-tailored casing (e.g. Turkish dotless i) -- see mobile-ffi-proposal.md
// utf8proc applies the locale-independent Unicode default case mappings; the `locale` argument is
// accepted but currently ignored.

std::string ToLower(std::string_view str, const std::string& /*locale*/) {
    return MapCodepoints(str, [](utf8proc_int32_t cp) { return utf8proc_tolower(cp); });
}

std::string ToUpper(std::string_view str, const std::string& /*locale*/) {
    return MapCodepoints(str, [](utf8proc_int32_t cp) { return utf8proc_toupper(cp); });
}

std::string Capitalize(std::string_view str, const std::string& /*locale*/) {
    // Title-case a single token: first codepoint upper-cased, the remainder lower-cased. The
    // generator only ever feeds single tokens (selector kinds, dictionary words) through this.
    std::string result;
    result.reserve(str.size());
    const auto* data = reinterpret_cast<const utf8proc_uint8_t*>(str.data());
    auto remaining = static_cast<utf8proc_ssize_t>(str.size());
    bool first = true;
    utf8proc_uint8_t buffer[4];
    while (remaining > 0) {
        utf8proc_int32_t cp = -1;
        auto consumed = utf8proc_iterate(data, remaining, &cp);
        if (consumed < 1 || cp < 0) {
            result.push_back(static_cast<char>(*data));
            ++data;
            --remaining;
            continue;
        }
        utf8proc_int32_t mapped = first ? utf8proc_toupper(cp) : utf8proc_tolower(cp);
        auto written = utf8proc_encode_char(mapped, buffer);
        result.append(reinterpret_cast<const char*>(buffer), static_cast<std::size_t>(written));
        first = false;
        data += consumed;
        remaining -= consumed;
    }
    return result;
}

std::string FirstUpper(std::string_view str, const std::string& /*locale*/) {
    // Upper-case only the first codepoint, leaving the rest of the string unchanged.
    if (str.empty()) {
        return std::string{};
    }
    const auto* data = reinterpret_cast<const utf8proc_uint8_t*>(str.data());
    utf8proc_int32_t cp = -1;
    auto consumed = utf8proc_iterate(data, static_cast<utf8proc_ssize_t>(str.size()), &cp);
    if (consumed < 1 || cp < 0) {
        return std::string{str};
    }
    utf8proc_uint8_t buffer[4];
    auto written = utf8proc_encode_char(utf8proc_toupper(cp), buffer);
    std::string result;
    result.reserve(str.size());
    result.append(reinterpret_cast<const char*>(buffer), static_cast<std::size_t>(written));
    result.append(str.substr(static_cast<std::size_t>(consumed)));
    return result;
}

std::string MixedCase(std::string_view str, const std::string& /*locale*/, CaseMask original_mask) {
    // Byte-indexed case mask: bit i controls byte i (1 -> upper, 0 -> lower). This matches the
    // byte-wise model of CountCaseToggleable/ExpandCaseMask. Only ASCII letters carry case, so we
    // map those with the locale-independent ASCII rules and pass every other byte (digits,
    // separators, and the bytes of multi-byte UTF-8 sequences) through unchanged.
    auto mask = original_mask.GetUnderlying();
    std::string result;
    result.reserve(str.size());
    for (std::size_t i = 0; i < str.size(); ++i) {
        auto byte = static_cast<unsigned char>(str[i]);
        bool upper = ((mask >> i) & 1ULL) != 0;
        if (byte < 0x80 && std::isalpha(byte) != 0) {
            result.push_back(static_cast<char>(
                upper ? std::toupper(byte) : std::tolower(byte)
            ));
        } else {
            result.push_back(static_cast<char>(byte));
        }
    }
    return result;
}

auto CountCaseToggleable(std::string_view str) noexcept -> std::size_t {
    std::size_t count = 0;
    for (unsigned char c : str) {
        if (std::isalpha(c) != 0) {
            ++count;
        }
    }
    return count;
}

auto ExpandCaseMask(std::string_view str, std::uint64_t compact) noexcept -> CaseMask {
    // Lay the low bits of `compact` onto the toggleable (letter) byte positions of `str`, in
    // order, so MixedCase reproduces exactly one distinct cased form per `compact` value. Byte
    // positions are limited to the 64 the mask can address; words are far shorter in practice.
    std::uint64_t mask = 0;
    for (std::size_t j = 0; j < str.size() && j < 64; ++j) {
        if (std::isalpha(static_cast<unsigned char>(str[j])) != 0) {
            if ((compact & 1ULL) != 0) {
                mask |= (std::uint64_t{1} << j);
            }
            compact >>= 1;
        }
    }
    return CaseMask{mask};
}

}  // namespace slugkit::utils::text
