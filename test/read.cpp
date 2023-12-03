#include <async/concepts.hpp>
#include <async/just_result_of.hpp>
#include <async/sync_wait.hpp>

#include <groov/object.hpp>
#include <groov/path.hpp>
#include <groov/read.hpp>

#include <catch2/catch_test_macros.hpp>

#include <concepts>
#include <cstdint>
#include <type_traits>

namespace {
struct bus {
    static inline int num_reads{};

    static async::sender auto read(std::uint32_t *addr) {
        return async::just_result_of([=] {
            ++num_reads;
            return *addr;
        });
    }
};

using F0 = groov::field<"field0", std::uint8_t, 0, 0>;
using F1 = groov::field<"field1", std::uint8_t, 4, 1>;
using F2 = groov::field<"field2", std::uint8_t, 7, 5>;

std::uint32_t data0{};
using R0 = groov::reg<"reg0", std::uint32_t, &data0, F0, F1, F2>;
std::uint32_t data1{};
using R1 = groov::reg<"reg1", std::uint32_t, &data1, F0, F1, F2>;

using G = groov::group<"group", bus, R0, R1>;
} // namespace

TEST_CASE("extract a field", "[read]") {
    auto const x = std::uint32_t{0b111'1010'1u};
    CHECK(groov::extract<G, "reg0">(x) == x);
    CHECK(groov::extract<G, "reg0", "field0">(x) == 1u);
    CHECK(groov::extract<G, "reg0", "field1">(x) == 0b1010u);
    CHECK(groov::extract<G, "reg0", "field2">(x) == 0b111u);
}

TEST_CASE("read a field", "[read]") {
    using namespace groov::literals;
    data0 = 0b10101u;

    constexpr auto grp = G{};
    auto r = groov::read(grp / "reg0.field1"_f);
    auto const result = async::sync_wait(r);

    REQUIRE(result.has_value());
    CHECK(get<0>(*result).value == data0);
}

TEST_CASE("read a register", "[read]") {
    using namespace groov::literals;
    data0 = 0xa5a5'a5a5u;

    constexpr auto grp = G{};
    auto r = groov::read(grp / "reg0"_r);
    auto const result = async::sync_wait(r);

    REQUIRE(result.has_value());
    CHECK(get<0>(*result).value == data0);
    CHECK(get<0>(*result)["reg0"_r] == data0);
}

TEST_CASE("extract a field after reading whole register", "[read]") {
    using namespace groov::literals;
    data0 = 0b10101u;

    constexpr auto grp = G{};
    auto r = groov::read(grp / "reg0"_r);
    auto const result = get<0>(*async::sync_wait(r));

    CHECK(result["reg0"_r] == data0);
    CHECK(result["reg0.field1"_f] == 0b1010u);
}

TEST_CASE("extract a field after reading whole register and (redundant) field",
          "[read]") {
    using namespace groov::literals;
    data0 = 0b10101u;

    constexpr auto grp = G{};
    auto r = groov::read(grp / "reg0"_r, grp / "reg0.field0"_r);
    auto const result = get<0>(*async::sync_wait(r));

    CHECK(result["reg0"_r] == data0);
    CHECK(result["reg0.field0"_f] == 0b1u);
    CHECK(result["reg0.field1"_f] == 0b1010u);
}

TEST_CASE("read result allows lookup by unambiguous path", "[read]") {
    using namespace groov::literals;
    data0 = 0b10101u;

    constexpr auto grp = G{};
    auto r = groov::read(grp / "reg0.field1"_f);
    auto const result = get<0>(*async::sync_wait(r));
    CHECK(result["reg0.field1"_f] == 0b1010u);
    CHECK(result["field1"_f] == 0b1010u);
}

TEST_CASE("read result allows implicit conversion to field type", "[read]") {
    using namespace groov::literals;
    data0 = 0b10101u;

    constexpr auto grp = G{};
    auto r = groov::read(grp / "reg0.field1"_f);
    auto const result = get<0>(*async::sync_wait(r));
    CHECK([](std::uint8_t value) { return value == 0b1010u; }(result));
}

