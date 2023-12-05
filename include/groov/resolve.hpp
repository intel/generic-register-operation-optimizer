#pragma once

#include <stdx/concepts.hpp>
#include <stdx/type_traits.hpp>

#include <type_traits>
#include <utility>

namespace groov {
template <typename T> struct is_path : std::false_type {};

struct invalid_t {};
struct too_long_t : invalid_t {};
struct mismatch_t : invalid_t {};
struct ambiguous_t : invalid_t {};

template <typename T, stdx::has_trait<is_path> Path, typename... Args>
constexpr auto resolve(T const &t, Path const &p, Args const &...args)
    -> decltype(auto) {
    return t.resolve(p, args...);
}

template <typename... Ts>
using resolve_t = decltype(resolve(std::declval<Ts>()...));

template <typename T, stdx::has_trait<is_path> Path, typename... Args>
constexpr auto checked_resolve([[maybe_unused]] T const &t,
                               [[maybe_unused]] Path const &p,
                               Args const &...args) {
    using R = resolve_t<T, Path, Args...>;
    if constexpr (std::is_same_v<R, too_long_t>) {
        static_assert(
            stdx::always_false_v<T, Path, Args...>,
            "Attempting to access value with a path that is too long");
    } else if constexpr (std::is_same_v<R, mismatch_t>) {
        static_assert(stdx::always_false_v<T, Path, Args...>,
                      "Attempting to access value with a mismatched path");
    } else if constexpr (std::is_same_v<R, ambiguous_t>) {
        static_assert(stdx::always_false_v<T, Path, Args...>,
                      "Attempting to access value with an ambiguous path");
    } else {
        static_assert(not stdx::derived_from<R, invalid_t>,
                      "Attempting to access value with an invalid path");
        return t.resolve(p, args...);
    }
}

template <typename... Args>
concept can_resolve = not(stdx::derived_from<resolve_t<Args...>, invalid_t>);

template <typename... Args>
constexpr static bool is_resolvable_v =
    can_resolve<std::remove_cvref_t<Args>...>;

template <typename... Ts>
using is_resolvable_t = std::bool_constant<is_resolvable_v<Ts...>>;

template <typename... Args> struct resolves_q {
    template <typename T> using fn = is_resolvable_t<T, Args...>;
};

template <typename T, typename... Args> struct resolve_result_q {
    template <stdx::has_trait<is_path> P> using fn = resolve_t<T, P, Args...>;
};
} // namespace groov
