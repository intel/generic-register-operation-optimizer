#pragma once

#include <groov/attach_value.hpp>
#include <groov/resolve.hpp>

#include <stdx/tuple.hpp>
#include <stdx/type_traits.hpp>

#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/list.hpp>

#include <iterator>

namespace groov {
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
        return value_path<decltype(groov::without_root(Path{})), Value>{{},
                                                                        value};
    }

  private:
    friend constexpr auto operator==(value_path const &, value_path const &)
        -> bool = default;

    template <pathlike P>
        requires(not valued<P>)
    friend constexpr auto operator/(P p, value_path const &v) {
        return value_path<decltype(p / Path{}), Value>{{}, v.value};
    }
};

template <pathlike P, typename... Vs>
    requires(not valued<P>)
constexpr auto tag_invoke(attach_value_t, P, Vs const &...vs) {
    if constexpr (sizeof...(Vs) > 1) {
        return value_path<P, stdx::tuple<Vs...>>{{}, {vs...}};
    } else {
        return value_path<P, Vs...>{{}, vs...};
    }
}
} // namespace groov
