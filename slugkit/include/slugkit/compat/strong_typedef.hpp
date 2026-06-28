#pragma once

/// @file slugkit/compat/strong_typedef.hpp
/// @brief Compat shim for userver::utils::StrongTypedef.
///
/// When SLUGKIT_USE_USERVER is defined the names alias the userver
/// implementation so the userver service build is unchanged. Otherwise a
/// standalone subset is provided covering the surface the generator uses:
/// GetUnderlying()/UnderlyingType, constexpr ctors (including {ptr,len}),
/// same-type and transparent-with-underlying comparisons, and the
/// IsStrongTypedef trait. The typedefs are never hashed nor fmt-formatted
/// directly (callers always extract GetUnderlying() first), so neither is
/// provided here.

#ifdef SLUGKIT_USE_USERVER

#include <userver/utils/strong_typedef.hpp>

namespace slugkit::compat {
using userver::utils::IsStrongTypedef;
using userver::utils::StrongTypedef;
using userver::utils::StrongTypedefOps;
}  // namespace slugkit::compat

#else

#include <fmt/format.h>

#include <type_traits>
#include <utility>

namespace slugkit::compat {

enum class StrongTypedefOps {
    kNoCompare = 0,
    kCompareStrong = 1 << 0,
    kCompareTransparentOnly = 1 << 1,
    kCompareTransparent = kCompareStrong | kCompareTransparentOnly,
};

constexpr bool operator&(StrongTypedefOps a, StrongTypedefOps b) noexcept {
    return (static_cast<int>(a) & static_cast<int>(b)) != 0;
}

template <typename Tag, typename T, StrongTypedefOps Ops = StrongTypedefOps::kCompareStrong, typename Enable = void>
class StrongTypedef;

template <typename T>
struct IsStrongTypedef : std::false_type {};

template <typename Tag, typename T, StrongTypedefOps Ops, typename Enable>
struct IsStrongTypedef<StrongTypedef<Tag, T, Ops, Enable>> : std::true_type {};

template <typename Tag, typename T, StrongTypedefOps Ops, typename Enable>
class StrongTypedef {
public:
    using UnderlyingType = T;
    using TagType = Tag;
    static constexpr StrongTypedefOps kOps = Ops;
    static constexpr bool kTransparent = (Ops & StrongTypedefOps::kCompareTransparentOnly);

    constexpr StrongTypedef() = default;
    constexpr StrongTypedef(const StrongTypedef&) = default;
    constexpr StrongTypedef(StrongTypedef&&) noexcept = default;
    StrongTypedef& operator=(const StrongTypedef&) = default;
    StrongTypedef& operator=(StrongTypedef&&) noexcept = default;

    explicit constexpr StrongTypedef(const T& v) : value_(v) {}
    explicit constexpr StrongTypedef(T&& v) : value_(std::move(v)) {}

    // Single-arg forwarding ctor for any type the underlying is constructible from, e.g.
    // Tag{std::string_view} -> std::string{std::string_view}. Constrained so it never shadows the
    // copy/move ctors (U == StrongTypedef) nor the const T&/T&& ctors above (U == T).
    template <
        typename U,
        std::enable_if_t<
            !std::is_same_v<std::decay_t<U>, StrongTypedef> && !std::is_same_v<std::decay_t<U>, T> &&
                std::is_constructible_v<T, U&&>,
            int> = 0>
    explicit constexpr StrongTypedef(U&& u) : value_(std::forward<U>(u)) {}

    // Multi-arg forwarding ctor: e.g. LanguageCode{ptr, len} -> std::string(ptr, len).
    // Constrained to >=2 args so it never shadows copy/move/single-value ctors above.
    template <typename A, typename B, typename... Rest>
    explicit constexpr StrongTypedef(A&& a, B&& b, Rest&&... rest)
        : value_(std::forward<A>(a), std::forward<B>(b), std::forward<Rest>(rest)...) {}

    constexpr const T& GetUnderlying() const& noexcept { return value_; }
    constexpr T& GetUnderlying() & noexcept { return value_; }
    constexpr T&& GetUnderlying() && noexcept { return std::move(value_); }

