#include <async/just.hpp>
#include <async/just_result_of.hpp>
#include <async/sync_wait.hpp>
#include <async/then.hpp>

#include <groov/mmio_bus.hpp>
#include <groov/path.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>

namespace {
uint32_t volatile reg32{};
auto addr32 = reinterpret_cast<std::uintptr_t>(&reg32);
} // namespace

using namespace groov::literals;

struct iface {
    static inline std::size_t num_stores{};
    static inline std::size_t num_loads{};

    template <std::unsigned_integral T> static auto store(std::uintptr_t addr) {
        ++num_stores;
        return groov::cpp_mem_iface::template store<T>(addr);
    }

    template <std::unsigned_integral T>
    static auto load(std::uintptr_t addr) -> async::sender auto {
        ++num_loads;
        return groov::cpp_mem_iface::template load<T>(addr);
    }
};

using bus = groov::mmio_bus<iface>;

TEST_CASE("fully and partially write a 32-bit without reading it",
          "[mmio_bus]") {
    reg32 = 0x8765'4321u;
    iface::num_stores = {};
    iface::num_loads = {};

    SECTION("write the whole register") {
        bus::write<0xffff'ffffu, 0u, 0u>(addr32, 0xc001'd00du) |
            async::sync_wait();

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 0);
        CHECK(reg32 == 0xc001'd00du);
    }

    SECTION("write byte 0") {
        bus::write<0xffu, 0u, 0u>(addr32, 0x38u) | async::sync_wait();

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 0);
        CHECK(reg32 == 0x8765'4338u);
    }

    SECTION("write byte 1") {
        bus::write<0xff00u, 0u, 0u>(addr32, 0x4200u) | async::sync_wait();

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 0);
        CHECK(reg32 == 0x8765'4221u);
    }

    SECTION("write byte 2") {
        bus::write<0xff'0000u, 0u, 0u>(addr32, 0x55'0000u) | async::sync_wait();

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 0);
        CHECK(reg32 == 0x8755'4321u);
    }

    SECTION("write byte 3") {
        bus::write<0xff00'0000u, 0u, 0u>(addr32, 0x1300'0000u) |
            async::sync_wait();

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 0);
        CHECK(reg32 == 0x1365'4321u);
    }

    SECTION("write word at offset 0") {
        bus::write<0xffffu, 0u, 0u>(addr32, 0xd00du) | async::sync_wait();

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 0);
        CHECK(reg32 == 0x8765'd00du);
    }

    SECTION("write word at offset 1") {
        bus::write<0xffff'00u, 0u, 0u>(addr32, 0xc001'00u) | async::sync_wait();

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 0);
        CHECK(reg32 == 0x87c00'121u);
    }

    SECTION("write word at offset 2") {
        bus::write<0xffff'0000u, 0u, 0u>(addr32, 0x6502'0000u) |
            async::sync_wait();

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 0);
        CHECK(reg32 == 0x6502'4321u);
    }
}

TEST_CASE("read-modify-write", "[mmio_bus]") {
    reg32 = 0xffff'ffffu;
    iface::num_stores = {};
    iface::num_loads = {};

    SECTION("write a single bit") {
        bus::write<0x1u, 0u, 0u>(addr32, 0u) | async::sync_wait();

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 1);
        CHECK(reg32 == 0xffff'fffeu);
    }

    SECTION("write a nibble") {
        bus::write<0xf000u, 0u, 0u>(addr32, 0x7000u) | async::sync_wait();

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 1);
        CHECK(reg32 == 0xffff'7fffu);
    }
}
