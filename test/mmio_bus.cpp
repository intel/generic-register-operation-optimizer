#include <groov/mmio_bus.hpp>
#include <groov/path.hpp>

#include <async/just.hpp>
#include <async/just_result_of.hpp>
#include <async/sync_wait.hpp>
#include <async/then.hpp>

#include <stdx/bit.hpp>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>

namespace {
uint32_t reg32{};
uint64_t reg64{};
} // namespace

using namespace groov::literals;

struct iface {
    static inline std::size_t num_stores{};
    static inline std::size_t num_loads{};

    template <std::unsigned_integral T> static auto store(auto addr) {
        ++num_stores;
        return groov::cpp_mem_iface::template store<T>(addr);
    }

    template <std::unsigned_integral T>
    static auto load(auto addr) -> async::sender auto {
        ++num_loads;
        return groov::cpp_mem_iface::template load<T>(addr);
    }

    template <typename T>
    constexpr static std::size_t alignment =
        groov::cpp_mem_iface::template alignment<T>;
};

using be_bus = groov::byte_enabled_mmio_bus<iface>;
using bus = groov::mmio_bus<iface>;

TEMPLATE_TEST_CASE("subword write optimization", "[byte_enabled_mmio_bus]",
                   std::uintptr_t, decltype(&reg32)) {
    auto addr = stdx::bit_cast<TestType>(&reg32);

    reg32 = 0x8765'4321u;
    iface::num_stores = {};
    iface::num_loads = {};

    SECTION("write the whole register") {
        CHECK(be_bus::write<"", 0xffff'ffffu, 0u, 0u>(addr, 0xc001'd00du) |
              async::sync_wait());

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 0);
        CHECK(reg32 == 0xc001'd00du);
    }

    SECTION("write byte 0") {
        CHECK(be_bus::write<"", 0xffu, 0u, 0u>(addr, 0x38u) |
              async::sync_wait());

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 0);
        CHECK(reg32 == 0x8765'4338u);
    }

    SECTION("write byte 1") {
        CHECK(be_bus::write<"", 0xff00u, 0u, 0u>(addr, 0x4200u) |
              async::sync_wait());

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 0);
        CHECK(reg32 == 0x8765'4221u);
    }

    SECTION("write byte 2") {
        CHECK(be_bus::write<"", 0xff'0000u, 0u, 0u>(addr, 0x55'0000u) |
              async::sync_wait());

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 0);
        CHECK(reg32 == 0x8755'4321u);
    }

    SECTION("write byte 3") {
        CHECK(be_bus::write<"", 0xff00'0000u, 0u, 0u>(addr, 0x1300'0000u) |
              async::sync_wait());

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 0);
        CHECK(reg32 == 0x1365'4321u);
    }

    SECTION("write word at offset 0") {
        CHECK(be_bus::write<"", 0xffffu, 0u, 0u>(addr, 0xd00du) |
              async::sync_wait());

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 0);
        CHECK(reg32 == 0x8765'd00du);
    }

    SECTION("write word at offset 2") {
        CHECK(be_bus::write<"", 0xffff'0000u, 0u, 0u>(addr, 0x6502'0000u) |
              async::sync_wait());

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 0);
        CHECK(reg32 == 0x6502'4321u);
    }
}

TEMPLATE_TEST_CASE("read-modify-write", "[byte_enabled_mmio_bus]",
                   std::uintptr_t, decltype(&reg32)) {
    auto addr = stdx::bit_cast<TestType>(&reg32);

    reg32 = 0xffff'ffffu;
    iface::num_stores = {};
    iface::num_loads = {};

    SECTION("write a single bit") {
        CHECK(be_bus::write<"", 0x1u, 0u, 0u>(addr, 0u) | async::sync_wait());

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 1);
        CHECK(reg32 == 0xffff'fffeu);
    }

    SECTION("write a nibble") {
        CHECK(be_bus::write<"", 0xf000u, 0u, 0u>(addr, 0x7000u) |
              async::sync_wait());

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 1);
        CHECK(reg32 == 0xffff'7fffu);
    }
}

