#include <groov/path.hpp>

#include <catch2/catch_test_macros.hpp>

#include <type_traits>

TEST_CASE("register literal", "[path]") {
    using namespace groov::literals;
    constexpr auto r = "reg"_r;
    STATIC_CHECK(std::is_same_v<decltype(r), groov::path<"reg"> const>);
}

TEST_CASE("field literal", "[path]") {
    using namespace groov::literals;
    constexpr auto f = "field"_f;
    STATIC_CHECK(std::is_same_v<decltype(f), groov::path<"field"> const>);
}

TEST_CASE("dot-separated literal", "[path]") {
    using namespace groov::literals;
    constexpr auto f = "a.b.c.d"_f;
    STATIC_CHECK(
        std::is_same_v<decltype(f), groov::path<"a", "b", "c", "d"> const>);
}

TEST_CASE("empty literal", "[path]") {
    using namespace groov::literals;
    constexpr auto r = ""_r;
    STATIC_CHECK(std::is_same_v<decltype(r), groov::path<> const>);
}

TEST_CASE("make_path", "[path]") {
    using namespace groov::literals;
    constexpr auto f = groov::make_path<"a.b.c.d">();
    STATIC_CHECK(
        std::is_same_v<decltype(f), groov::path<"a", "b", "c", "d"> const>);
}

TEST_CASE("path concatenation with /", "[path]") {
    using namespace groov::literals;
    constexpr auto p = "reg"_r / "field"_f;
    STATIC_CHECK(p == "reg.field"_f);
}

TEST_CASE("path can resolve itself", "[path]") {
    using namespace groov::literals;
    constexpr auto p = "a"_r;
    STATIC_CHECK(groov::is_resolvable_v<decltype(p), decltype(p)>);
}

TEST_CASE("path resolves itself to empty path", "[path]") {
    using namespace groov::literals;
    constexpr auto p = "a"_r;
    constexpr auto r = groov::resolve(p, p);
    STATIC_CHECK(std::is_same_v<decltype(r), groov::path<> const>);
}

TEST_CASE("path resolves a shorter path", "[path]") {
    using namespace groov::literals;
    constexpr auto p = "a.b.c"_r;
    constexpr auto r1 = groov::resolve(p, "a"_r);
    STATIC_CHECK(r1 == "b.c"_r);
    constexpr auto r2 = groov::resolve(p, "a.b"_r);
    STATIC_CHECK(r2 == "c"_r);
}

TEST_CASE("path doesn't resolve a non-path", "[path]") {
    using namespace groov::literals;
    constexpr auto p = "a.b.c"_r;
    STATIC_CHECK(not groov::can_resolve<decltype(p), int>);
}

TEST_CASE("mismatched path gives invalid resolution", "[path]") {
    using namespace groov::literals;
    constexpr auto p = "a.b.c"_r;
    STATIC_CHECK(std::is_same_v<groov::mismatch_t,
                                decltype(groov::resolve(p, "invalid"_r))>);
}

TEST_CASE("too-long path gives invalid resolution", "[path]") {
    using namespace groov::literals;
    constexpr auto p = "a.b"_r;
    STATIC_CHECK(std::is_same_v<groov::too_long_t,
                                decltype(groov::resolve(p, "a.b.c"_r))>);
}

TEST_CASE("root of a path", "[path]") {
    using namespace groov::literals;
    constexpr auto p = "a.b.c"_r;
    STATIC_CHECK(root(p) == stdx::ct_string{"a"});
}

TEST_CASE("root of a one-layer path", "[path]") {
    using namespace groov::literals;
    constexpr auto p = "a"_r;
    STATIC_CHECK(root(p) == stdx::ct_string{"a"});
}

TEST_CASE("root of an empty path", "[path]") {
    using namespace groov::literals;
    constexpr auto p = ""_r;
    STATIC_CHECK(root(p) == stdx::ct_string{""});
}

TEST_CASE("path without its root", "[path]") {
    using namespace groov::literals;
    constexpr auto p = "a.b.c"_r;
    STATIC_CHECK(without_root(p) == "b.c"_r);
}

TEST_CASE("one-layer path without its root", "[path]") {
    using namespace groov::literals;
    constexpr auto p = "a"_r;
    STATIC_CHECK(without_root(p) == ""_r);
}

TEST_CASE("empty path without its root", "[path]") {
    using namespace groov::literals;
    constexpr auto p = ""_r;
    STATIC_CHECK(without_root(p) == ""_r);
}

TEST_CASE("parent of a path", "[path]") {
    using namespace groov::literals;
    constexpr auto p = "a.b.c"_r;
    STATIC_CHECK(parent(p) == "a.b"_r);
}

TEST_CASE("parent of a one-layer path", "[path]") {
    using namespace groov::literals;
    constexpr auto p = "a"_r;
    STATIC_CHECK(parent(p) == ""_r);
}

TEST_CASE("parent of an empty path", "[path]") {
    using namespace groov::literals;
    constexpr auto p = ""_r;
    STATIC_CHECK(parent(p) == ""_r);
}

TEST_CASE("empty path predicate", "[path]") {
    using P = groov::path<>;
    STATIC_CHECK(std::empty(P{}));
    using Q = groov::path<"hello">;
    STATIC_CHECK(not std::empty(Q{}));
}

TEST_CASE("path is pathlike", "[path]") {
    using namespace groov::literals;
    STATIC_CHECK(groov::pathlike<decltype("reg"_r / "field"_f)>);
    STATIC_CHECK(not groov::valued_pathlike<decltype("reg"_r / "field"_f)>);
}
