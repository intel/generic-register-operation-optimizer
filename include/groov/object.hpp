#pragma once

#include <groov/path.hpp>

#include <stdx/ct_string.hpp>
#include <stdx/type_traits.hpp>

#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/list.hpp>

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace groov {
namespace detail {
template <stdx::ct_string Name> struct name_t;
template <typename T> using name_of = typename T::name_t;
} // namespace detail

template <typename T>
concept named =
    stdx::is_specialization_of<typename T::name_t, detail::name_t>().value;

template <stdx::ct_string Name> struct has_name_q {
    template <named T>
    using fn = std::is_same<detail::name_of<T>, detail::name_t<Name>>;
};

template <stdx::ct_string Name, typename... Ts> struct named_container {
    constexpr static auto name = Name;
    using name_t = detail::name_t<Name>;
    using children_t = boost::mp11::mp_list<Ts...>;

    template <stdx::ct_string S>
    using child_t = boost::mp11::mp_front<
        boost::mp11::mp_copy_if_q<children_t, has_name_q<S>>>;
};

namespace detail {
template <typename T, stdx::has_trait<is_path> P>
constexpr auto recursive_resolve(P);

template <typename L, stdx::has_trait<is_path> P>
constexpr auto resolve_matches([[maybe_unused]] P p) {
    if constexpr (boost::mp11::mp_empty<L>::value) {
        return invalid_t{};
    } else if constexpr (boost::mp11::mp_size<L>::value > 1) {
        return ambiguous_t{};
    } else {
        return recursive_resolve<boost::mp11::mp_front<L>>(p);
    }
}

template <typename T, stdx::has_trait<is_path> P>
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
          named... SubFields>
struct field : named_container<Name, SubFields...> {
    using type = T;

    constexpr static auto extract(auto value) {
        constexpr auto mask = (1u << (Msb - Lsb + 1u)) - 1u;
        return static_cast<type>((value >> Lsb) & mask);
    }

    template <stdx::has_trait<is_path> P> constexpr static auto resolve(P p) {
        return detail::recursive_resolve<field>(p);
    }
};

template <stdx::ct_string Name, typename T, auto Address, named... Fields>
struct reg : named_container<Name, Fields...> {
    using type = T;
    constexpr static inline auto address = Address;

    constexpr static auto extract(type value) { return value; }

    template <stdx::has_trait<is_path> P> constexpr static auto resolve(P p) {
        return detail::recursive_resolve<reg>(p);
    }
};

template <typename Reg> struct reg_with_value : Reg {
    typename Reg::type value;
};

namespace detail {
template <typename L> struct any_resolves_q {
    template <stdx::has_trait<is_path> P>
    using fn = boost::mp11::mp_any_of_q<L, resolves_q<P>>;
};
} // namespace detail

template <stdx::ct_string Name, typename...> struct group_with_paths;

template <stdx::ct_string Name, typename Bus, named... Registers>
struct group : named_container<Name, Registers...> {
    using bus_t = Bus;

    template <stdx::has_trait<is_path> P> constexpr static auto resolve(P p) {
        return detail::recursive_resolve<group>(p);
    }

    template <stdx::has_trait<is_path>... Ps>
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
