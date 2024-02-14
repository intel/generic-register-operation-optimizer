#pragma once

#include <stdx/concepts.hpp>
#include <stdx/ct_string.hpp>
#include <stdx/type_traits.hpp>

#include <type_traits>
#include <utility>

namespace groov {
template <stdx::ct_string... Parts> struct path;

template <typename T>
concept pathlike = requires(T const &t) {
    {
        []<stdx::ct_string... Parts>(path<Parts...> const &) {}(t)
    } -> std::same_as<void>;
};

template <typename T>
concept valued = requires { typename T::value_t; };

template <typename T>
concept valued_pathlike = pathlike<T> and valued<T>;

namespace detail {
template <stdx::ct_string... Parts>
constexpr auto get_path(path<Parts...> const &) -> path<Parts...>;
}
template <pathlike T>
using get_path_t = decltype(detail::get_path(std::declval<T>()));

struct invalid_t {};
struct too_long_t : invalid_t {};
struct mismatch_t : invalid_t {};
struct ambiguous_t : invalid_t {};

template <typename T, pathlike Path, typename... Args>
constexpr auto resolve(T const &t, Path const &p, Args const &...args)
    -> decltype(auto) {
    return t.resolve(p, args...);
}

template <typename... Ts>
using resolve_t = decltype(resolve(std::declval<Ts>()...));

template <typename T, pathlike Path, typename... Args>
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
    template <pathlike P> using fn = resolve_t<T, P, Args...>;
};

template <stdx::ct_string P, stdx::ct_string... Ps>
constexpr auto root(path<P, Ps...> const &) {
    return P;
}

template <pathlike P> constexpr auto without_root(P const &p) {
    return p.without_root();
}
} // namespace groov