    explicit constexpr operator const T&() const noexcept { return value_; }

    // Same-type comparisons (used for std::set ordering and equality).
    friend constexpr bool operator==(const StrongTypedef& a, const StrongTypedef& b) { return a.value_ == b.value_; }
    friend constexpr bool operator!=(const StrongTypedef& a, const StrongTypedef& b) { return a.value_ != b.value_; }
    friend constexpr bool operator<(const StrongTypedef& a, const StrongTypedef& b) { return a.value_ < b.value_; }
    friend constexpr bool operator<=(const StrongTypedef& a, const StrongTypedef& b) { return a.value_ <= b.value_; }
    friend constexpr bool operator>(const StrongTypedef& a, const StrongTypedef& b) { return a.value_ > b.value_; }
    friend constexpr bool operator>=(const StrongTypedef& a, const StrongTypedef& b) { return a.value_ >= b.value_; }

    // Transparent comparisons against any value comparable with the underlying type (heterogeneous
    // lookup / comparing against raw values, e.g. Tag{std::string} vs std::string_view). Templated
    // and SFINAE-constrained so they only participate when value_ <op> rhs is well-formed and rhs
    // is not itself a StrongTypedef (the same-type overloads above handle that). Harmless for
    // non-transparent tags since the underlying type differs from the typedef itself.
    template <typename U, std::enable_if_t<!std::is_same_v<std::decay_t<U>, StrongTypedef>, int> = 0>
    friend constexpr auto operator==(const StrongTypedef& a, const U& b) -> decltype(a.GetUnderlying() == b, bool{}) {
        return a.GetUnderlying() == b;
    }
    template <typename U, std::enable_if_t<!std::is_same_v<std::decay_t<U>, StrongTypedef>, int> = 0>
    friend constexpr auto operator==(const U& a, const StrongTypedef& b) -> decltype(a == b.GetUnderlying(), bool{}) {
        return a == b.GetUnderlying();
    }
    template <typename U, std::enable_if_t<!std::is_same_v<std::decay_t<U>, StrongTypedef>, int> = 0>
    friend constexpr auto operator!=(const StrongTypedef& a, const U& b) -> decltype(a.GetUnderlying() != b, bool{}) {
        return a.GetUnderlying() != b;
    }
    template <typename U, std::enable_if_t<!std::is_same_v<std::decay_t<U>, StrongTypedef>, int> = 0>
    friend constexpr auto operator!=(const U& a, const StrongTypedef& b) -> decltype(a != b.GetUnderlying(), bool{}) {
        return a != b.GetUnderlying();
    }
    template <typename U, std::enable_if_t<!std::is_same_v<std::decay_t<U>, StrongTypedef>, int> = 0>
    friend constexpr auto operator<(const StrongTypedef& a, const U& b) -> decltype(a.GetUnderlying() < b, bool{}) {
        return a.GetUnderlying() < b;
    }
    template <typename U, std::enable_if_t<!std::is_same_v<std::decay_t<U>, StrongTypedef>, int> = 0>
    friend constexpr auto operator<(const U& a, const StrongTypedef& b) -> decltype(a < b.GetUnderlying(), bool{}) {
        return a < b.GetUnderlying();
    }

private:
    T value_{};
};

}  // namespace slugkit::compat

// fmt formatter that forwards to the underlying type's formatter, mirroring userver's StrongTypedef
// formatting so core code can `fmt::format("{}", typedef)` without extracting GetUnderlying() first.
template <typename Tag, typename T, slugkit::compat::StrongTypedefOps Ops, typename Enable>
struct fmt::formatter<slugkit::compat::StrongTypedef<Tag, T, Ops, Enable>, char> : fmt::formatter<T, char> {
    template <typename FormatContext>
    auto format(const slugkit::compat::StrongTypedef<Tag, T, Ops, Enable>& value, FormatContext& ctx) const {
        return fmt::formatter<T, char>::format(value.GetUnderlying(), ctx);
    }
};

#endif  // SLUGKIT_USE_USERVER
