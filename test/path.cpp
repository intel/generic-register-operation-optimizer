#include <groov/path.hpp>

#include <catch2/catch_test_macros.hpp>

#include <type_traits>

TEST_CASE("register literal", "[path]") {
    using namespace groov::literals;
    constexpr auto r = "reg"_r;
    static_assert(std::is_same_v<decltype(r), groov::path<"reg"> const>);
}

TEST_CASE("field literal", "[path]") {
    using namespace groov::literals;
    constexpr auto f = "field"_f;
    static_assert(std::is_same_v<decltype(f), groov::path<"field"> const>);
}

TEST_CASE("dot-separated literal", "[path]") {
    using namespace groov::literals;
    constexpr auto f = "a.b.c.d"_f;
    static_assert(
        std::is_same_v<decltype(f), groov::path<"a", "b", "c", "d"> const>);
}

TEST_CASE("path concatenation with /", "[path]") {
    using namespace groov::literals;
    constexpr auto p = "reg"_r / "field"_f;
    static_assert(p == "reg.field"_f);
}

TEST_CASE("path can resolve itself", "[path]") {
    using namespace groov::literals;
    constexpr auto p = "a"_r;
    static_assert(groov::is_resolvable_v<decltype(p), decltype(p)>);
}

TEST_CASE("path resolves itself to empty path", "[path]") {
    using namespace groov::literals;
    constexpr auto p = "a"_r;
    constexpr auto r = groov::resolve(p, p);
    static_assert(std::is_same_v<decltype(r), groov::path<> const>);
}

TEST_CASE("path resolves a shorter path", "[path]") {
    using namespace groov::literals;
    constexpr auto p = "a.b.c"_r;
    constexpr auto r1 = groov::resolve(p, "a"_r);
    static_assert(r1 == "b.c"_r);
    constexpr auto r2 = groov::resolve(p, "a.b"_r);
    static_assert(r2 == "c"_r);
}

TEST_CASE("path doesn't resolve a non-path", "[path]") {
    using namespace groov::literals;
    constexpr auto p = "a.b.c"_r;
    static_assert(not groov::can_resolve<decltype(p), int>);
}

TEST_CASE("path doesn't resolve a different path", "[path]") {
    using namespace groov::literals;
    constexpr auto p = "a.b.c"_r;
    constexpr auto x = "c"_r;
    static_assert(not groov::can_resolve<decltype(p), decltype(x)>);
}

TEST_CASE("root of a path", "[path]") {
    using namespace groov::literals;
    constexpr auto p = "a.b.c"_r;
    static_assert(root(p) == stdx::ct_string{"a"});
}

TEST_CASE("path without its root", "[path]") {
    using namespace groov::literals;
    constexpr auto p = "a.b.c"_r;
    static_assert(without_root(p) == "b.c"_r);
}

TEST_CASE("path with value (operator=)", "[path]") {
    using namespace groov::literals;
    constexpr auto v = "reg"_r / "field"_f = 5;
    static_assert(std::is_same_v<groov::get_path_t<decltype(v)>,
                                 decltype("reg.field"_f)>);
    static_assert(v.value == 5);
}

TEST_CASE("path with value (operator())", "[path]") {
    using namespace groov::literals;
    constexpr auto v = "reg"_r(5);
    static_assert(
        std::is_same_v<groov::get_path_t<decltype(v)>, decltype("reg"_f)>);
    static_assert(v.value == 5);
}

TEST_CASE("path with value can resolve path", "[path]") {
    using namespace groov::literals;
    constexpr auto v = "reg"_r / "field"_f = 5;
    constexpr auto r = groov::resolve(v, "reg"_r);
    static_assert(
        std::is_same_v<groov::get_path_t<decltype(r)>, decltype("field"_f)>);
    static_assert(r.value == 5);
}

TEST_CASE("mismatched path gives invalid resolution", "[path]") {
    using namespace groov::literals;
    constexpr auto v = "reg"_r / "field"_f = 5;
    static_assert(std::is_same_v<groov::mismatch_t,
                                 decltype(groov::resolve(v, "invalid"_r))>);
}

TEST_CASE("too-long path gives invalid resolution", "[path]") {
    using namespace groov::literals;
    constexpr auto v = "reg"_r / "field"_f = 5;
    static_assert(
        std::is_same_v<groov::too_long_t,
                       decltype(groov::resolve(v, "reg.field.foo"_r))>);
}

