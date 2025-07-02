#pragma once

#include <groov/attach_value.hpp>
#include <groov/resolve.hpp>

#include <stdx/compiler.hpp>
#include <stdx/ct_string.hpp>
#include <stdx/type_traits.hpp>

#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/list.hpp>

#include <concepts>
#include <type_traits>

namespace groov {
template <stdx::ct_string... Parts> struct path {
    template <typename... Vs> constexpr auto operator()(Vs const &...vs) const {
        return attach_value(*this, vs...);
    }

    template <typename T>
    // NOLINTNEXTLINE(misc-unconventional-assign-operator)
    constexpr auto operator=(T const &value) {
        return (*this)(value);
    }

    template <pathlike P> constexpr static auto resolve(P) {
        constexpr auto len = sizeof...(Parts);
        constexpr auto other_len = boost::mp11::mp_size<P>::value;
        if constexpr (len >= other_len) {
            constexpr auto valid =
                std::same_as<boost::mp11::mp_take_c<path, other_len>,
                             boost::mp11::mp_take_c<P, other_len>>;
            if constexpr (valid) {
                return boost::mp11::mp_drop_c<path, other_len>{};
            } else {
                return mismatch_t{};
            }
        } else {
            return too_long_t{};
        }
    }

    constexpr static auto without_root()
        requires(sizeof...(Parts) > 0)
    {
        return boost::mp11::mp_drop_c<path, 1>{};
    }

    constexpr static auto empty = std::bool_constant<sizeof...(Parts) == 0>{};

  private:
    friend constexpr auto operator==(path, path) -> bool = default;

    template <pathlike P>
        requires(not valued<P>)
    friend constexpr auto operator/(path, P p) {
        return []<stdx::ct_string... As>(path<As...>) -> path<Parts..., As...> {
            return {};
        }(p);
    }
};

namespace literals {
namespace detail {
template <stdx::ct_string S, stdx::ct_string... Parts>
CONSTEVAL auto make_path() -> pathlike auto {
    constexpr auto p = stdx::split<S, '.'>();
    if constexpr (p.second.empty()) {
        return path<Parts..., p.first>{};
    } else {
        return make_path<p.second, Parts..., p.first>();
    }
}
} // namespace detail

#if __clang__ && __clang_major__ <= 14
template <class T, T... chars>
CONSTEVAL_UDL auto operator""_g() -> pathlike auto {
    constexpr auto s = stdx::ct_string<sizeof...(chars) + 1U>{{chars..., 0}};
    return detail::make_path<s>();
}

template <class T, T... chars>
CONSTEVAL_UDL auto operator""_r() -> pathlike auto {
    constexpr auto s = stdx::ct_string<sizeof...(chars) + 1U>{{chars..., 0}};
    return detail::make_path<s>();
}

template <class T, T... chars>
CONSTEVAL_UDL auto operator""_f() -> pathlike auto {
    constexpr auto s = stdx::ct_string<sizeof...(chars) + 1U>{{chars..., 0}};
    return detail::make_path<s>();
}
#else
template <stdx::ct_string S>
CONSTEVAL_UDL auto operator""_g() -> pathlike auto {
    return detail::make_path<S>();
}

template <stdx::ct_string S>
CONSTEVAL_UDL auto operator""_r() -> pathlike auto {
    return detail::make_path<S>();
}

template <stdx::ct_string S>
CONSTEVAL_UDL auto operator""_f() -> pathlike auto {
    return detail::make_path<S>();
}
#endif

} // namespace literals
} // namespace groov
