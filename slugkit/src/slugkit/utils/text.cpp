#include <slugkit/utils/text.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/locale.hpp>
#include <boost/locale/conversion.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include <userver/engine/shared_mutex.hpp>

#include <cctype>

namespace slugkit::utils::text {

const std::string kEnUsLocale{"en_US.UTF-8"};

const std::locale& GetLocale(const std::string& name) {
    static userver::engine::SharedMutex m;
    using locales_map_t = std::unordered_map<std::string, std::locale>;
    static locales_map_t locales;
    {
        std::shared_lock read_lock(m);
        auto it = static_cast<const locales_map_t&>(locales).find(name);
        if (it != locales.cend()) {
            return it->second;
        }
    }

    boost::locale::generator gen;
    std::locale loc = gen(name);
    {
        std::unique_lock write_lock(m);
        return locales.emplace(name, std::move(loc)).first->second;
    }
}

std::string ToLower(std::string_view str, const std::string& locale) {
    return boost::locale::to_lower(str.data(), str.data() + str.size(), GetLocale(locale));
}

std::string ToUpper(std::string_view str, const std::string& locale) {
    return boost::locale::to_upper(str.data(), str.data() + str.size(), GetLocale(locale));
}

std::string Capitalize(std::string_view str, const std::string& locale) {
    return boost::locale::to_title(str.data(), str.data() + str.size(), GetLocale(locale));
}

std::string MixedCase(std::string_view str, const std::string& locale, CaseMask original_mask) {
    auto mask = original_mask.GetUnderlying();
    std::string result;
    result.reserve(str.size());
    auto loc = GetLocale(locale);
    auto it = str.begin();
    auto current_case = mask & 1;
    mask >>= 1;
    while (it != str.end()) {
        if (current_case == 0) {
            result.append(boost::locale::to_lower(it, it + 1, loc));
        } else {
            result.append(boost::locale::to_upper(it, it + 1, loc));
        }
        current_case = mask & 1;
        mask >>= 1;
        ++it;
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
