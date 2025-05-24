#include <groov/path.hpp>

#include <catch2/catch_test_macros.hpp>

#include <type_traits>

TEST_CASE("register literal", "[path]") {
    using namespace groov::literals;
    constexpr auto r = "reg"_r;
    STATIC_REQUIRE(std::is_same_v<decltype(r), groov::path<"reg"> const>);
}

TEST_CASE("field literal", "[path]") {
    using namespace groov::literals;
    constexpr auto f = "field"_f;
    STATIC_REQUIRE(std::is_same_v<decltype(f), groov::path<"field"> const>);
}

TEST_CASE("dot-separated literal", "[path]") {
    using namespace groov::literals;
    constexpr auto f = "a.b.c.d"_f;
    STATIC_REQUIRE(
        std::is_same_v<decltype(f), groov::path<"a", "b", "c", "d"> const>);
}

TEST_CASE("path concatenation with /", "[path]") {
    using namespace groov::literals;
    constexpr auto p = "reg"_r / "field"_f;
    STATIC_REQUIRE(p == "reg.field"_f);
}

TEST_CASE("path can resolve itself", "[path]") {
    using namespace groov::literals;
    constexpr auto p = "a"_r;
    STATIC_REQUIRE(groov::is_resolvable_v<decltype(p), decltype(p)>);
}

TEST_CASE("path resolves itself to empty path", "[path]") {
    using namespace groov::literals;
    constexpr auto p = "a"_r;
    constexpr auto r = groov::resolve(p, p);
    STATIC_REQUIRE(std::is_same_v<decltype(r), groov::path<> const>);
}

TEST_CASE("path resolves a shorter path", "[path]") {
    using namespace groov::literals;
    constexpr auto p = "a.b.c"_r;
    constexpr auto r1 = groov::resolve(p, "a"_r);
    STATIC_REQUIRE(r1 == "b.c"_r);
    constexpr auto r2 = groov::resolve(p, "a.b"_r);
    STATIC_REQUIRE(r2 == "c"_r);
}

TEST_CASE("path doesn't resolve a non-path", "[path]") {
    using namespace groov::literals;
    constexpr auto p = "a.b.c"_r;
    STATIC_REQUIRE(not groov::can_resolve<decltype(p), int>);
}

TEST_CASE("mismatched path gives invalid resolution", "[path]") {
    using namespace groov::literals;
    constexpr auto p = "a.b.c"_r;
    STATIC_REQUIRE(std::is_same_v<groov::mismatch_t,
                                  decltype(groov::resolve(p, "invalid"_r))>);
}

TEST_CASE("too-long path gives invalid resolution", "[path]") {
    using namespace groov::literals;
    constexpr auto p = "a.b"_r;
    STATIC_REQUIRE(std::is_same_v<groov::too_long_t,
                                  decltype(groov::resolve(p, "a.b.c"_r))>);
}

TEST_CASE("root of a path", "[path]") {
    using namespace groov::literals;
    constexpr auto p = "a.b.c"_r;
    STATIC_REQUIRE(root(p) == stdx::ct_string{"a"});
}

TEST_CASE("path without its root", "[path]") {
    using namespace groov::literals;
    constexpr auto p = "a.b.c"_r;
    STATIC_REQUIRE(without_root(p) == "b.c"_r);
}

TEST_CASE("empty path predicate", "[path]") {
    using P = groov::path<>;
    STATIC_REQUIRE(std::empty(P{}));
    using Q = groov::path<"hello">;
    STATIC_REQUIRE(not std::empty(Q{}));
}

TEST_CASE("path is pathlike", "[path]") {
    using namespace groov::literals;
    STATIC_REQUIRE(groov::pathlike<decltype("reg"_r / "field"_f)>);
    STATIC_REQUIRE(not groov::valued_pathlike<decltype("reg"_r / "field"_f)>);
}
