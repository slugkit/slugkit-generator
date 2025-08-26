#include <slugkit/utils/text.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/locale.hpp>
#include <boost/locale/conversion.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include <userver/engine/shared_mutex.hpp>

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

}  // namespace slugkit::utils::text
