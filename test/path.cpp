#include <groov/path.hpp>

#include <catch2/catch_test_macros.hpp>

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
    static_assert(
        std::is_same_v<decltype(p), groov::path<"reg", "field"> const>);
}

TEST_CASE("path with value", "[identifier]") {
    using namespace groov::literals;
    constexpr auto v = ("reg"_r / "field"_f = 5);
    static_assert(
        std::is_same_v<
            decltype(v),
            groov::with_values_t<groov::path<"reg", "field">, int> const>);
    static_assert(v.value == 5);
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
