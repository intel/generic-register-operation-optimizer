#pragma once

#include <stdx/bit.hpp>

#include <concepts>
#include <cstddef>

namespace groov {
template <typename T>
concept mask_spec = requires {
    { T::template mask<std::size_t{}>() } -> std::same_as<std::size_t>;
};

template <typename T>
concept identity_write_function = mask_spec<typename T::id_spec>;
template <typename T>
concept set_write_function = mask_spec<typename T::set_spec>;
template <typename T>
concept clear_write_function = mask_spec<typename T::clear_spec>;

template <typename T>
concept write_function = identity_write_function<T> or set_write_function<T> or
                         clear_write_function<T>;

namespace detail {
template <write_function W, std::unsigned_integral T, std::size_t Msb,
          std::size_t Lsb>
constexpr auto compute_identity_mask() {
    if constexpr (identity_write_function<W>) {
        return stdx::bit_mask<T, Msb, Lsb>();
    } else {
        return T{};
    }
}

template <write_function W, std::unsigned_integral T, T Mask>
constexpr auto compute_identity_value() {
    if constexpr (identity_write_function<W>) {
        return W::id_spec::template mask<Mask>();
    } else {
        return T{};
    }
}
} // namespace detail

namespace m {
struct zero {
    template <auto Mask> constexpr static auto mask() -> decltype(Mask) {
        return {};
    }
};
struct one {
    template <auto Mask> constexpr static auto mask() -> decltype(Mask) {
        return Mask;
    }
};
struct any {
    template <auto Mask> constexpr static auto mask() -> decltype(Mask) {
        return {};
    }
};
} // namespace m

namespace w {
struct replace {
    using set_spec = m::one;
    using clear_spec = m::zero;
};
struct ignore {
    using id_spec = m::any;
    using set_spec = m::one;
    using clear_spec = m::zero;
};
struct one_to_set {
    using id_spec = m::zero;
    using set_spec = m::one;
};
struct one_to_clear {
    using id_spec = m::zero;
    using clear_spec = m::one;
};
struct zero_to_set {
    using id_spec = m::one;
    using set_spec = m::zero;
};
struct zero_to_clear {
    using id_spec = m::one;
    using clear_spec = m::zero;
};
} // namespace w

template <identity_write_function T> struct read_only : T {
    using read_only_t = int;
};

template <typename T>
concept no_identity_write_function = not identity_write_function<T>;

template <no_identity_write_function T> struct write_only : T {
    using write_only_t = int;
};

template <typename T>
concept read_only_write_function =
    identity_write_function<T> and requires { typename T::read_only_t; };

template <typename T>
concept write_only_write_function =
    write_function<T> and requires { typename T::write_only_t; };

template <typename T>
using is_read_only =
    std::bool_constant<read_only_write_function<typename T::write_fn_t>>;

template <typename T>
using is_write_only =
    std::bool_constant<write_only_write_function<typename T::write_fn_t>>;
} // namespace groov
