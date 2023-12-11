#include <groov/path.hpp>

#include <catch2/catch_test_macros.hpp>

#include <type_traits>

TEST_CASE("register literal", "[identifier]") {
    using namespace groov::literals;
    constexpr auto r = "reg"_r;
    static_assert(std::is_same_v<decltype(r), groov::path<"reg"> const>);
}

TEST_CASE("field literal", "[identifier]") {
    using namespace groov::literals;
    constexpr auto f = "field"_f;
    static_assert(std::is_same_v<decltype(f), groov::path<"field"> const>);
}

TEST_CASE("dot-separated literal", "[identifier]") {
    using namespace groov::literals;
    constexpr auto f = "reg.field"_f;
    static_assert(
        std::is_same_v<decltype(f), groov::path<"reg", "field"> const>);
}

TEST_CASE("path concatenation with /", "[identifier]") {
    using namespace groov::literals;
    constexpr auto p = "reg"_r / "field"_f;
    static_assert(p == "reg.field"_f);
}

TEST_CASE("path can resolve itself", "[resolve]") {
    using namespace groov::literals;
    constexpr auto p = "a"_r;
    static_assert(groov::is_resolvable_v<decltype(p), decltype(p)>);
}

TEST_CASE("path resolves itself to empty path", "[resolve]") {
    using namespace groov::literals;
    constexpr auto p = "a"_r;
    constexpr auto r = groov::resolve(p, p);
    static_assert(std::is_same_v<decltype(r), groov::path<> const>);
}

TEST_CASE("path resolves a shorter path", "[resolve]") {
    using namespace groov::literals;
    constexpr auto p = "a.b.c"_r;
    constexpr auto r1 = groov::resolve(p, "a"_r);
    static_assert(r1 == "b.c"_r);
    constexpr auto r2 = groov::resolve(p, "a.b"_r);
    static_assert(r2 == "c"_r);
}

TEST_CASE("path doesn't resolve a non-path", "[resolve]") {
    using namespace groov::literals;
    constexpr auto p = "a.b.c"_r;
    static_assert(not groov::can_resolve<decltype(p), int>);
}

TEST_CASE("path doesn't resolve a different path", "[resolve]") {
    using namespace groov::literals;
    constexpr auto p = "a.b.c"_r;
    constexpr auto x = "c"_r;
    static_assert(not groov::can_resolve<decltype(p), decltype(x)>);
}

TEST_CASE("path provides its own root", "[resolve]") {
    using namespace groov::literals;
    constexpr auto p = "a.b.c"_r;
    static_assert(p.root() == stdx::ct_string{"a"});
}

TEST_CASE("path without its root", "[resolve]") {
    using namespace groov::literals;
    constexpr auto p = "a.b.c"_r;
    static_assert(p.without_root() == "b.c"_r);
}

TEST_CASE("path with value (operator=)", "[identifier]") {
    using namespace groov::literals;
    constexpr auto v = ("reg"_r / "field"_f = 5);
    static_assert(std::is_same_v<groov::get_path_t<decltype(v)>,
                                 decltype("reg.field"_f)>);
    static_assert(v.value == 5);
}

TEST_CASE("path with value (operator())", "[identifier]") {
    using namespace groov::literals;
    constexpr auto v = "reg"_r(5);
    static_assert(
        std::is_same_v<groov::get_path_t<decltype(v)>, decltype("reg"_f)>);
    static_assert(v.value == 5);
}

TEST_CASE("path with value can resolve path", "[identifier]") {
    using namespace groov::literals;
    constexpr auto v = ("reg"_r / "field"_f = 5);
    constexpr auto r = groov::resolve(v, "reg"_r);
    static_assert(
        std::is_same_v<groov::get_path_t<decltype(r)>, decltype("field"_f)>);
    static_assert(r.value == 5);
}

TEST_CASE("mismatched path gives invalid resolution", "[identifier]") {
    using namespace groov::literals;
    constexpr auto v = ("reg"_r / "field"_f = 5);
    static_assert(std::is_same_v<groov::mismatch_t,
                                 decltype(groov::resolve(v, "invalid"_r))>);
}

TEST_CASE("too-long path gives invalid resolution", "[identifier]") {
    using namespace groov::literals;
    constexpr auto v = ("reg"_r / "field"_f = 5);
    static_assert(
        std::is_same_v<groov::too_long_t,
                       decltype(groov::resolve(v, "reg.field.foo"_r))>);
}

TEST_CASE("register with multiple field values", "[identifier]") {
    using namespace groov::literals;
    constexpr auto v = "reg"_r("field1"_f = 5, "field2"_f = 10.0);
    static_assert(
        std::is_same_v<
            decltype(v),
            groov::with_values_t<
                groov::path<"reg">,
                groov::with_values_t<groov::path<"field1">, int>,
                groov::with_values_t<groov::path<"field2">, double>> const>);
}

TEST_CASE("retrieve values by path", "[identifier]") {
    using namespace groov::literals;
    constexpr auto v = ("field"_f = 5);
    static_assert(v["field"_f] == 5);
}

TEST_CASE("drill down into value with partial path", "[identifier]") {
    using namespace groov::literals;
    constexpr auto v = ("reg"_r / "field"_f = 5);
    constexpr auto r = v["reg"_r];
    static_assert(
        std::is_same_v<groov::get_path_t<decltype(r)>, decltype("field"_f)>);
    static_assert(r.value == 5);
}

TEST_CASE("retrieve located value by path", "[identifier]") {
    using namespace groov::literals;
    constexpr auto v = ("reg"_r / "field"_f = 5);
    constexpr auto f = v["reg"_r];
    static_assert(
        std::is_same_v<decltype(f),
                       groov::with_values_t<groov::path<"field">, int> const>);
    static_assert(f["field"_f] == 5);
}

TEST_CASE("retrieve among multiple located values", "[identifier]") {
    using namespace groov::literals;
    constexpr auto v = "reg"_r("field1"_f = 5, "field2"_f = 10.0);
    static_assert(v["reg"_r]["field1"_f] == 5);
}

TEST_CASE("tree of values", "[identifier]") {
    using namespace groov::literals;
    constexpr auto v = "reg"_r(
        "field1"_f = 5, "field2"_f("subfield1"_f = 10.0, "subfield2"_f = true));
    static_assert(v["reg"_r]["field1"_f] == 5);
    static_assert(v["reg"_r]["field2"_f]["subfield1"_f] == 10.0);
}

TEST_CASE("retrieve by path", "[identifier]") {
    using namespace groov::literals;
    constexpr auto v = "reg"_r(
        "field1"_f = 5, "field2"_f("subfield1"_f = 10.0, "subfield2"_f = true));
    static_assert(v["reg.field1"_f] == 5);
    static_assert(v["reg.field2"_f]["subfield1"_f] == 10.0);
}
