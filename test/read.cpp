#include <groov/path.hpp>
#include <groov/read.hpp>
#include <groov/read_spec.hpp>

#include <async/concepts.hpp>
#include <async/just.hpp>
#include <async/just_result_of.hpp>
#include <async/sync_wait.hpp>
#include <async/then.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>

namespace {
struct bus {
    static inline int num_reads{};
    static inline std::uint32_t last_mask{};

    template <stdx::ct_string, auto Mask>
    static auto read(auto addr) -> async::sender auto {
        return async::just_result_of([=] {
            ++num_reads;
            last_mask = Mask;
            return *addr;
        });
    }

    struct dummy_sender {
        using is_sender = void;
    };
    template <stdx::ct_string, auto...>
    static auto write(auto...) -> async::sender auto {
        return dummy_sender{};
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

TEST_CASE("read a register", "[read]") {
    using namespace groov::literals;
    data0 = 0xa5a5'a5a5u;
    auto s = read(grp / "reg0"_r);
    auto r = get<0>(*(s | async::sync_wait()));
    CHECK(r["reg0"_r] == data0);
}

TEST_CASE("sync_read a register", "[read]") {
    using namespace groov::literals;
    data0 = 0xa5a5'a5a5u;
    auto r = sync_read(grp / "reg0"_r);
    CHECK(r["reg0"_r] == data0);
}

TEST_CASE("read a field", "[read]") {
    using namespace groov::literals;
    data0 = 0b10101u;
    auto r = sync_read(grp / "reg0.field1"_f);
    CHECK(r["reg0.field1"_f] == 0b1010u);
}

TEST_CASE("extract a field after reading whole register", "[read]") {
    using namespace groov::literals;
    data0 = 0b10101u;

    auto r = sync_read(grp / "reg0"_r);
    CHECK(r["reg0"_r] == data0);
    CHECK(r["reg0.field1"_f] == 0b1010u);
}

TEST_CASE("read result allows lookup by unambiguous path", "[read]") {
    using namespace groov::literals;
    data0 = 0b10101u;

    auto r = sync_read(grp / "reg0.field1"_r);
    CHECK(r["reg0.field1"_f] == 0b1010u);
    CHECK(r["field1"_f] == 0b1010u);
}

TEST_CASE("extract a field by unambiguous path after reading whole register",
          "[read]") {
    using namespace groov::literals;
    data0 = 0b10101u;

    auto r = sync_read(grp / "reg0"_r);
    CHECK(r["reg0"_r] == data0);
    CHECK(r["field1"_f] == 0b1010u);
}

TEST_CASE("single read result allows implicit conversion", "[read]") {
    using namespace groov::literals;
    data0 = 0b10101u;

    auto r = sync_read(grp / "reg0.field1"_r);
    CHECK([](std::uint8_t value) { return value == 0b1010u; }(r));
}

TEST_CASE("read multiple fields from the same register", "[read]") {
    using namespace groov::literals;
    data0 = 0b111'1010'1u;

    bus::num_reads = 0;
    auto r = sync_read(grp("reg0.field0"_r, "reg0.field1"_r));
    CHECK(bus::num_reads == 1);

    CHECK(r["reg0.field1"_f] == 0b1010u);
    CHECK(r["field1"_f] == 0b1010u);
    CHECK(r["reg0.field0"_f] == 1u);
    CHECK(r["field0"_f] == 1u);
}

TEST_CASE("read multiple fields from different registers", "[read]") {
    using namespace groov::literals;
    data0 = 0b111'1010'0u;
    data1 = 0b111'0101'1u;

    bus::num_reads = 0;
    auto r = sync_read(grp("reg0.field1"_r, "reg1.field0"_r));
    CHECK(bus::num_reads == 2);

    CHECK(r["reg0.field1"_f] == 0b1010u);
    CHECK(r["field1"_f] == 0b1010u);
    CHECK(r["reg1.field0"_f] == 0b1);
    CHECK(r["field0"_f] == 0b1);
}

TEST_CASE("read multiple fields from each of multiple registers", "[read]") {
    using namespace groov::literals;
    data0 = 0b111'1010'0u;
    data1 = 0b111'0101'1u;

    bus::num_reads = 0;
    auto r = sync_read(grp("reg0.field0"_f, "reg1.field0"_f, "reg0.field1"_f,
                           "reg1.field1"_f));
    CHECK(bus::num_reads == 2);

    CHECK(r["reg0.field0"_f] == 0);
    CHECK(r["reg0.field1"_f] == 0b1010u);
    CHECK(r["reg1.field0"_f] == 1u);
    CHECK(r["reg1.field1"_f] == 0b0101u);
}

TEST_CASE("read multiple registers then get fields", "[read]") {
    using namespace groov::literals;
    data0 = 0b111'1010'0u;
    data1 = 0b111'0101'1u;

    bus::num_reads = 0;
    auto r = sync_read(grp("reg0"_f, "reg1"_f));
    CHECK(bus::num_reads == 2);

    CHECK(r["reg0"_f] == data0);
    CHECK(r["reg1"_f] == data1);
    CHECK(r["reg0.field0"_f] == 0);
    CHECK(r["reg0.field1"_f] == 0b1010u);
    CHECK(r["reg1.field0"_f] == 1u);
    CHECK(r["reg1.field1"_f] == 0b0101u);
}

TEST_CASE("read mask is passed to bus", "[read]") {
    using namespace groov::literals;
    [[maybe_unused]] auto r = sync_read(grp("reg0.field0"_f, "reg0.field2"_f));
    CHECK(bus::last_mask == 0b111'0000'1u);
}

TEST_CASE("read is pipeable", "[read]") {
    using namespace groov::literals;
    data0 = 0xa5a5'a5a5u;
    auto r = async::just(grp / "reg0"_r) | groov::read() | async::sync_wait();
    CHECK(get<0>(*r)["reg0"_r] == data0);
}

TEST_CASE("sync_read is pipeable", "[read]") {
    using namespace groov::literals;
    data0 = 0xa5a5'a5a5u;
    auto r = async::just(grp / "reg0"_r) | groov::sync_read();
    CHECK(r["reg0"_r] == data0);
}

TEST_CASE("read is adaptor-pipeable", "[read]") {
    using namespace groov::literals;
    data0 = 0xa5a5'a5a5u;
    auto s = groov::read() | async::then([](auto spec) { return spec; });
    auto r = async::just(grp / "reg0"_r) | s | async::sync_wait();
    CHECK(get<0>(*r)["reg0"_r] == data0);
}

namespace {
template <auto Addr> std::uint32_t data{};

struct ct_address_bus {
    template <stdx::ct_string, auto Mask, typename T>
    static auto read(T) -> async::sender auto {
        return async::just_result_of([=] { return data<T::value>; });
    }

    struct dummy_sender {
        using is_sender = void;
    };
    template <stdx::ct_string, auto...>
    static auto write(auto...) -> async::sender auto {
        return dummy_sender{};
    }
};
} // namespace

TEST_CASE("read a register with integral constant address", "[read]") {
    using R =
        groov::reg<"reg", std::uint32_t, std::integral_constant<int, 42>{}>;
    constexpr auto g = groov::group<"group", ct_address_bus, R>{};

    using namespace groov::literals;
    data<42> = 0xa5a5'a5a5u;
    auto r = sync_read(g / "reg"_r);
    CHECK(r["reg"_r] == data<42>);
}

namespace {
struct bus_u8 {
    template <stdx::ct_string Name, auto Mask>
    static auto read(auto addr) -> async::sender auto {
        STATIC_REQUIRE(Name == stdx::ct_string{"reg"});
        return async::just_result_of([=] { return *addr; });
    }

    struct dummy_sender {
        using is_sender = void;
    };
    template <stdx::ct_string, auto...>
    static auto write(auto...) -> async::sender auto {
        return dummy_sender{};
    }
};

std::uint8_t data_u8{};
using R_u8 =
    groov::reg<"reg", std::uint8_t, &data_u8, groov::w::replace, F0, F1, F2>;
using G_u8 = groov::group<"group", bus_u8, R_u8>;
constexpr auto grp_u8 = G_u8{};
} // namespace

TEST_CASE("read a std::uint8_t register", "[read]") {
    using namespace groov::literals;
    data_u8 = 0xa5u;
    auto r = sync_read(grp_u8 / "reg"_r);
    CHECK(r["reg"_r] == data_u8);
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

TEST_CASE("read an enum field", "[read]") {
    using namespace groov::literals;
    data_enum = 0b10u;
    auto r = sync_read(grp_enum / "reg"_r);
    CHECK(r["reg.field"_r] == E::C);
}

namespace {
struct be_bus {
    template <stdx::ct_string, auto, auto, auto>
    static auto write(auto addr, auto) -> async::sender auto {
        return async::just_result_of([=] { *addr = 42; });
    }

    template <stdx::ct_string, auto>
    static auto read(auto) -> async::sender auto {
        return async::just_result_of([] { return 42; });
    }

    template <typename RegType>
    CONSTEVAL static auto transform_mask(RegType mask) -> RegType {
        using A = std::array<std::uint8_t, sizeof(RegType)>;
        auto arr = stdx::bit_cast<A>(mask);
        for (auto &i : arr) {
            i = i == 0 ? 0u : 0xffu;
        }
        return stdx::bit_cast<RegType>(arr);
    }
};

using FB0 = groov::field<"field0", std::uint8_t, 7, 0>;
using FB1_WO = groov::field<"field1", std::uint8_t, 8, 8,
                            groov::write_only<groov::w::replace>>;

std::uint32_t data3{};
using R3 = groov::reg<"reg3", std::uint32_t, &data3,
                      groov::write_only<groov::w::replace>, FB0>;
using R4 =
    groov::reg<"reg4", std::uint32_t, &data3, groov::w::replace, FB0, FB1_WO>;

using GBE = groov::group<"group", be_bus, R3, R4>;
constexpr auto grp_be = GBE{};
} // namespace

TEST_CASE("read a field in a WO register under a byte-enable bus", "[read]") {
    using namespace groov::literals;
    auto r = sync_read(grp_be / "reg3.field0"_r);
    CHECK(r["reg3.field0"_r] == 42);
}

TEST_CASE("read a field in a register containing WO fields which won't be read "
          "under a byte-enable bus",
          "[read]") {
    using namespace groov::literals;
    auto r = sync_read(grp_be / "reg4.field0"_r);
    CHECK(r["reg4.field0"_r] == 42);
}
