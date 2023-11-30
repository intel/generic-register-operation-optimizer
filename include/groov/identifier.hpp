#pragma once

#include <stdx/compiler.hpp>
#include <stdx/ct_string.hpp>
#include <stdx/tuple.hpp>
#include <stdx/type_traits.hpp>

#include <boost/mp11/algorithm.hpp>

#include <algorithm>
#include <concepts>
#include <type_traits>

namespace groov {
template <typename T>
concept located_value = requires(T &t) {
    typename T::path_t;
    typename T::value_t;
    { t.value } -> std::same_as<typename T::value_t &>;
};

template <typename Path, typename... Values> struct with_values_t;

template <stdx::ct_string... Parts> struct path {
    template <typename T>
    // NOLINTNEXTLINE(cppcoreguidelines-c-copy-assignment-signature)
    constexpr auto operator=(T const &value) -> with_values_t<path, T> {
        return {value};
    }

    template <located_value... Vs>
        requires(sizeof...(Vs) > 0)
    constexpr auto operator()(Vs const &...vs) -> with_values_t<path, Vs...> {
        return {vs...};
    }
};

template <stdx::ct_string... As, stdx::ct_string... Bs>
constexpr auto operator/(path<As...>, path<Bs...>) -> path<As..., Bs...> {
    return {};
}

namespace detail {
template <typename, typename> struct drill_down;
template <stdx::ct_string... As, stdx::ct_string... Bs>
struct drill_down<path<As...>, path<Bs...>> {
    constexpr static auto len = std::min(sizeof...(As), sizeof...(Bs));
    using leftover_lhs = boost::mp11::mp_drop_c<path<As...>, len>;
    using leftover_rhs = boost::mp11::mp_drop_c<path<Bs...>, len>;

    constexpr static auto valid =
        std::is_same_v<boost::mp11::mp_take_c<path<As...>, len>,
                       boost::mp11::mp_take_c<path<Bs...>, len>>;
};
} // namespace detail

template <stdx::ct_string... Parts, typename Value>
struct with_values_t<path<Parts...>, Value> {
    using path_t = path<Parts...>;
    using value_t = Value;
    value_t value;

    template <typename P>
    using resolvable_t =
        std::bool_constant<detail::drill_down<P, path<Parts...>>::valid>;

    template <stdx::ct_string... Ps>
    constexpr auto operator[](path<Ps...>) const {
        using T = detail::drill_down<path<Ps...>, path<Parts...>>;
        static_assert(T::valid, "Attempting to access with an invalid path");
        static_assert(boost::mp11::mp_empty<typename T::leftover_lhs>::value,
                      "Attempting to access with a too-long path");
        if constexpr (boost::mp11::mp_empty<typename T::leftover_rhs>::value) {
            return value;
        } else {
            return with_values_t<typename T::leftover_rhs, Value>{value};
        }
    }
};

namespace detail {
template <typename P> struct resolvable {
    template <typename T> using fn = typename T::template resolvable_t<P>;
};
} // namespace detail

template <stdx::ct_string... Parts, located_value... Values>
struct with_values_t<path<Parts...>, Values...> {
    using path_t = path<Parts...>;
    using value_t = stdx::tuple<Values...>;
    value_t value;

    template <typename P>
    using resolvable_t = std::bool_constant<
        detail::drill_down<P, path<Parts...>>::valid and
        (... or
         detail::resolvable<typename detail::drill_down<
             P, path<Parts...>>::leftover_lhs>::template fn<Values>::value)>;

    template <stdx::ct_string... Ps>
    constexpr auto operator[](path<Ps...>) const {
        using T = detail::drill_down<path<Ps...>, path<Parts...>>;
        static_assert(T::valid, "Attempting to access with an invalid path");
        if constexpr (boost::mp11::mp_empty<typename T::leftover_lhs>::value) {
            return with_values_t<typename T::leftover_rhs, Values...>{value};
        } else {
            using valid_indices = boost::mp11::mp_transform_q<
                detail::resolvable<typename T::leftover_lhs>,
                stdx::type_list<Values...>>;
            using index = boost::mp11::mp_find<valid_indices, std::true_type>;
            static_assert(index::value !=
                              boost::mp11::mp_size<valid_indices>::value,
                          "Attempting to access with an invalid path");
            return stdx::get<index::value>(value)[typename T::leftover_lhs{}];
        }
    }
};

namespace literals {
namespace detail {
template <stdx::ct_string S, stdx::ct_string... Parts>
CONSTEVAL auto make_path() {
    constexpr auto p = stdx::split<S, '.'>();
    if constexpr (p.second.empty()) {
        return path<Parts..., p.first>{};
    } else {
        return make_path<p.second, Parts..., p.first>();
    }
}
} // namespace detail

template <class T, T... chars> CONSTEVAL auto operator""_g() {
    constexpr auto s = stdx::ct_string<sizeof...(chars) + 1U>{{chars..., 0}};
    return detail::make_path<s>();
}

template <class T, T... chars> CONSTEVAL auto operator""_r() {
    constexpr auto s = stdx::ct_string<sizeof...(chars) + 1U>{{chars..., 0}};
    return detail::make_path<s>();
}

template <class T, T... chars> CONSTEVAL auto operator""_f() {
    constexpr auto s = stdx::ct_string<sizeof...(chars) + 1U>{{chars..., 0}};
    return detail::make_path<s>();
}
} // namespace literals
} // namespace groov
