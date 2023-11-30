#pragma once

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
    using type_t = T;
};

using address_t = std::uint32_t;

template <stdx::ct_string Name, typename T, address_t Address, named... Fields>
struct reg : named_container<Name, Fields...> {
    using type_t = T;
};

template <stdx::ct_string Name, typename Bus, named... Registers>
struct group : named_container<Name, Registers...> {};

template <stdx::ct_string Name, typename C>
using get_child = typename C::template child_t<Name>;
} // namespace groov
