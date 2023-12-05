#pragma once

#include <async/concepts.hpp>

#include <stdx/ct_string.hpp>
#include <stdx/type_traits.hpp>

#include <concepts>
#include <cstdint>

namespace groov {
namespace detail {
template <stdx::ct_string Name> struct name_t;
template <typename T> using name_of = typename T::name_t;
} // namespace detail

template <typename T>
concept named =
    stdx::is_specialization_of<typename T::name_t, detail::name_t>().value;

template <typename T>
concept containerlike = requires { typename T::children_t; };

template <typename T>
concept fieldlike = named<T> and containerlike<T> and
                    std::unsigned_integral<decltype(T::mask)> and
                    requires { typename T::type_t; };

template <typename T>
concept registerlike = fieldlike<T> and requires {
    typename T::address_t;
    { T::address } -> std::same_as<typename T::address_t const &>;
};

template <typename T, typename Reg>
concept bus_for = requires {
    { T::template read<typename Reg::type_t{}>(Reg::address) } -> async::sender;
};
} // namespace groov
