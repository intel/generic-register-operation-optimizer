#include <groov/object.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("fields inside a register", "[object]") {
    using F = groov::field<"field", std::uint32_t, 0, 0>;
    using R = groov::reg<"reg", std::uint32_t, 0, F>;
    using X = groov::get_child<"field", R>;
    static_assert(std::same_as<F, X>);
}

TEST_CASE("registers in a group", "[object]") {
    using R = groov::reg<"reg", std::uint32_t, 0>;
    using G = groov::group<"group", void, R>;
    using X = groov::get_child<"reg", G>;
    static_assert(std::same_as<R, X>);
}

TEST_CASE("subfields inside a field", "[object]") {
    using SubF = groov::field<"subfield", std::uint32_t, 0, 0>;
    using F = groov::field<"field", std::uint32_t, 0, 0, SubF>;
    using X = groov::get_child<"subfield", F>;
    static_assert(std::same_as<SubF, X>);
}