TEST_CASE("ambiguous path gives invalid resolution", "[path]") {
    using namespace groov::literals;
    constexpr auto v = "reg"_r("field"_f = 5, "field"_f = 6);
    static_assert(std::is_same_v<groov::ambiguous_t,
                                 decltype(groov::resolve(v, "reg.field"_r))>);
}

TEST_CASE("register with multiple field values", "[path]") {
    using namespace groov::literals;
    constexpr auto v = "reg"_r("field1"_f = 5, "field2"_f = 10.0);
    static_assert(
        std::is_same_v<
            decltype(v),
            groov::value_path<
                groov::path<"reg">,
                stdx::tuple<groov::value_path<groov::path<"field1">, int>,
                            groov::value_path<groov::path<"field2">,
                                                double>>> const>);
}

TEST_CASE("retrieve values by path", "[path]") {
    using namespace groov::literals;
    constexpr auto v = ("field"_f = 5);
    static_assert(v["field"_f] == 5);
}

TEST_CASE("drill down into value with partial path", "[path]") {
    using namespace groov::literals;
    constexpr auto v = "reg"_r / "field"_f = 5;
    constexpr auto r = v["reg"_r];
    static_assert(
        std::is_same_v<groov::get_path_t<decltype(r)>, decltype("field"_f)>);
    static_assert(r.value == 5);
}

TEST_CASE("retrieve located value by path", "[path]") {
    using namespace groov::literals;
    constexpr auto v = "reg"_r / "field"_f = 5;
    constexpr auto f = v["reg"_r];
    static_assert(
        std::is_same_v<decltype(f),
                       groov::value_path<groov::path<"field">, int> const>);
    static_assert(f["field"_f] == 5);
}

TEST_CASE("retrieve among multiple located values", "[path]") {
    using namespace groov::literals;
    constexpr auto v = "reg"_r("field1"_f = 5, "field2"_f = 10.0);
    static_assert(v["reg"_r]["field1"_f] == 5);
    static_assert(v["reg.field2"_f] == 10.0);
}

TEST_CASE("tree of values", "[path]") {
    using namespace groov::literals;
    constexpr auto v = "reg"_r(
        "field1"_f = 5, "field2"_f("subfield1"_f = 10.0, "subfield2"_f = true));
    static_assert(v["reg"_r]["field1"_f] == 5);
    static_assert(v["reg"_r]["field2"_f]["subfield1"_f] == 10.0);
}

TEST_CASE("retrieve by path", "[path]") {
    using namespace groov::literals;
    constexpr auto v = "reg"_r(
        "field1"_f = 5, "field2"_f("subfield1"_f = 10.0, "subfield2"_f = true));
    static_assert(v["reg.field1"_f] == 5);
    static_assert(v["reg.field2"_f]["subfield1"_f] == 10.0);
}

TEST_CASE("empty path predicate", "[path]") {
    using P = groov::path<>;
    static_assert(std::empty(P{}));
    using Q = groov::path<"hello">;
    static_assert(not std::empty(Q{}));
}

TEST_CASE("path with value can be extended", "[path]") {
    using namespace groov::literals;
    constexpr auto v = "reg"_r / ("field"_f = 5);
    static_assert(std::is_same_v<groov::get_path_t<decltype(v)>,
                                 decltype("reg.field"_f)>);
    static_assert(v.value == 5);
    static_assert(v["reg"_r]["field"_f] == 5);
}

TEST_CASE("root of a path with value", "[path]") {
    using namespace groov::literals;
    constexpr auto v = "reg"_r / "field"_f = 5;
    static_assert(root(v) == stdx::ct_string{"reg"});
}

TEST_CASE("path with value without root", "[path]") {
    using namespace groov::literals;
    constexpr auto v = "reg"_r / "field"_f = 5;
    constexpr auto r = without_root(v);
    static_assert(r["field"_f] == 5);
}

TEST_CASE("register is pathlike", "[path]") {
    using namespace groov::literals;
    static_assert(groov::pathlike<decltype("reg"_r)>);
}

TEST_CASE("field is pathlike", "[path]") {
    using namespace groov::literals;
    static_assert(groov::pathlike<decltype("field"_f)>);
}

