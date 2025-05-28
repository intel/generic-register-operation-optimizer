#pragma once

#include <groov/identity.hpp>
#include <groov/make_spec.hpp>
#include <groov/resolve.hpp>

#include <async/concepts.hpp>

#include <stdx/bit.hpp>
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
concept fieldlike =
    named<T> and containerlike<T> and requires { typename T::type_t; };

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
          write_function WriteFn = w::replace, fieldlike... SubFields>
    requires std::is_trivial_v<T>
struct field : named_container<Name, SubFields...> {
    using type_t = T;
    using write_fn_t = WriteFn;
    using id_spec_t = typename write_fn_t::id_spec;

    template <std::unsigned_integral RegType>
    constexpr static auto identity =
        detail::compute_identity<id_spec_t, RegType, Msb, Lsb>();

    template <std::unsigned_integral RegType>
    constexpr static auto mask = stdx::bit_mask<RegType, Msb, Lsb>();

    template <std::unsigned_integral RegType>
    constexpr static auto identity_mask =
        detail::compute_identity_mask<id_spec_t, RegType, Msb, Lsb>();

    template <std::unsigned_integral RegType>
    constexpr static auto extract(RegType value) -> type_t {
        return static_cast<type_t>((value & mask<RegType>) >> Lsb);
    }

    template <std::unsigned_integral RegType>
    constexpr static auto insert(RegType &dest, type_t value) -> void {
        dest = static_cast<RegType>((dest & ~mask<RegType>) |
                                    (static_cast<RegType>(value) << Lsb));
    }

    template <pathlike P> constexpr static auto resolve(P p) {
        return detail::recursive_resolve<field>(p);
    }
};

template <stdx::ct_string Name, std::unsigned_integral T, auto Address,
          write_function WriteFn = w::replace, fieldlike... Fields>
struct reg : field<Name, T, std::numeric_limits<T>::digits - 1, 0u, WriteFn,
                   Fields...> {
    using address_t = decltype(Address);
    constexpr static auto address = Address;

    template <std::same_as<T> RegType>
    constexpr static auto extract(RegType value) {
        return value;
    }

    template <std::same_as<T> RegType>
    constexpr static auto insert(RegType &dest, RegType value) -> void {
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
    { T::template read<typename Reg::type_t{}>(Reg::address) } -> async::sender;
    {
        T::template write<typename Reg::type_t{}, typename Reg::type_t{},
                          typename Reg::type_t{}>(Reg::address, data)
    } -> async::sender;
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

template <typename Paths> struct fields_for_reg_q {
    template <typename R> struct resolve_path_t {
        template <typename P> using fn = resolve_t<R, P>;
    };

    template <typename R>
    using fields_t = boost::mp11::mp_transform_q<resolve_path_t<R>, Paths>;

    template <typename R>
    using fn = boost::mp11::mp_remove<fields_t<R>, invalid_t>;
};

template <typename M1, typename M2>
using bitwise_or_t =
    std::integral_constant<typename M1::value_type, M1::value | M2::value>;

template <typename Reg> struct mask_q {
    template <typename Obj>
    using fn = std::integral_constant<typename Reg::type_t,
                                      Obj::template mask<typename Reg::type_t>>;
};

template <typename Reg> struct id_mask_q {
    template <typename Obj>
    using fn = std::integral_constant<
        typename Reg::type_t,
        Obj::template identity_mask<typename Reg::type_t>>;
};

template <typename Reg> struct id_value_q {
    template <typename Obj>
    using fn =
        std::integral_constant<typename Reg::type_t,
                               Obj::template identity<typename Reg::type_t>>;
};

template <typename ObjList, template <typename...> typename QFn, typename Reg>
using bitwise_accum_t =
    boost::mp11::mp_fold<boost::mp11::mp_transform_q<QFn<Reg>, ObjList>,
                         std::integral_constant<typename Reg::type_t, 0>,
                         bitwise_or_t>;

template <typename Paths> struct field_mask_for_reg_q {
    template <typename R>
    using fn = bitwise_accum_t<typename fields_for_reg_q<Paths>::template fn<R>,
                               mask_q, R>;
};

template <typename Obj>
using get_children =
    stdx::conditional_t<boost::mp11::mp_empty<typename Obj::children_t>::value,
                        boost::mp11::mp_list<Obj>, typename Obj::children_t>;

template <typename ObjList>
using expand_children =
    boost::mp11::mp_flatten<boost::mp11::mp_transform<get_children, ObjList>>;

template <typename ObjList>
using maybe_expand_children =
    std::enable_if_t<not std::is_same_v<ObjList, expand_children<ObjList>>,
                     expand_children<ObjList>>;

template <typename ObjList>
using all_fields_t = boost::mp11::mp_back<boost::mp11::mp_iterate<
    ObjList, boost::mp11::mp_identity_t, maybe_expand_children>>;
} // namespace detail
} // namespace groov