TEST_CASE("read multiple fields from the same register", "[read]") {
    using namespace groov::literals;
    data0 = 0b111'1010'1u;

    constexpr auto grp = G{};
    auto r = groov::read(grp / "reg0.field1"_f, grp / "reg0.field0"_f);
    bus::num_reads = 0;
    auto const result = get<0>(*async::sync_wait(r));

    CHECK(bus::num_reads == 1);
    CHECK(result["reg0.field1"_f] == 0b1010u);
    CHECK(result["field1"_f] == 0b1010u);
    CHECK(result["reg0.field0"_f] == 1u);
    CHECK(result["field0"_f] == 1u);
}

TEST_CASE("read multiple fields from different registers", "[read]") {
    using namespace groov::literals;
    data0 = 0b111'1010'0u;
    data1 = 0b111'0101'1u;

    constexpr auto grp = G{};
    auto r = groov::read(grp / "reg0.field1"_f, grp / "reg1.field0"_f);
    bus::num_reads = 0;
    auto const result = get<0>(*async::sync_wait(r));

    CHECK(bus::num_reads == 2);
    CHECK(result["reg0.field1"_f] == 0b1010u);
    CHECK(result["field1"_f] == 0b1010u);
    CHECK(result["reg1.field0"_f] == 0b1);
    CHECK(result["field0"_f] == 0b1);
}

TEST_CASE("read multiple fields from each of multiple registers", "[read]") {
    using namespace groov::literals;
    data0 = 0b111'1010'0u;
    data1 = 0b111'0101'1u;

    constexpr auto grp = G{};
    auto r = groov::read(grp / "reg0.field0"_f, grp / "reg1.field0"_f,
                         grp / "reg0.field1"_f, grp / "reg1.field1"_f);
    bus::num_reads = 0;
    auto const result = get<0>(*async::sync_wait(r));

    CHECK(bus::num_reads == 2);
    CHECK(result["reg0.field0"_f] == 0);
    CHECK(result["reg0.field1"_f] == 0b1010u);
    CHECK(result["reg1.field0"_f] == 1u);
    CHECK(result["reg1.field1"_f] == 0b0101u);
}

TEST_CASE("read multiple registers then get fields", "[read]") {
    using namespace groov::literals;
    data0 = 0b111'1010'0u;
    data1 = 0b111'0101'1u;

    constexpr auto grp = G{};
    auto r = groov::read(grp / "reg0"_r, grp / "reg1"_r);
    bus::num_reads = 0;
    auto const result = get<0>(*async::sync_wait(r));

    CHECK(bus::num_reads == 2);
    CHECK(result["reg0"_f] == data0);
    CHECK(result["reg1"_f] == data1);
    CHECK(result["reg0.field0"_f] == 0);
    CHECK(result["reg0.field1"_f] == 0b1010u);
    CHECK(result["reg1.field0"_f] == 1u);
    CHECK(result["reg1.field1"_f] == 0b0101u);
}

TEST_CASE("read multiple registers plus redundant field, then get fields",
          "[read]") {
    using namespace groov::literals;
    data0 = 0b111'1010'0u;
    data1 = 0b111'0101'1u;

    constexpr auto grp = G{};
    auto r = groov::read(grp / "reg0"_r, grp / "reg1"_r, grp / "reg1.field1"_f);
    bus::num_reads = 0;
    auto const result = get<0>(*async::sync_wait(r));

    CHECK(bus::num_reads == 2);
    CHECK(result["reg0"_f] == data0);
    CHECK(result["reg1"_f] == data1);
    CHECK(result["reg0.field0"_f] == 0);
    CHECK(result["reg0.field1"_f] == 0b1010u);
    CHECK(result["reg1.field0"_f] == 1u);
    CHECK(result["reg1.field1"_f] == 0b0101u);
}
