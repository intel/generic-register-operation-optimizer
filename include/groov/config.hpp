#pragma once

#include <async/concepts.hpp>

#include <groov/make_spec.hpp>
#include <groov/resolve.hpp>

#include <stdx/ct_string.hpp>
#include <stdx/type_traits.hpp>

#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/list.hpp>
#include <boost/mp11/set.hpp>

#include <concepts>
#include <cstddef>
#include <iterator>
#include <limits>
#include <type_traits>

namespace groov {
namespace detail {
template <stdx::ct_string Name> struct name_t;
template <typename T> using name_of = typename T::name_t;
} // namespace detail

template <typename T>
concept named =
    stdx::is_specialization_of<typename T::name_t, detail::name_t>().value;

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

template <typename T>
concept containerlike = requires { typename T::children_t; };

template <typename C, stdx::ct_string Name>
using get_child = typename C::template child_t<Name>;

template <typename T>
concept fieldlike = named<T> and containerlike<T> and
                    std::unsigned_integral<decltype(T::mask)> and
                    requires { typename T::type_t; };

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

template <typename T, pathlike P> constexpr auto recursive_resolve(P p) {
    constexpr auto r = root(p);
    using children_t = typename T::children_t;
    if constexpr (r == T::name) {
        auto const leftover_path = without_root(p);
        if constexpr (std::empty(leftover_path)) {
            return T{};
        } else {
            using matches =
                boost::mp11::mp_copy_if_q<children_t,
                                          resolves_q<decltype(leftover_path)>>;
            return resolve_matches<matches>(leftover_path);
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

    template <std::unsigned_integral RegType>
    constexpr static auto extract(RegType value) {
        return static_cast<type_t>((value & mask) >> Lsb);
    }

    template <std::unsigned_integral RegType>
    constexpr static void insert(RegType &dest, type_t value) {
        dest = (dest & ~mask) | (static_cast<RegType>(value) << Lsb);
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

    constexpr static auto insert(type_t &dest, type_t value) -> void {
        dest = value;
    }

    template <pathlike P> constexpr static auto resolve(P p) {
        return detail::recursive_resolve<reg>(p);
    }
};

template <typename Reg> struct reg_with_value : Reg {
    typename Reg::type_t value;
};

template <typename T>
concept registerlike = fieldlike<T> and requires {
    typename T::address_t;
    { T::address } -> std::same_as<typename T::address_t const &>;
};

template <typename T, typename Reg>
concept bus_for = requires(typename Reg::type_t data) {
    { T::template read<Reg::mask>(Reg::address) } -> async::sender;
    { T::template write<Reg::mask>(Reg::address, data) } -> async::sender;
};

template <stdx::ct_string Name, typename Bus, registerlike... Registers>
    requires(... and bus_for<Bus, Registers>)
struct group : named_container<Name, Registers...> {
    using bus_t = Bus;

    template <pathlike P> constexpr static auto resolve(P p) {
        return detail::recursive_resolve<group>(p);
    }

    constexpr auto operator()(pathlike auto const &...ps) const {
        return make_spec(*this, ps...);
    }

  private:
    friend constexpr auto operator/(group g, pathlike auto const &p) {
        return g(p);
    }
};

namespace detail {
template <typename L> struct any_resolves_q {
    template <pathlike P> using fn = boost::mp11::mp_any_of_q<L, resolves_q<P>>;
};

template <typename G, typename L> constexpr auto check_valid_config() -> void {
    static_assert(boost::mp11::mp_is_set<L>::value,
                  "Duplicate path passed to group");
    static_assert(
        boost::mp11::mp_all_of_q<L,
                                 any_resolves_q<typename G::children_t>>::value,
        "Unresolvable path passed to group");
    stdx::template_for_each<L>([]<typename P>() {
        using rest = boost::mp11::mp_remove<L, P>;
        static_assert(boost::mp11::mp_none_of_q<rest, resolves_q<P>>::value,
                      "Redundant path passed to group");
    });
}

template <typename Group> struct register_for_path_q {
    template <pathlike P>
    using fn = typename Group::template child_t<root(P{})>;
};

template <typename Group> struct register_for_paths_q {
    template <typename L>
    using fn = typename register_for_path_q<Group>::template fn<
        boost::mp11::mp_front<L>>;
};

template <typename Paths> struct field_mask_for_reg_q {
    template <typename R> using mask_t = typename R::type_t;

    template <typename R> constexpr static auto compute_mask() {
        mask_t<R> mask{};
        stdx::template_for_each<Paths>([&]<typename P>() {
            using object_t = resolve_t<R, P>;
            if constexpr (not std::same_as<object_t, invalid_t>) {
                mask |= object_t::mask;
            }
        });
        return mask;
    }

    template <typename R>
    using fn = std::integral_constant<mask_t<R>, compute_mask<R>()>;
};
} // namespace detail
} // namespace groov
