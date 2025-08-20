#include <groov/path.hpp>
#include <groov/read.hpp>
#include <groov/test.hpp>
#include <groov/value_path.hpp>
#include <groov/write.hpp>
#include <groov/write_spec.hpp>

#include <async/just.hpp>
#include <async/sync_wait.hpp>
#include <async/then.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("value get (correct type)", "[test_bus]") {
    groov::test::value v{42};
    auto o = v.get<int>();
    REQUIRE(o);
    CHECK(*o == 42);
}

TEST_CASE("value get (wrong type)", "[test_bus]") {
    groov::test::value v{42};
    auto o = v.get<bool>();
    CHECK(not o);
}

TEST_CASE("value equality", "[test_bus]") {
    groov::test::value v{42};
    CHECK(v == groov::test::value{42});
    CHECK(v != groov::test::value{17});
    CHECK(v != groov::test::value{true});
}

TEST_CASE("test bus read (X-value)", "[test_bus]") {
    groov::test::bus<>::reset();
    groov::test::bus<> b{};
    auto r = b.read<1>(1) | async::sync_wait();
    REQUIRE(r);
    CHECK(not get<0>(*r));
}

TEST_CASE("test bus read with alternate XPolicy", "[test_bus]") {
    using P = decltype([]<auto>(auto addr, auto value) {
        if (not value) {
            throw addr;
        }
        return value;
    });
    groov::test::bus<P>::reset();
    groov::test::bus<P> b{};
    try {
        [[maybe_unused]] auto r = b.read<1>(1) | async::sync_wait();
    } catch (int &addr) {
        CHECK(addr == 1);
    }
}

TEST_CASE("test bus write", "[test_bus]") {
    groov::test::bus<>::reset();
    groov::test::bus<> b{};
    auto r = b.write<1, 0, 0>(1, 42) | async::sync_wait();
    REQUIRE(r);
}

TEST_CASE("test bus write and read back", "[test_bus]") {
    groov::test::bus<>::reset();
    groov::test::bus<> b{};
    REQUIRE(b.write<1, 0, 0>(1, 42) | async::sync_wait());
    auto r = b.read<1>(1) | async::sync_wait();
    REQUIRE(r);
    auto opt_val = get<0>(*r);
    REQUIRE(opt_val);
    CHECK(*opt_val == 42);
}

namespace {
using R = groov::reg<"reg", std::uint32_t, 0xcafe, groov::w::replace>;
using G = groov::group<"group", groov::test::bus<>, R>;
constexpr auto grp = G{};
} // namespace

TEST_CASE("write then read back", "[test_bus]") {
    using namespace groov::literals;
    groov::test::bus<>::reset();

    CHECK(sync_write(grp("reg"_r = 0xa5a5'a5a5u)));
    auto r = sync_read(grp / "reg"_r);
    REQUIRE(r);
    CHECK((*r)["reg"_r] == 0xa5a5'a5a5u);
}

TEST_CASE("read-modify-write", "[test_bus]") {
    using namespace groov::literals;
    groov::test::bus<>::reset();

    CHECK(sync_write(grp("reg"_r = 0xa5a5'a5a5u)));
    auto rmw = async::just(grp / "reg"_r) //
               | groov::read()            //
               | async::then([](auto spec) {
                     REQUIRE(spec);
                     (*spec)["reg"_r] ^= 0xffff'ffffu;
                     return *spec;
                 })             //
               | groov::write() //
               | async::sync_wait();
    REQUIRE(rmw);

    auto r = sync_read(grp / "reg"_r);
    REQUIRE(r);
    CHECK((*r)["reg"_r] == 0x5a5a'5a5au);
}
