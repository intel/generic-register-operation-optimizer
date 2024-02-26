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
    reg32 = 0x87654321u;
    iface::num_stores = {};
    iface::num_loads = {};

    SECTION("write the whole register") {
        bus::write<0xffffffffu>(addr32, 0xc001d00du) | async::sync_wait();

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 0);
        CHECK(reg32 == 0xc001d00du);
    }

    SECTION("write byte 0") {
        bus::write<0xffu>(addr32, 0x38u) | async::sync_wait();

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 0);
        CHECK(reg32 == 0x87654338u);
    }

    SECTION("write byte 1") {
        bus::write<0xff00u>(addr32, 0x4200u) | async::sync_wait();

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 0);
        CHECK(reg32 == 0x87654221u);
    }

    SECTION("write byte 2") {
        bus::write<0xff0000u>(addr32, 0x550000u) | async::sync_wait();

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 0);
        CHECK(reg32 == 0x87554321u);
    }

    SECTION("write byte 3") {
        bus::write<0xff000000u>(addr32, 0x13000000u) | async::sync_wait();

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 0);
        CHECK(reg32 == 0x13654321u);
    }

    SECTION("write word at offset 0") {
        bus::write<0xffffu>(addr32, 0xd00du) | async::sync_wait();

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 0);
        CHECK(reg32 == 0x8765d00du);
    }

    SECTION("write word at offset 1") {
        bus::write<0xffff00u>(addr32, 0xc00100u) | async::sync_wait();

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 0);
        CHECK(reg32 == 0x87c00121u);
    }

    SECTION("write word at offset 2") {
        bus::write<0xffff0000u>(addr32, 0x65020000u) | async::sync_wait();

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 0);
        CHECK(reg32 == 0x65024321u);
    }
}

TEST_CASE("read-modify-write", "[mmio_bus]") {
    reg32 = 0xffffffffu;
    iface::num_stores = {};
    iface::num_loads = {};

    SECTION("write a single bit") {
        bus::write<0x1u>(addr32, 0u) | async::sync_wait();

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 1);
        CHECK(reg32 == 0xfffffffeu);
    }

    SECTION("write a nibble") {
        bus::write<0xf000u>(addr32, 0x7000u) | async::sync_wait();

        CHECK(iface::num_stores == 1);
        CHECK(iface::num_loads == 1);
        CHECK(reg32 == 0xffff7fffu);
    }
}