TEMPLATE_TEST_CASE("idmask write optimization", "[byte_enabled_mmio_bus]",
                   std::uintptr_t, decltype(&reg32)) {
    auto addr = stdx::bit_cast<TestType>(&reg32);

    reg32 = 0xffff'ffffu;
    iface::num_stores = {};
    iface::num_loads = {};

    SECTION("write bit in byte 0") {
        CHECK(be_bus::write<"", 0x1u, 0xffff'fffeu, 0xffff'fffeu>(addr, 0u) |
              async::sync_wait());

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 0);
        CHECK(reg32 == 0xffff'fffeu);
    }
    SECTION("write bit in byte 0 - requires subword write") {
        CHECK(be_bus::write<"", 0x1u, 0xfeu, 0xfeu>(addr, 0u) |
              async::sync_wait());

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 0);
        CHECK(reg32 == 0xffff'fffeu);
    }

    SECTION("write nibble in byte 1") {
        CHECK(be_bus::write<"", 0x0f00u, 0xffff'f0ffu, 0xffff'f0ffu>(addr,
                                                                     0x0300u) |
              async::sync_wait());

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 0);
        CHECK(reg32 == 0xffff'f3ffu);
    }

    SECTION("write nibble in byte 1 - requires subword write") {
        CHECK(be_bus::write<"", 0x0f00u, 0xf0ffu, 0xf0ffu>(addr, 0x0300u) |
              async::sync_wait());

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 0);
        CHECK(reg32 == 0xffff'f3ffu);
    }

    SECTION("write across two bytes - requires subword write") {
        CHECK(be_bus::write<"", 0x0ff0u, 0xf00fu, 0xf00fu>(addr, 0x0340u) |
              async::sync_wait());

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 0);
        CHECK(reg32 == 0xffff'f34fu);
    }

    SECTION("write nibble in byte 2") {
        CHECK(be_bus::write<"", 0xf0'0000u, 0xff0f'ffffu, 0xff0f'ffffu>(
                  addr, 0x40'0000u) |
              async::sync_wait());

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 0);
        CHECK(reg32 == 0xff4f'ffffu);
    }

    SECTION("write nibble in byte 3") {
        CHECK(be_bus::write<"", 0x0f00'0000u, 0xf0ff'ffffu, 0xf0ff'ffffu>(
                  addr, 0x1300'0000u) |
              async::sync_wait());

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 0);
        CHECK(reg32 == 0xf3ff'ffffu);
    }

    SECTION("write across three bytes") {
        CHECK(be_bus::write<"", 0xff'ffffu, 0xff00'0000u, 0xff00'0000u>(
                  addr, 0x41'd00du) |
              async::sync_wait());

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 0);
        CHECK(reg32 == 0xff41'd00du);
    }

    SECTION("write across three bytes - check id bit write") {
        CHECK(be_bus::write<"", 0x00ff'ffffu, 0xff00'0000u, 0u>(addr,
                                                                0x41'd00du) |
              async::sync_wait());

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 0);
        CHECK(reg32 == 0x0041'd00du);
    }

    SECTION("write across three bytes - start at byte 1") {
        CHECK(be_bus::write<"", 0xffff'ff00u, 0x0000'00ffu, 0x0000'00ffu>(
                  addr, 0x41d0'0d00u) |
              async::sync_wait());

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 0);
        CHECK(reg32 == 0x41d0'0dffu);
    }
}

TEMPLATE_TEST_CASE("32-bit register write", "[mmio_bus]", std::uintptr_t,
                   decltype(&reg32)) {
    auto addr = stdx::bit_cast<TestType>(&reg32);

    reg32 = 0x8765'4321u;
    iface::num_stores = {};
    iface::num_loads = {};

    SECTION("mask covers the whole register") {
        CHECK(bus::write<"", 0xffff'ffffu, 0u, 0u>(addr, 0xc001'd00du) |
              async::sync_wait());

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 0);
        CHECK(reg32 == 0xc001'd00du);
    }

    SECTION("mask and idmask together cover the whole register") {
        CHECK(bus::write<"", 0xffff'0000u, 0x0000'ffffu, 0x0000'beefu>(
                  addr, 0x6502'0000u) |
              async::sync_wait());

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 0);
        CHECK(reg32 == 0x6502'beefu);
    }

    SECTION("a byte write needs a read") {
        CHECK(bus::write<"", 0xffu, 0u, 0u>(addr, 0x38u) | async::sync_wait());

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 1);
        CHECK(reg32 == 0x8765'4338u);
    }

    SECTION("a byte-wide idmask is not enough to avoid a read") {
        CHECK(bus::write<"", 0x1u, 0xfeu, 0xfeu>(addr, 0u) |
              async::sync_wait());

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 1);
        CHECK(reg32 == 0x8765'43feu);
    }
}

TEST_CASE("64-bit register write", "[mmio_bus]") {
    auto addr = stdx::bit_cast<std::uintptr_t>(&reg64);

    reg64 = 0x0123'4567'89ab'cdefu;
    iface::num_stores = {};
    iface::num_loads = {};

    SECTION("whole register") {
        CHECK(bus::write<"", std::uint64_t{0xffff'ffff'ffff'ffff},
                         std::uint64_t{}, std::uint64_t{}>(
                  addr, std::uint64_t{0x1122'3344'5566'7788}) |
              async::sync_wait());

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 0);
        CHECK(reg64 == 0x1122'3344'5566'7788u);
    }

    SECTION("upper half") {
        CHECK(bus::write<"", std::uint64_t{0xffff'ffff'0000'0000},
                         std::uint64_t{}, std::uint64_t{}>(
                  addr, std::uint64_t{0xdead'beef'0000'0000}) |
              async::sync_wait());

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 0);
        CHECK(reg64 == 0xdead'beef'89ab'cdefu);
    }

    SECTION("lower half") {
        CHECK(bus::write<"", std::uint64_t{0x0000'0000'ffff'ffff},
                         std::uint64_t{}, std::uint64_t{}>(
                  addr, std::uint64_t{0x0000'0000'c001'd00d}) |
              async::sync_wait());

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 0);
        CHECK(reg64 == 0x0123'4567'c001'd00du);
    }

    SECTION("part of the upper half still reads the whole register") {
        CHECK(bus::write<"", std::uint64_t{0xffff'0000'0000'0000},
                         std::uint64_t{}, std::uint64_t{}>(
                  addr, std::uint64_t{0xdead'0000'0000'0000}) |
              async::sync_wait());

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 1);
        CHECK(reg64 == 0xdead'4567'89ab'cdefu);
    }

    SECTION("part of the upper half but read no longer needed") {
        CHECK(bus::write<"", std::uint64_t{0xffff'0000'0000'0000},
                         std::uint64_t{0x0000'ffff'0000'0000},
                         std::uint64_t{0x0000'cafe'0000'0000}>(
                  addr, std::uint64_t{0xbeef'0000'0000'0000}) |
              async::sync_wait());

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 0);
        CHECK(reg64 == 0xbeef'cafe'89ab'cdefu);
    }
}
