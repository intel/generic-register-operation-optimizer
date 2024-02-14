#include <async/concepts.hpp>
#include <async/just_result_of.hpp>
#include <async/sync_wait.hpp>

#include <groov/tree.hpp>
#include <groov/path.hpp>
#include <groov/read.hpp>

#include <catch2/catch_test_macros.hpp>

#include <concepts>
#include <cstdint>
#include <type_traits>

namespace {
struct bus {
    static inline int num_reads{};
    static inline std::uint32_t last_mask{};

    template <auto Mask> static auto read(auto addr) -> async::sender auto {
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
using R0 = groov::reg<"reg0", std::uint32_t, &data0, F0, F1, F2>;
std::uint32_t data1{};
using R1 = groov::reg<"reg1", std::uint32_t, &data1, F0, F1, F2>;

using G = groov::group<"group", bus, R0, R1>;
constexpr auto grp = G{};

template <typename T> auto sync_read(T const &t) {
    return get<0>(*(groov::read(t) | async::sync_wait()));
}
} // namespace

TEST_CASE("read a register", "[read]") {
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
