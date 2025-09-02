#include <groov/test.hpp>

#include <cstdint>

//----------------------------------------------------------------
namespace {
struct my_test_bus;
}
namespace groov::test {

using test_bus_list = make_test_bus_list<default_test_bus<"test_group_0">,
                                         test_bus<"test_group_1", my_test_bus>>;
} // namespace groov::test
//----------------------------------------------------------------

#include <groov/groov.hpp>

#include <catch2/catch_test_macros.hpp>

namespace {
struct my_test_bus {
    static inline int num_reads{};
    static inline int num_writes{};
    template <auto Mask> static auto read(auto addr) -> async::sender auto {
        return async::just_result_of([=] {
            ++num_reads;
            return *addr;
        });
    }

    template <auto...>
    static auto write(auto addr, auto value) -> async::sender auto {
        return async::just_result_of([=] {
            ++num_writes;
            *addr = value;
        });
    }
};

struct my_bus {
    static inline int num_reads{};
    static inline int num_writes{};
    template <auto Mask> static auto read(auto addr) -> async::sender auto {
        return async::just_result_of([=] {
            ++num_reads;
            return *addr;
        });
    }

    template <auto...> static auto write(auto...) -> async::sender auto {
        return async::just_result_of([=] { ++num_writes; });
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

using G0 = groov::group<"test_group_0", my_bus, R0, R1>;
constexpr auto grp0 = G0{};

using G1 = groov::group<"test_group_1", my_bus, R0, R1>;
constexpr auto grp1 = G1{};

} // namespace

TEST_CASE("read a test_bus register", "[test]") {
    using namespace groov::literals;

    my_bus::num_reads = 0;
    data0 = 0xa5a5'a5a5u;

    groov::test::reset_store<G0>();
    groov::test::set_value<G0>("reg0"_r, 0x5a5a5a5a);

    auto r = groov::sync_read(grp0 / "reg0"_r);

    CHECK(data0 == 0xa5a5'a5a5u);
    CHECK(my_bus::num_reads == 0);
    REQUIRE(r);
    CHECK((*r)["reg0"_r] == 0x5a5a'5a5au);
}

TEST_CASE("write a test_bus register", "[test]") {
    using namespace groov::literals;

    my_bus::num_reads = 0;
    my_bus::num_writes = 0;
    data0 = 0xa5a5'a5a5u;
    groov::test::reset_store<G0>();
    groov::test::set_value<G0>("reg0"_r, 0xdeadbeef);

    groov::sync_write(grp0("reg0.field1"_f = 0x8));

    CHECK(data0 == 0xa5a5'a5a5u);
    CHECK(my_bus::num_reads == 0);
    CHECK(my_bus::num_writes == 0);
    auto v = groov::test::get_value<G0>("reg0"_f);
    REQUIRE(v);
    CHECK(*v == 0xdeadbef1u);
}

TEST_CASE("test_bus read function", "[test]") {
    using namespace groov::literals;

    int read_call_count = 0;
    void *read_addr = 0;

    my_bus::num_reads = 0;
    my_bus::num_writes = 0;
    data0 = 0xa5a5'a5a5u;
    groov::test::reset_store<G0>();
    groov::test::set_value<G0>("reg0"_r, 0xdeadbeef);
    groov::test::set_read_function<G0>(
        "reg0"_r, [&read_call_count, &read_addr](auto read_ptr) {
            ++read_call_count;
            read_addr = get<std::uint32_t *>(read_ptr).value_or(nullptr);
            return 0xbabeface;
        });

    auto r = groov::sync_read(grp0 / "reg0.field1"_f);

    CHECK(data0 == 0xa5a5'a5a5u);
    CHECK(my_bus::num_reads == 0);
    CHECK(my_bus::num_writes == 0);
    CHECK(read_call_count == 1);
    auto v = groov::test::get_value<G0>("reg0"_r);
    REQUIRE(v);
    CHECK(*v == 0xbabefaceu);
    REQUIRE(r);
    CHECK((*r)["reg0.field1"_r] == 0x7);
    CHECK(read_addr == &data0);
}

TEST_CASE("test_bus write function", "[test]") {
    using namespace groov::literals;

    int write_call_count = 0;
    void *write_addr = 0;
    std::uint32_t write_value = 0;

    my_bus::num_reads = 0;
    my_bus::num_writes = 0;
    data0 = 0xa5a5'a5a5u;
    groov::test::reset_store<G0>();
    groov::test::set_value<G0>("reg0"_r, 0xbabeface);
    groov::test::set_write_function<G0>("reg0"_r, [&](auto addr, auto value) {
        ++write_call_count;
        write_addr = get<std::uint32_t *>(addr).value_or(nullptr);
        write_value = get<std::uint32_t>(value).value_or(0);
    });

    groov::sync_write(grp0("reg0"_r = 0x76));

    CHECK(data0 == 0xa5a5'a5a5u);
    CHECK(my_bus::num_reads == 0);
    CHECK(my_bus::num_writes == 0);
    CHECK(write_call_count == 1);
    CHECK(write_addr == &data0);
    CHECK(write_value == 0xbabefa76u);
    auto v = groov::test::get_value<G0>("reg0"_f);
    REQUIRE(v);
    CHECK(*v == 0xbabefaceu);
}

TEST_CASE("test_bus check reset", "[test]") {
    using namespace groov::literals;

    int read_call_count = 0;
    void *read_addr = 0;

    my_bus::num_reads = 0;
    my_bus::num_writes = 0;
    data0 = 0xa5a5'a5a5u;
    groov::test::reset_store(grp0);
    groov::test::set_value(grp0, "reg0"_r, 0xa5a5a5a5);
    groov::test::set_read_function(
        grp0, "reg0"_r, [&read_call_count, &read_addr](auto read_ptr) {
            ++read_call_count;
            read_addr = get<std::uint32_t *>(read_ptr).value_or(nullptr);
            return 0xbabeface;
        });

    auto r0 = groov::sync_read(grp0 / "reg0.field1"_r);

    groov::test::reset_store(grp0);
    groov::test::set_value(grp0, "reg0"_r, 0xa5a5a5a5u);

    auto r1 = groov::sync_read(grp0 / "reg0.field1"_f);

    CHECK(data0 == 0xa5a5'a5a5u);
    CHECK(my_bus::num_reads == 0);
    CHECK(my_bus::num_writes == 0);
    CHECK(read_call_count == 1);
    CHECK(read_addr == &data0);
    REQUIRE(r0);
    CHECK((*r0)["reg0.field1"_r] == 0x7);
    REQUIRE(r1);
    CHECK((*r1)["reg0.field1"_r] == 0x2);
}

TEST_CASE("inject user test bus", "[test]") {
    using namespace groov::literals;

    my_bus::num_reads = 0;
    my_bus::num_writes = 0;
    my_test_bus::num_reads = 0;
    my_test_bus::num_writes = 0;
    data0 = 0xa5a5'a5a5u;

    auto r0 = groov::sync_read(grp1 / "reg0"_r);
    groov::sync_write(grp1("reg0.field1"_f = 0xf));
    auto r1 = groov::sync_read(grp1 / "reg0"_r);

    CHECK(my_bus::num_reads == 0);
    CHECK(my_bus::num_writes == 0);
    CHECK(my_test_bus::num_reads == 2);
    CHECK(my_test_bus::num_writes == 1);
    CHECK(r0["reg0"_r] == 0xa5a5a5a5u);
    CHECK(r1["reg0"_r] == 0x1e);
}
