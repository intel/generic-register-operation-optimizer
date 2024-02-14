#pragma once

#include <groov/resolve.hpp>

#include <stdx/compiler.hpp>
#include <stdx/concepts.hpp>
#include <stdx/ct_string.hpp>
#include <stdx/tuple.hpp>

#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/list.hpp>
#include <boost/mp11/set.hpp>

#include <concepts>
#include <type_traits>

namespace groov {
template <stdx::ct_string P, stdx::ct_string... Ps>
constexpr auto root(path<P, Ps...> const &) {
    return P;
}

template <pathlike P> constexpr auto without_root(P const &p) {
    return p.without_root();
}

template <pathlike Path, typename Value> struct value_path : Path {
    using path_t = Path;
    using value_t = Value;
    [[no_unique_address]] value_t value;

    template <pathlike P> constexpr auto resolve([[maybe_unused]] P p) const {
        constexpr auto len = boost::mp11::mp_size<Path>::value;
        constexpr auto other_len = boost::mp11::mp_size<P>::value;
        if constexpr (other_len > len) {
            using leftover_t = resolve_t<P, Path>;
            auto const leftover_path = groov::resolve(p, Path{});
            if constexpr (pathlike<leftover_t>) {
                if constexpr (stdx::is_specialization_of_v<value_t,
                                                           stdx::tuple>) {
                    using L = boost::mp11::mp_copy_if_q<value_t,
                                                        resolves_q<leftover_t>>;
                    if constexpr (boost::mp11::mp_empty<L>::value) {
                        return mismatch_t{};
                    } else if constexpr (boost::mp11::mp_size<L>::value > 1) {
                        return ambiguous_t{};
                    } else {
                        return groov::resolve(
                            stdx::get<boost::mp11::mp_front<L>>(value),
                            leftover_path);
                    }
                } else {
                    return too_long_t{};
                }
            } else {
                return leftover_path;
            }
        } else {
            using leftover_t = resolve_t<Path, P>;
            auto const leftover_path = groov::resolve(Path{}, p);
            if constexpr (pathlike<leftover_t>) {
                if constexpr (std::empty(leftover_path) and
                              not stdx::is_specialization_of_v<value_t,
                                                               stdx::tuple>) {
                    return value;
                } else {
                    return value_path<leftover_t, Value>{{}, value};
                }
            } else {
                return leftover_path;
            }
        }
    }

    template <pathlike P> constexpr auto operator[](P p) const {
        return checked_resolve(*this, p);
    }

    constexpr auto without_root() const {
        return value_path<decltype(groov::without_root(Path{})), Value>{
            {}, value};
    }

  private:
    friend constexpr auto operator==(value_path const &, value_path const &)
        -> bool = default;

    template <typename P>
        requires(stdx::is_value_specialization_of_v<P, groov::path>)
    friend constexpr auto operator/(P p, value_path const &v) {
        return value_path<decltype(p / Path{}), Value>{{}, v.value};
    }
};

template <stdx::ct_string... Parts> struct path {
    template <typename... Vs> constexpr auto operator()(Vs const &...vs) {
        if constexpr (sizeof...(Vs) > 1) {
            return value_path<path, stdx::tuple<Vs...>>{{}, {vs...}};
        } else {
            return value_path<path, Vs...>{{}, vs...};
        }
    }

    template <typename T>
    // NOLINTNEXTLINE(cppcoreguidelines-c-copy-assignment-signature)
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

    template <typename P>
        requires(stdx::is_value_specialization_of_v<P, path>)
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

template <class T, T... chars> CONSTEVAL auto operator""_g() -> pathlike auto {
    constexpr auto s = stdx::ct_string<sizeof...(chars) + 1U>{{chars..., 0}};
    return detail::make_path<s>();
}

template <class T, T... chars> CONSTEVAL auto operator""_r() -> pathlike auto {
    constexpr auto s = stdx::ct_string<sizeof...(chars) + 1U>{{chars..., 0}};
    return detail::make_path<s>();
}

template <class T, T... chars> CONSTEVAL auto operator""_f() -> pathlike auto {
    constexpr auto s = stdx::ct_string<sizeof...(chars) + 1U>{{chars..., 0}};
    return detail::make_path<s>();
}
} // namespace literals
} // namespace groov
