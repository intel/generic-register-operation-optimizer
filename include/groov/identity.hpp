#pragma once

#include <stdx/bit.hpp>
#include <stdx/utility.hpp>

#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/list.hpp>

#include <concepts>
#include <cstddef>
#include <limits>
#include <type_traits>

namespace groov {
template <typename T>
concept identity_spec = requires {
    {
        T::template identity<unsigned int, std::size_t{}, std::size_t{}>()
    } -> std::same_as<unsigned int>;
};

namespace detail {
template <typename I, std::unsigned_integral T, std::size_t Msb,
          std::size_t Lsb>
constexpr auto compute_identity_mask() {
    if constexpr (identity_spec<I>) {
        return stdx::bit_mask<T, Msb, Lsb>();
    } else {
        return T{};
    }
}

template <typename I, std::unsigned_integral T, std::size_t Msb,
          std::size_t Lsb>
constexpr auto compute_identity() {
    if constexpr (identity_spec<I>) {
        return I::template identity<T, Msb, Lsb>();
    } else {
        return T{};
    }
}
} // namespace detail

namespace id {
struct none {};
struct zero {
    template <std::unsigned_integral T, std::size_t, std::size_t>
    constexpr static auto identity() -> T {
        return {};
    }
};
struct one {
    template <std::unsigned_integral T, std::size_t Msb, std::size_t Lsb>
    constexpr static auto identity() -> T {
        return stdx::bit_mask<T, Msb, Lsb>();
    }
};
struct any {
    template <std::unsigned_integral T, std::size_t, std::size_t>
    constexpr static auto identity() -> T {
        return {};
    }
};
} // namespace id

template <typename T>
concept write_function = requires { typename T::id_spec; };

namespace w {
struct replace {
    using id_spec = id::none;
};
struct ignore {
    using id_spec = id::any;
};
struct one_to_set {
    using id_spec = id::zero;
};
struct one_to_clear {
    using id_spec = id::zero;
};
struct zero_to_set {
    using id_spec = id::one;
};
struct zero_to_clear {
    using id_spec = id::one;
};
} // namespace w

template <typename T>
concept identity_write_function =
    write_function<T> and identity_spec<typename T::id_spec>;

template <identity_write_function T> struct read_only : T {
    using read_only_t = int;
};

template <typename T>
concept read_only_write_function =
    identity_write_function<T> and requires { typename T::read_only_t; };
} // namespace groov
