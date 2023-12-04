#pragma once

#include <groov/identifier.hpp>

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

template <stdx::ct_string Name> struct has_name {
    template <named T>
    using fn = std::is_same<detail::name_of<T>, detail::name_t<Name>>;
};

template <stdx::ct_string Name, typename... Ts> struct named_container {
    using name_t = detail::name_t<Name>;

    template <stdx::ct_string S>
    using child_t = boost::mp11::mp_front<
        boost::mp11::mp_copy_if_q<boost::mp11::mp_list<Ts...>, has_name<S>>>;
};

template <stdx::ct_string Name, typename T, std::size_t Msb, std::size_t Lsb,
          named... SubFields>
struct field : named_container<Name, SubFields...> {
    using type = T;

    constexpr static auto extract(auto value) {
        constexpr auto mask = (1u << (Msb - Lsb + 1u)) - 1u;
        return static_cast<type>((value >> Lsb) & mask);
    }
};

template <stdx::ct_string Name, typename T, auto Address, named... Fields>
struct reg : named_container<Name, Fields...> {
    using type = T;
    constexpr static inline auto address = Address;

    constexpr static auto extract(type value) { return value; }
};

template <stdx::ct_string Name, typename Bus, named... Registers>
struct group : named_container<Name, Registers...> {
    using bus_t = Bus;
};

template <stdx::ct_string Name, typename C>
using get_child = typename C::template child_t<Name>;

template <stdx::ct_string Name, typename Bus, typename Path, named... Registers>
struct group_with_path : group<Name, Bus, Registers...> {
    using path_t = Path;
};

template <stdx::ct_string Name, typename Bus, typename... Registers,
          stdx::ct_string... Parts>
constexpr auto operator/(group<Name, Bus, Registers...>, path<Parts...>)
    -> group_with_path<Name, Bus, path<Parts...>, Registers...> {
    return {};
}

} // namespace groov
