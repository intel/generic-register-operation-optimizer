#include <groov/path.hpp>
#include <groov/value_path.hpp>

#include <catch2/catch_test_macros.hpp>

#include <type_traits>

TEST_CASE("path with value (operator=)", "[value_path]") {
    using namespace groov::literals;
    constexpr auto v = "reg"_r / "field"_f = 5;
    static_assert(std::is_same_v<groov::get_path_t<decltype(v)>,
                                 decltype("reg.field"_f)>);
    static_assert(v.value == 5);
}

TEST_CASE("path with value (operator())", "[value_path]") {
    using namespace groov::literals;
    constexpr auto v = "reg"_r(5);
    static_assert(
        std::is_same_v<groov::get_path_t<decltype(v)>, decltype("reg"_f)>);
    static_assert(v.value == 5);
}

TEST_CASE("path with value can resolve path", "[value_path]") {
    using namespace groov::literals;
    constexpr auto v = "reg"_r / "field"_f = 5;
    constexpr auto r = groov::resolve(v, "reg"_r);
    static_assert(
        std::is_same_v<groov::get_path_t<decltype(r)>, decltype("field"_f)>);
    static_assert(r.value == 5);
}

TEST_CASE("mismatched path gives invalid resolution", "[value_path]") {
    using namespace groov::literals;
    constexpr auto v = "reg"_r / "field"_f = 5;
    static_assert(std::is_same_v<groov::mismatch_t,
                                 decltype(groov::resolve(v, "invalid"_r))>);
}

TEST_CASE("too-long path gives invalid resolution", "[value_path]") {
    using namespace groov::literals;
    constexpr auto v = "reg"_r / "field"_f = 5;
    static_assert(
        std::is_same_v<groov::too_long_t,
                       decltype(groov::resolve(v, "reg.field.foo"_r))>);
}

TEST_CASE("ambiguous path gives invalid resolution", "[value_path]") {
    using namespace groov::literals;
    constexpr auto v = "reg"_r("field"_f = 5, "field"_f = 6);
    static_assert(std::is_same_v<groov::ambiguous_t,
                                 decltype(groov::resolve(v, "reg.field"_r))>);
}

TEST_CASE("register with multiple field values", "[value_path]") {
    using namespace groov::literals;
    constexpr auto v = "reg"_r("field1"_f = 5, "field2"_f = 10.0);
    static_assert(
        std::is_same_v<
            decltype(v),
            groov::value_path<
                groov::path<"reg">,
                stdx::tuple<
                    groov::value_path<groov::path<"field1">, int>,
                    groov::value_path<groov::path<"field2">, double>>> const>);
}

TEST_CASE("retrieve values by path", "[value_path]") {
    using namespace groov::literals;
    constexpr auto v = ("field"_f = 5);
    static_assert(v["field"_f] == 5);
}

TEST_CASE("drill down into value with partial path", "[value_path]") {
    using namespace groov::literals;
    constexpr auto v = "reg"_r / "field"_f = 5;
    constexpr auto r = v["reg"_r];
    static_assert(
        std::is_same_v<groov::get_path_t<decltype(r)>, decltype("field"_f)>);
    static_assert(r.value == 5);
}

TEST_CASE("retrieve located value by path", "[value_path]") {
    using namespace groov::literals;
    constexpr auto v = "reg"_r / "field"_f = 5;
    constexpr auto f = v["reg"_r];
    static_assert(
        std::is_same_v<decltype(f),
                       groov::value_path<groov::path<"field">, int> const>);
    static_assert(f["field"_f] == 5);
}

TEST_CASE("retrieve among multiple located values", "[value_path]") {
    using namespace groov::literals;
    constexpr auto v = "reg"_r("field1"_f = 5, "field2"_f = 10.0);
    static_assert(v["reg"_r]["field1"_f] == 5);
    static_assert(v["reg.field2"_f] == 10.0);
}

TEST_CASE("tree of values", "[value_path]") {
    using namespace groov::literals;
    constexpr auto v = "reg"_r(
        "field1"_f = 5, "field2"_f("subfield1"_f = 10.0, "subfield2"_f = true));
    static_assert(v["reg"_r]["field1"_f] == 5);
    static_assert(v["reg"_r]["field2"_f]["subfield1"_f] == 10.0);
}

TEST_CASE("retrieve by path", "[value_path]") {
    using namespace groov::literals;
    constexpr auto v = "reg"_r(
        "field1"_f = 5, "field2"_f("subfield1"_f = 10.0, "subfield2"_f = true));
    static_assert(v["reg.field1"_f] == 5);
    static_assert(v["reg.field2"_f]["subfield1"_f] == 10.0);
}

TEST_CASE("path with value can be extended", "[value_path]") {
    using namespace groov::literals;
    constexpr auto v = "reg"_r / ("field"_f = 5);
    static_assert(std::is_same_v<groov::get_path_t<decltype(v)>,
                                 decltype("reg.field"_f)>);
    static_assert(v.value == 5);
    static_assert(v["reg"_r]["field"_f] == 5);
}

TEST_CASE("path (with value) equals (path with) value ", "[value_path]") {
    using namespace groov::literals;
    constexpr auto v1 = "reg"_r / ("field"_f = 5);
    constexpr auto v2 = ("reg"_r / "field"_f) = 5;
    static_assert(v1 == v2);
}

TEST_CASE("root of a path with value", "[value_path]") {
    using namespace groov::literals;
    constexpr auto v = "reg"_r / "field"_f = 5;
    static_assert(root(v) == stdx::ct_string{"reg"});
}

TEST_CASE("path with value without root", "[value_path]") {
    using namespace groov::literals;
    constexpr auto v = "reg"_r / "field"_f = 5;
    constexpr auto r = without_root(v);
    static_assert(r["field"_f] == 5);
}

TEST_CASE("path with value is valued_pathlike", "[value_path]") {
    using namespace groov::literals;
    static_assert(groov::pathlike<decltype("reg"_r / "field"_f = 5)>);
    static_assert(groov::valued_pathlike<decltype("reg"_r / "field"_f = 5)>);
}
