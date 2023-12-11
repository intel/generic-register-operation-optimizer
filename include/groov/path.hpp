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
template <typename T>
concept located_value = pathlike<T> and requires(T &t) {
    { t.value } -> std::same_as<typename T::value_t &>;
};

template <typename Path, typename... Values> struct with_values_t;

template <stdx::ct_string... Parts> struct path {
    template <typename T>
    // NOLINTNEXTLINE(cppcoreguidelines-c-copy-assignment-signature)
    constexpr auto operator=(T const &value) -> with_values_t<path, T> {
        return {{}, value};
    }

    template <typename T> constexpr auto operator()(T const &value) {
        return *this = value;
    }

    template <located_value... Vs>
        requires(sizeof...(Vs) > 0)
    constexpr auto operator()(Vs const &...vs) -> with_values_t<path, Vs...> {
        return {{}, vs...};
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

    constexpr static auto root()
        requires(sizeof...(Parts) > 0)
    {
        return boost::mp11::mp_front<path>::value;
    }

    constexpr static auto without_root()
        requires(sizeof...(Parts) > 0)
    {
        return boost::mp11::mp_drop_c<path, 1>{};
    }

  private:
    friend constexpr auto operator==(path, path) -> bool = default;
};

template <stdx::ct_string... As, stdx::ct_string... Bs>
constexpr auto operator/(path<As...>, path<Bs...>) -> path<As..., Bs...> {
    return {};
}

template <pathlike Path, typename Value>
struct with_values_t<Path, Value> : Path {
    using path_t = Path;
    using value_t = Value;
    value_t value;

    template <pathlike P> constexpr auto resolve(P) const {
        using leftover_path = resolve_t<Path, P>;
        if constexpr (pathlike<leftover_path>) {
            if constexpr (boost::mp11::mp_empty<leftover_path>::value) {
                return value;
            } else {
                return with_values_t<leftover_path, Value>{{}, value};
            }
        } else {
            return leftover_path{};
        }
    }

    template <pathlike P> constexpr auto operator[](P p) const {
        return checked_resolve(*this, p);
    }
};

template <pathlike Path, located_value... Values>
struct with_values_t<Path, Values...> : Path {
    using value_t = stdx::tuple<Values...>;
    value_t value;

    static_assert(boost::mp11::mp_is_set<
                      boost::mp11::mp_transform<get_path_t, value_t>>::value,
                  "Values must have unique paths");

    template <pathlike P> constexpr auto resolve([[maybe_unused]] P p) const {
        constexpr auto len = boost::mp11::mp_size<Path>::value;
        constexpr auto other_len = boost::mp11::mp_size<P>::value;
        if constexpr (other_len > len) {
            using leftover_path = resolve_t<P, Path>;
            if constexpr (pathlike<leftover_path>) {
                using index =
                    boost::mp11::mp_find_if_q<value_t,
                                              resolves_q<leftover_path>>;
                if constexpr (index::value !=
                              boost::mp11::mp_size<value_t>::value) {
                    return groov::resolve(stdx::get<index::value>(value),
                                          leftover_path{});
                } else {
                    return mismatch_t{};
                }
            } else {
                return leftover_path{};
            }
        } else {
            using leftover_path = resolve_t<Path, P>;
            if constexpr (pathlike<leftover_path>) {
                return with_values_t<leftover_path, Values...>{{}, value};
            } else {
                return leftover_path{};
            }
        }
    }

    template <pathlike P> constexpr auto operator[](P p) const {
        return checked_resolve(*this, p);
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
