#pragma once

#include <groov/identity.hpp>

#include <async/concepts.hpp>
#include <async/just.hpp>
#include <async/just_result_of.hpp>
#include <async/then.hpp>

#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/integral.hpp>
#include <boost/mp11/list.hpp>

#include <bit>
#include <concepts>
#include <cstdint>

namespace groov {
namespace detail {
using namespace boost::mp11;

template <std::unsigned_integral T, std::size_t ByteOffset>
struct subword_spec {
    using subword_t = T;
    constexpr static auto offset = ByteOffset;

    template <typename M>
    constexpr static M mask =
        static_cast<M>(std::numeric_limits<T>::max()) << (ByteOffset * 8);
};

using subword_permutations =
    mp_list<subword_spec<std::uint8_t, 0u>, subword_spec<std::uint8_t, 1u>,
            subword_spec<std::uint8_t, 2u>, subword_spec<std::uint8_t, 3u>,
            subword_spec<std::uint8_t, 4u>, subword_spec<std::uint8_t, 5u>,
            subword_spec<std::uint8_t, 6u>, subword_spec<std::uint8_t, 7u>,

            subword_spec<std::uint16_t, 0u>, subword_spec<std::uint16_t, 1u>,
            subword_spec<std::uint16_t, 2u>, subword_spec<std::uint16_t, 3u>,
            subword_spec<std::uint16_t, 4u>, subword_spec<std::uint16_t, 5u>,
            subword_spec<std::uint16_t, 6u>,

            subword_spec<std::uint32_t, 0u>, subword_spec<std::uint32_t, 1u>,
            subword_spec<std::uint32_t, 2u>, subword_spec<std::uint32_t, 3u>,
            subword_spec<std::uint32_t, 4u>,

            subword_spec<std::uint64_t, 0u>>;

template <std::unsigned_integral BaseType> struct fits_within {
    constexpr static auto base_size = sizeof(BaseType);

    template <typename S>
    using fn = std::bool_constant<(sizeof(typename S::subword_t) + S::offset) <=
                                  base_size>;
};

template <typename HardwareInterface> struct aligned_for {
    using iface = HardwareInterface;

    template <typename S>
    using fn = std::bool_constant<
        (S::offset % iface::template alignment<typename S::subword_t>) == 0>;
};

template <auto Mask, decltype(Mask) IdMask> struct subword_satisfies {
    using base_type = decltype(Mask);

    template <typename S> constexpr static auto calculate() -> bool {
        constexpr base_type subword_mask = S::template mask<base_type>;
        constexpr bool subword_covers_mask = (Mask & subword_mask) == Mask;
        constexpr bool subword_covered =
            ((Mask | IdMask) & subword_mask) == subword_mask;
        return subword_covers_mask and subword_covered;
    }

    template <typename S> using fn = std::bool_constant<calculate<S>()>;
};

} // namespace detail

struct cpp_mem_iface {
    template <std::unsigned_integral T> static auto store(std::uintptr_t addr) {
        return async::then([=](T value) -> void {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            *reinterpret_cast<T volatile *>(addr) = value;
        });
    }

    template <std::unsigned_integral T>
    static auto load(std::uintptr_t addr) -> async::sender auto {
        return async::just_result_of([=]() -> T {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            return *reinterpret_cast<T volatile *>(addr);
        });
    }

    template <typename T> constexpr static std::size_t alignment = alignof(T);
};

template <typename HardwareInterface = cpp_mem_iface> struct mmio_bus {
    using iface = HardwareInterface;

    template <auto Mask, decltype(Mask) IdMask, decltype(Mask) IdValue>
        requires std::unsigned_integral<decltype(Mask)>
    static auto write(auto addr, decltype(Mask) value) -> async::sender auto {
        static_assert((Mask & IdMask) == decltype(Mask){});
        static_assert((Mask & IdValue) == decltype(Mask){});

        using base_type = decltype(Mask);

        using subwords_aligned =
            detail::mp_copy_if_q<detail::subword_permutations,
                                 detail::aligned_for<iface>>;

        using subwords_aligned_and_fit =
            detail::mp_copy_if_q<subwords_aligned,
                                 detail::fits_within<base_type>>;

        using subword_candidates =
            detail::mp_copy_if_q<subwords_aligned_and_fit,
                                 detail::subword_satisfies<Mask, IdMask>>;

        if constexpr (!detail::mp_empty<subword_candidates>::value) {
            using subword = detail::mp_first<subword_candidates>;
            using subword_t = typename subword::subword_t;

            auto const write_val = value | IdValue;
            auto const subword_addr = addr + subword::offset;
            auto const subword_write_val =
                static_cast<subword_t>(write_val >> (subword::offset * 8));

            return async::just(subword_write_val) |
                   iface::template store<subword_t>(subword_addr);

        } else {
            return iface::template load<base_type>(addr) |
                   async::then([=](base_type old) {
                       auto const bits_to_update = value & Mask;
                       auto const bits_to_writeback = old & ~Mask & ~IdMask;

                       return bits_to_update | bits_to_writeback | IdValue;
                   }) |
                   iface::template store<base_type>(addr);
        }
    }

    template <std::unsigned_integral auto Mask>
    static auto read(auto addr) -> async::sender auto {
        using base_type = decltype(Mask);
        return iface::template load<base_type>(addr);
    }
};
} // namespace groov
