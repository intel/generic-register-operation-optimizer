#include <groov/config.hpp>
#include <groov/path.hpp>
#include <groov/resolve.hpp>
#include <groov/value_path.hpp>
#include <groov/write_spec.hpp>

#include <async/concepts.hpp>

#include <catch2/catch_test_macros.hpp>

namespace {
struct bus {
    struct sender {
        using is_sender = void;
    };

    template <auto> static auto read(auto...) -> async::sender auto {
        return sender{};
    }
    template <auto...> static auto write(auto...) -> async::sender auto {
        return sender{};
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

TEST_CASE("write_spec is made by combining group and paths", "[write_spec]") {
    using namespace groov::literals;
    auto spec = grp("reg0"_r = 5);
    STATIC_REQUIRE(
        stdx::is_specialization_of_v<decltype(spec), groov::write_spec>);
}

TEST_CASE("valid paths are captured", "[write_spec]") {
    using namespace groov::literals;
    auto p = "reg0"_r = 5;
    auto spec = grp(p);
    STATIC_REQUIRE(
        std::is_same_v<decltype(spec)::paths_t,
                       boost::mp11::mp_list<typename decltype(p)::path_t>>);
}

TEST_CASE("multiple paths can be passed", "[write_spec]") {
    using namespace groov::literals;
    auto p = "reg0"_r = 5;
    auto q = "reg1"_r = 6;
    auto spec = grp(p, q);
    STATIC_REQUIRE(
        std::is_same_v<decltype(spec)::paths_t,
                       boost::mp11::mp_list<typename decltype(p)::path_t,
                                            typename decltype(q)::path_t>>);
}

TEST_CASE("operator/ is overloaded to make write_spec", "[write_spec]") {
    using namespace groov::literals;
    auto p = "reg0"_r = 5;
    auto spec = grp / p;
    STATIC_REQUIRE(
        std::is_same_v<decltype(spec)::paths_t,
                       boost::mp11::mp_list<typename decltype(p)::path_t>>);
}

TEST_CASE("write spec can be indexed by path", "[write_spec]") {
    using namespace groov::literals;
    auto const spec = grp("reg0"_r = 5);
    CHECK(spec["reg0"_r] == 5);
}

TEST_CASE("write spec specified with whole reg can be indexed by field",
          "[write_spec]") {
    using namespace groov::literals;
    auto spec = grp("reg0"_r = 5);
    CHECK(spec["reg0.field0"_f] == 1);
}

TEST_CASE("write spec allows lookup by unambiguous path", "[write_spec]") {
    using namespace groov::literals;
    auto spec = (grp("reg0.field1"_r = 0b1010u));
    CHECK(spec["field1"_f] == 0b1010u);
}

TEST_CASE(
    "write spec specified with whole reg can be indexed by unambiguous field",
    "[write_spec]") {
    using namespace groov::literals;
    auto spec = grp("reg0"_r = 5);
    CHECK(spec["field0"_f] == 1);
}

TEST_CASE("write spec with only one value implicitly converts",
          "[write_spec]") {
    using namespace groov::literals;
    auto spec = grp("reg0"_r = 5);
    CHECK(spec == 5);
}

TEST_CASE("write spec indexing supports assignment", "[write_spec]") {
    using namespace groov::literals;
    auto spec = grp("reg0"_r = 5);
    spec["reg0"_r] = 4;
    CHECK(spec["reg0"_r] == 4);
}

TEST_CASE("write spec indexing supports op-assignment", "[write_spec]") {
    using namespace groov::literals;
    auto spec = grp("reg0"_r = 5);
    spec["reg0"_r] += 1;
    spec["reg0"_r] -= 1;
    spec["reg0"_r] *= 1;
    spec["reg0"_r] /= 1;
    spec["reg0"_r] %= 6;
    CHECK(spec["reg0"_r] == 5);
    spec["reg0"_r] &= 3;
    CHECK(spec["reg0"_r] == 1);
    spec["reg0"_r] |= 4;
    CHECK(spec["reg0"_r] == 5);
    spec["reg0"_r] ^= 4;
    CHECK(spec["reg0"_r] == 1);
}

TEST_CASE("write spec indexing supports increment/decrement", "[write_spec]") {
    using namespace groov::literals;
    auto spec = grp("reg0"_r = 5);
    ++spec["reg0"_r];
    --spec["reg0"_r];
    CHECK(spec["reg0"_r] == 5);

    CHECK(spec["reg0"_r]++ == 5);
    CHECK(spec["reg0"_r]-- == 6);
    CHECK(spec["reg0"_r] == 5);
}
