#include <groov/path.hpp>
#include <groov/read.hpp>
#include <groov/value_path.hpp>
#include <groov/write.hpp>
#include <groov/write_spec.hpp>

#include <async/just.hpp>
#include <async/just_result_of.hpp>
#include <async/sync_wait.hpp>
#include <async/then.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>

namespace {
struct bus {
    static inline int num_reads{};
    static inline int num_writes{};
    static inline std::uint32_t last_mask{};

    template <stdx::ct_string, auto Mask, auto IdMask, auto IdValue>
    static auto write(auto addr, auto value) -> async::sender auto {
        return async::just_result_of([=] {
            ++num_writes;
            last_mask = Mask;
            auto prev = *addr & ~(Mask | IdMask);
            *addr = prev | value | IdValue;
        });
    }

    template <stdx::ct_string, auto Mask>
    static auto read(auto addr) -> async::sender auto {
        return async::just_result_of([=] {
            ++num_reads;
            last_mask = Mask;
            return *addr;
        });
    }
};

using F0 = groov::field<"field0", std::uint8_t, 0, 0>;
using F1 = groov::field<"field1", std::uint8_t, 4, 1>;
using F2 = groov::field<"field2", std::uint8_t, 7, 5>;

std::uint32_t data0{};
using R0 =
    groov::reg<"reg0", std::uint32_t, &data0, groov::w::replace, F0, F1, F2>;
std::uint32_t data1{};
using R1 =
    groov::reg<"reg1", std::uint32_t, &data1, groov::w::replace, F0, F1, F2>;

using G = groov::group<"group", bus, R0, R1>;
constexpr auto grp = G{};
} // namespace

