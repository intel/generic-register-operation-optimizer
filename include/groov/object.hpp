#pragma once

#include <groov/concepts.hpp>
#include <groov/path.hpp>

#include <stdx/ct_string.hpp>
#include <stdx/type_traits.hpp>

#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/list.hpp>

#include <cstddef>
#include <limits>
#include <type_traits>

namespace groov {
template <stdx::ct_string Name, named... Ts> struct named_container {
  private:
    template <stdx::ct_string N> struct has_name_q {
        template <named T>
        using fn = std::is_same<detail::name_of<T>, detail::name_t<N>>;
    };

  public:
    constexpr static auto name = Name;
    using name_t = detail::name_t<Name>;
    using children_t = boost::mp11::mp_list<Ts...>;

    template <stdx::ct_string S>
    using child_t = boost::mp11::mp_front<
        boost::mp11::mp_copy_if_q<children_t, has_name_q<S>>>;
};

namespace detail {
template <typename T, pathlike P> constexpr auto recursive_resolve(P);

template <typename L, pathlike P>
constexpr auto resolve_matches([[maybe_unused]] P p) {
    if constexpr (boost::mp11::mp_empty<L>::value) {
        return invalid_t{};
    } else if constexpr (boost::mp11::mp_size<L>::value > 1) {
        return ambiguous_t{};
    } else {
        return recursive_resolve<boost::mp11::mp_front<L>>(p);
    }
}

template <typename T, pathlike P>
constexpr auto recursive_resolve([[maybe_unused]] P p) {
    constexpr auto root = P::root();
    using children_t = typename T::children_t;
    if constexpr (root == T::name) {
        using leftover_path = decltype(P::without_root());
        if constexpr (boost::mp11::mp_empty<leftover_path>::value) {
            return T{};
        } else {
            using matches =
                boost::mp11::mp_copy_if_q<children_t,
                                          resolves_q<leftover_path>>;
            return resolve_matches<matches>(leftover_path{});
        }
    } else {
        using matches = boost::mp11::mp_copy_if_q<children_t, resolves_q<P>>;
        return resolve_matches<matches>(p);
    }
}
} // namespace detail

template <stdx::ct_string Name, typename T, std::size_t Msb, std::size_t Lsb,
          fieldlike... SubFields>
struct field : named_container<Name, SubFields...> {
    using type_t = T;
    constexpr static auto mask = ((1u << (Msb - Lsb + 1u)) - 1u) << Lsb;

    constexpr static auto extract(std::unsigned_integral auto value) {
        return static_cast<type_t>((value & mask) >> Lsb);
    }

    template <pathlike P> constexpr static auto resolve(P p) {
        return detail::recursive_resolve<field>(p);
    }
};

template <stdx::ct_string Name, std::unsigned_integral T, auto Address,
          fieldlike... Fields>
struct reg : named_container<Name, Fields...> {
    using type_t = T;
    constexpr static auto mask = std::numeric_limits<type_t>::max();

    using address_t = decltype(Address);
    constexpr static auto address = Address;

    constexpr static auto extract(std::unsigned_integral auto value) {
        return value;
    }

    template <pathlike P> constexpr static auto resolve(P p) {
        return detail::recursive_resolve<reg>(p);
    }
};

template <typename Reg> struct reg_with_value : Reg {
    typename Reg::type_t value;
};

namespace detail {
template <typename L> struct any_resolves_q {
    template <pathlike P> using fn = boost::mp11::mp_any_of_q<L, resolves_q<P>>;
};
} // namespace detail

template <stdx::ct_string Name, typename...> struct group_with_paths;

template <stdx::ct_string Name, typename Bus, registerlike... Registers>
    requires(... and bus_for<Bus, Registers>)
struct group : named_container<Name, Registers...> {
    using bus_t = Bus;

    template <pathlike P> constexpr static auto resolve(P p) {
        return detail::recursive_resolve<group>(p);
    }

    template <pathlike... Ps>
        requires(sizeof...(Ps) > 0)
    constexpr auto operator()(Ps const &...) const
        -> group_with_paths<Name, Bus, boost::mp11::mp_list<Ps...>,
                            Registers...> {
        using L = boost::mp11::mp_list<Ps...>;
        static_assert(boost::mp11::mp_is_set<L>::value,
                      "Duplicate path passed to group");
        static_assert(
            boost::mp11::mp_all_of_q<
                L, detail::any_resolves_q<typename group::children_t>>::value,
            "Unresolvable path passed to group");
        stdx::template_for_each<L>([]<typename P>() {
            using rest = boost::mp11::mp_remove<L, P>;
            static_assert(boost::mp11::mp_none_of_q<rest, resolves_q<P>>::value,
                          "Redundant path passed to group");
        });
        return {};
    }
};

template <stdx::ct_string Name, typename C>
using get_child = typename C::template child_t<Name>;

template <stdx::ct_string Name, typename Bus, typename Paths,
          named... Registers>
struct group_with_paths<Name, Bus, Paths, Registers...>
    : group<Name, Bus, Registers...> {
    using paths_t = Paths;
};

template <stdx::ct_string Name, typename Bus, typename... Registers,
          stdx::ct_string... Parts>
constexpr auto operator/(group<Name, Bus, Registers...> g, path<Parts...> p) {
    return g(p);
}
} // namespace groov
