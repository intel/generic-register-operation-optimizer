#pragma once

#include <stdx/utility.hpp>

#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/list.hpp>

#include <concepts>
#include <cstddef>
#include <limits>
#include <type_traits>

namespace groov {
namespace detail {
template <std::unsigned_integral T, std::size_t Bit>
    requires(Bit <= std::numeric_limits<T>::digits)
constexpr auto mask_bits() -> T {
    if constexpr (Bit == std::numeric_limits<T>::digits) {
        return std::numeric_limits<T>::max();
    } else {
        return (T{1} << Bit) - T{1};
    }
}

template <std::unsigned_integral T, std::size_t Msb, std::size_t Lsb>
constexpr auto compute_mask() -> T {
    return mask_bits<T, Msb + 1>() - mask_bits<T, Lsb>();
}
} // namespace detail

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
        return compute_mask<T, Msb, Lsb>();
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
        return detail::compute_mask<T, Msb, Lsb>();
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
struct read_only {
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
} // namespace groov