TEST_CASE("write a register", "[write]") {
    using namespace groov::literals;
    data0 = 0;
    CHECK(groov::write(grp("reg0"_r = 0xa5a5'a5a5u)) | async::sync_wait());
    CHECK(data0 == 0xa5a5'a5a5u);
}

TEST_CASE("write with passthrough", "[write]") {
    using namespace groov::literals;
    data0 = 0;
    auto value =
        groov::write(grp("reg0"_r = 0xa5a5'a5a5u), 42, 17) | async::sync_wait();
    CHECK(value);
    CHECK(get<0>(*value) == 42);
    CHECK(get<1>(*value) == 17);
    CHECK(data0 == 0xa5a5'a5a5u);
}

TEST_CASE("sync_write a register", "[write]") {
    using namespace groov::literals;
    data0 = 0;
    CHECK(sync_write(grp("reg0"_r = 0xa5a5'a5a5u)));
    CHECK(data0 == 0xa5a5'a5a5u);
}

TEST_CASE("sync_write with passthrough", "[write]") {
    using namespace groov::literals;
    data0 = 0;
    auto value = sync_write(grp("reg0"_r = 0xa5a5'a5a5u), 42, 17);
    CHECK(value);
    CHECK(get<0>(*value) == 42);
    CHECK(get<1>(*value) == 17);
    CHECK(data0 == 0xa5a5'a5a5u);
}

TEST_CASE("non-blocking sync_write is not nodiscard", "[write]") {
    using namespace groov::literals;
    data0 = 0;
    sync_write(grp("reg0"_r = 0xa5a5'a5a5u));
    CHECK(data0 == 0xa5a5'a5a5u);
}

TEST_CASE("write a field", "[write]") {
    using namespace groov::literals;
    data0 = 0b1'1010'1u;
    CHECK(sync_write(grp("reg0.field1"_f = 1)));
    CHECK(data0 == 0b1'0001'1u);
}

TEST_CASE("write a field (alt)", "[write]") {
    using namespace groov::literals;
    data0 = 0b1'1010'1u;
    CHECK(sync_write(grp("reg0"_r("field1"_f = 1))));
    CHECK(data0 == 0b1'0001'1u);
}

TEST_CASE("set a field", "[write]") {
    using namespace groov::literals;
    data0 = 0u;
    CHECK(sync_write(grp("reg0.field1"_f = groov::set)));
    CHECK(data0 == 0b1111'0u);
}

TEST_CASE("clear a field", "[write]") {
    using namespace groov::literals;
    data0 = 0b1'1111'1u;
    CHECK(sync_write(grp("reg0.field1"_f = groov::clear)));
    CHECK(data0 == 0b1'0000'1u);
}

TEST_CASE("write multiple fields to the same register", "[write]") {
    using namespace groov::literals;
    data0 = 0b1'1010'0u;
    bus::num_writes = 0;
    CHECK(sync_write(grp("reg0.field0"_f = 1, "reg0.field1"_f = 0b101u)));
    CHECK(bus::num_writes == 1);
    CHECK(data0 == 0b1'0101'1u);
}

TEST_CASE("write multiple fields to the same register (alt)", "[write]") {
    using namespace groov::literals;
    data0 = 0b1'1010'0u;
    bus::num_writes = 0;
    CHECK(sync_write(grp("reg0"_r("field0"_f = 1, "field1"_f = 0b101u))));
    CHECK(bus::num_writes == 1);
    CHECK(data0 == 0b1'0101'1u);
}

TEST_CASE("write multiple fields to different registers", "[write]") {
    using namespace groov::literals;
    data0 = 0b1'1010'0u;
    data1 = 0b1'1010'0u;
    bus::num_writes = 0;
    CHECK(sync_write(grp("reg0.field0"_f = 1, "reg1.field1"_f = 0b101u)));
    CHECK(bus::num_writes == 2);
    CHECK(data0 == 0b1'1010'1u);
    CHECK(data1 == 0b1'0101'0u);
}

TEST_CASE("write multiple fields to different registers (alt)", "[write]") {
    using namespace groov::literals;
    data0 = 0b1'1010'0u;
    data1 = 0b1'1010'0u;
    bus::num_writes = 0;
    CHECK(sync_write(
        grp("reg0"_r("field0"_f = 1), "reg1"_r("field1"_f = 0b101u))));
    CHECK(bus::num_writes == 2);
    CHECK(data0 == 0b1'1010'1u);
    CHECK(data1 == 0b1'0101'0u);
}

TEST_CASE("write multiple fields to each of multiple registers", "[write]") {
    using namespace groov::literals;
    data0 = 0b1'0000'0u;
    data1 = 0b1'0000'1u;
    bus::num_writes = 0;
    CHECK(sync_write(grp("reg0.field0"_f = 1, "reg0.field1"_f = 0b101u,
                         "reg1.field0"_f = 0, "reg1.field1"_f = 0b1010u)));
    CHECK(bus::num_writes == 2);
    CHECK(data0 == 0b1'0101'1u);
    CHECK(data1 == 0b1'1010'0u);
}

TEST_CASE("write multiple fields to each of multiple registers (alt)",
          "[write]") {
    using namespace groov::literals;
    data0 = 0b1'0000'0u;
    data1 = 0b1'0000'1u;
    bus::num_writes = 0;
    CHECK(sync_write(grp("reg0"_r("field0"_f = 1, "field1"_f = 0b101u),
                         "reg1"_r("field0"_f = 0, "field1"_f = 0b1010u))));
    CHECK(bus::num_writes == 2);
    CHECK(data0 == 0b1'0101'1u);
    CHECK(data1 == 0b1'1010'0u);
}

TEST_CASE("write mask is passed to bus", "[write]") {
    using namespace groov::literals;
    CHECK(sync_write(grp("reg0.field1"_f = 5)));
    CHECK(bus::last_mask == 0b1111'0u);
}

TEST_CASE("write is pipeable", "[write]") {
    using namespace groov::literals;
    data0 = 0;
    auto r = async::just(grp("reg0"_r = 0xa5a5'a5a5u)) | groov::write() |
             async::sync_wait();
    CHECK(r);
    CHECK(data0 == 0xa5a5'a5a5u);
}

TEST_CASE("write is pipeable with passthrough", "[write]") {
    using namespace groov::literals;
    data0 = 0;
    auto r = async::just(grp("reg0"_r = 0xa5a5'a5a5u)) | groov::write(42, 17) |
             async::sync_wait();
    CHECK(r);
    CHECK(get<0>(*r) == 42);
    CHECK(get<1>(*r) == 17);
    CHECK(data0 == 0xa5a5'a5a5u);
}

TEST_CASE("sync_write is pipeable", "[write]") {
    using namespace groov::literals;
    data0 = 0;
    auto r = async::just(grp("reg0"_r = 0xa5a5'a5a5u)) | groov::sync_write();
    CHECK(r);
    CHECK(data0 == 0xa5a5'a5a5u);
}

TEST_CASE("sync_write is pipeable with passthrough", "[write]") {
    using namespace groov::literals;
    data0 = 0;
    auto r =
        async::just(grp("reg0"_r = 0xa5a5'a5a5u)) | groov::sync_write(42, 17);
    CHECK(r);
    CHECK(get<0>(*r) == 42);
    CHECK(get<1>(*r) == 17);
    CHECK(data0 == 0xa5a5'a5a5u);
}

TEST_CASE("write is adaptor-pipeable", "[write]") {
    using namespace groov::literals;
    data0 = 0;
    auto s = groov::write() | async::then([] {});
    auto r = async::just(grp("reg0"_r = 0xa5a5'a5a5u)) | s | async::sync_wait();
    CHECK(r);
    CHECK(data0 == 0xa5a5'a5a5u);
}

TEST_CASE("piped read-modify-write", "[write]") {
    using namespace groov::literals;
    data0 = 0xa5u;
    auto r = async::just(grp / "reg0"_r) //
             | groov::read()             //
             | async::then([](auto spec) {
                   spec["reg0"_r] ^= 0xff;
                   return spec;
               })             //
             | groov::write() //
             | async::sync_wait();
    CHECK(r);
    CHECK(data0 == 0x5au);
}

namespace {
template <auto Addr> std::uint32_t data{};

struct ct_address_bus {
    template <stdx::ct_string, auto Mask, typename T>
    static auto read(T) -> async::sender auto {
        return async::just_result_of([=] { return data<T::value>; });
    }

    template <stdx::ct_string, auto Mask, auto IdMask, auto IdValue, typename T>
    static auto write(T, auto value) -> async::sender auto {
        return async::just_result_of([=] {
            auto prev = data<T::value> & ~(Mask | IdMask);
            data<T::value> = prev | value | IdValue;
        });
    }
};
} // namespace

TEST_CASE("write a register with integral constant address", "[write]") {
    using R =
        groov::reg<"reg", std::uint32_t, std::integral_constant<int, 42>{}>;
    constexpr auto g = groov::group<"group", ct_address_bus, R>{};

    using namespace groov::literals;
    CHECK(sync_write(g("reg"_r = 0xa5a5'a5a5u)));
    CHECK(data<42> == 0xa5a5'a5a5u);
}

namespace {
struct bus_u8 {
    struct dummy_sender {
        using is_sender = void;
    };
    template <stdx::ct_string, auto>
    static auto read(auto) -> async::sender auto {
        return dummy_sender{};
    }

    template <stdx::ct_string Name, auto Mask, auto IdMask, auto IdValue>
    static auto write(auto addr, auto value) -> async::sender auto {
        STATIC_REQUIRE(Name == stdx::ct_string{"reg"});
        return async::just_result_of([=] {
            auto prev = *addr & ~(Mask | IdMask);
            *addr = static_cast<std::uint8_t>(prev | value | IdValue);
        });
    }
};

std::uint8_t data_u8{};
using R_u8 =
    groov::reg<"reg", std::uint8_t, &data_u8, groov::w::replace, F0, F1, F2>;
using G_u8 = groov::group<"group", bus_u8, R_u8>;
constexpr auto grp_u8 = G_u8{};
} // namespace

TEST_CASE("write a std::uint8_t register", "[write]") {
    using namespace groov::literals;
    data_u8 = 0b1'1010'1u;
    CHECK(sync_write(grp_u8("reg.field1"_f = 1)));
    CHECK(data_u8 == 0b1'0001'1u);
}

namespace {
std::uint32_t data2{};
struct custom_write_fn {
    using id_spec = groov::m::one;
};

using R_partial =
    groov::reg<"reg2", std::uint32_t, &data2, custom_write_fn, F0, F1, F2>;

using G1 = groov::group<"group", bus, R_partial>;
} // namespace

TEST_CASE("write a partial register", "[write]") {
    using namespace groov::literals;
    data2 = 0b1'1010'1u;
    CHECK(sync_write(G1{}("reg2.field1"_f = 1)));
    CHECK(data2 == 0b1111'1111'1111'1111'1111'1111'001'0001'1u);
}

namespace {
enum struct E : std::uint8_t { A = 0, B = 1, C = 2 };
using F_enum = groov::field<"field", E, 1, 0>;

std::uint32_t data_enum{};
using R_enum =
    groov::reg<"reg", std::uint32_t, &data_enum, groov::w::replace, F_enum>;

using G_enum = groov::group<"group", bus, R_enum>;
constexpr auto grp_enum = G_enum{};
} // namespace

TEST_CASE("write an enum field", "[write]") {
    using namespace groov::literals;
    data_enum = 0u;
    CHECK(sync_write(grp_enum("reg.field"_f = E::C)));
    CHECK(data_enum == 0b10u);
}

TEST_CASE("set an enum field", "[write]") {
    using namespace groov::literals;
    data_enum = 0u;
    CHECK(sync_write(grp_enum("reg.field"_f = groov::set)));
    CHECK(data_enum == 0b11u);
}

TEST_CASE("clear an enum field", "[write]") {
    using namespace groov::literals;
    data_enum = 0b11u;
    CHECK(sync_write(grp_enum("reg.field"_f = groov::clear)));
    CHECK(data_enum == 0u);
}

namespace {
enum struct EnableTest : std::uint8_t { DISABLE = 0, ENABLE = 1 };
using F_enum_enable = groov::field<"field", EnableTest, 0, 0>;

using R_enum_enable = groov::reg<"reg", std::uint32_t, &data_enum,
                                 groov::w::replace, F_enum_enable>;

using G_enum_enable = groov::group<"group", bus, R_enum_enable>;
constexpr auto grp_enum_enable = G_enum_enable{};
} // namespace

TEST_CASE("enable an enum field", "[write]") {
    using namespace groov::literals;
    data_enum = 0u;
    CHECK(sync_write(grp_enum_enable("reg.field"_f = groov::enable)));
    CHECK(data_enum == 0b1u);
}

TEST_CASE("disable an enum field", "[write]") {
    using namespace groov::literals;
    data_enum = 1u;
    CHECK(sync_write(grp_enum_enable("reg.field"_f = groov::disable)));
    CHECK(data_enum == 0u);
}

TEST_CASE("enable with field_proxy", "[write]") {
    using namespace groov::literals;
    data_enum = 0u;
    auto r = async::just(grp_enum_enable / "reg"_r) //
             | groov::read()                        //
             | async::then([](auto spec) {
                   spec["reg.field"_r] = groov::enable;
                   return spec;
               })             //
             | groov::write() //
             | async::sync_wait();
    CHECK(r);
    CHECK(data_enum == 0b1u);
}

TEST_CASE("disable with field_proxy", "[write]") {
    using namespace groov::literals;
    data_enum = 1u;
    auto r = async::just(grp_enum_enable / "reg"_r) //
             | groov::read()                        //
             | async::then([](auto spec) {
                   spec["reg.field"_r] = groov::disable;
                   return spec;
               })             //
             | groov::write() //
             | async::sync_wait();
    CHECK(r);
    CHECK(data_enum == 0u);
}

namespace {
std::uint32_t data_fn{};
}

TEST_CASE("write a register with address function", "[write]") {
    using R = groov::reg<"reg", std::uint32_t, [] { return &data_fn; }>;
    constexpr auto g = groov::group<"group", bus, R>{};

    using namespace groov::literals;
    CHECK(sync_write(g("reg"_r = 0xa5a5'a5a5u)));
    CHECK(data_fn == 0xa5a5'a5a5u);
}

TEST_CASE("set with field_proxy", "[write]") {
    using namespace groov::literals;
    data0 = 0u;
    auto r = async::just(grp / "reg0"_r) //
             | groov::read()             //
             | async::then([](auto spec) {
                   spec["reg0.field1"_r] = groov::set;
                   return spec;
               })             //
             | groov::write() //
             | async::sync_wait();
    CHECK(r);
    CHECK(data0 == 0b1111'0u);
}

TEST_CASE("clear with field_proxy", "[write]") {
    using namespace groov::literals;
    data0 = 0b1'1111'1u;
    auto r = async::just(grp / "reg0"_r) //
             | groov::read()             //
             | async::then([](auto spec) {
                   spec["reg0.field1"_r] = groov::clear;
                   return spec;
               })             //
             | groov::write() //
             | async::sync_wait();
    CHECK(r);
    CHECK(data0 == 0b1'0000'1u);
}
