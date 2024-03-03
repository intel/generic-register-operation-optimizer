#pragma once

#include <async/concepts.hpp>
#include <async/just.hpp>
#include <async/just_result_of.hpp>
#include <async/then.hpp>

#include <bit>
#include <concepts>
#include <cstdint>

namespace groov {
namespace detail {
template <std::size_t NumBits> struct uint_of_width;

template <> struct uint_of_width<8> {
    using type = std::uint8_t;
};

template <> struct uint_of_width<16> {
    using type = std::uint16_t;
};

template <> struct uint_of_width<32> {
    using type = std::uint32_t;
};

template <> struct uint_of_width<64> {
    using type = std::uint64_t;
};

template <std::size_t NumBits>
using uint_of_width_t = typename uint_of_width<NumBits>::type;
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
};

template <typename HardwareInterface = cpp_mem_iface> struct mmio_bus {
    using iface = HardwareInterface;

    template <auto Mask, decltype(Mask) IdMask, decltype(Mask) IdValue>
        requires std::unsigned_integral<decltype(Mask)>
    static auto write(auto addr, decltype(Mask) value) -> async::sender auto {
        using base_type = decltype(Mask);

        constexpr std::size_t lsb = std::countr_zero(Mask);
        constexpr std::size_t width = std::countr_one(Mask >> lsb);
        constexpr bool byte_aligned = (lsb % 8) == 0;
        constexpr bool contiguous = std::popcount(Mask) == width;
        constexpr bool pow2_width = std::has_single_bit(width);
        constexpr bool accessible_width = width & (8u | 16u | 32u | 64u);
        constexpr bool subword_access_en =
            byte_aligned && contiguous && pow2_width && accessible_width;

        if constexpr (subword_access_en) {
            using subword_t = detail::uint_of_width_t<width>;
            constexpr auto byte_offset = lsb / 8;
            auto const subword_addr = addr + byte_offset;
            auto const subword_val = static_cast<subword_t>(value >> lsb);

            return async::just(subword_val) |
                   iface::template store<subword_t>(subword_addr);

        } else {
            return iface::template load<base_type>(addr) |
                   async::then([=](base_type old) {
                       return (value & Mask) | (old & ~Mask);
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
