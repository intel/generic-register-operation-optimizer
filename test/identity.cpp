#include <groov/identity.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <limits>
#include <type_traits>

TEST_CASE("all specs fulfil mask_spec", "[identity]") {
    STATIC_REQUIRE(groov::mask_spec<groov::m::zero>);
    STATIC_REQUIRE(groov::mask_spec<groov::m::one>);
    STATIC_REQUIRE(groov::mask_spec<groov::m::any>);
}

TEST_CASE("zero id spec returns a zeroed identity", "[identity]") {
    constexpr auto i = groov::m::zero::mask<std::uint8_t{0xffu}>();
    STATIC_REQUIRE(i == 0);
}

TEST_CASE("one id spec returns a identity with ones", "[identity]") {
    constexpr auto i = groov::m::one::mask<std::uint8_t{0xffu}>();
    STATIC_REQUIRE(i == 0xffu);
}

TEST_CASE("any id spec returns a zeroed identity", "[identity]") {
    constexpr auto i = groov::m::any::mask<std::uint8_t{0xffu}>();
    STATIC_REQUIRE(i == 0);
}

TEST_CASE("replace write function has no identity", "[identity]") {
    STATIC_REQUIRE(groov::write_function<groov::w::replace>);
    STATIC_REQUIRE(not groov::identity_write_function<groov::w::replace>);
}

TEST_CASE("ignore write function has any identity", "[identity]") {
    STATIC_REQUIRE(groov::write_function<groov::w::ignore>);
    STATIC_REQUIRE(std::is_same_v<groov::w::ignore::id_spec, groov::m::any>);
}

TEST_CASE("write one to set/clear write function has zero identity",
          "[identity]") {
    STATIC_REQUIRE(groov::write_function<groov::w::one_to_set>);
    STATIC_REQUIRE(groov::write_function<groov::w::one_to_clear>);
    STATIC_REQUIRE(
        std::is_same_v<groov::w::one_to_set::id_spec, groov::m::zero>);
    STATIC_REQUIRE(
        std::is_same_v<groov::w::one_to_clear::id_spec, groov::m::zero>);
}

TEST_CASE("write zero to set/clear write function has one identity",
          "[identity]") {
    STATIC_REQUIRE(groov::write_function<groov::w::zero_to_set>);
    STATIC_REQUIRE(groov::write_function<groov::w::zero_to_clear>);
    STATIC_REQUIRE(
        std::is_same_v<groov::w::zero_to_set::id_spec, groov::m::one>);
    STATIC_REQUIRE(
        std::is_same_v<groov::w::zero_to_clear::id_spec, groov::m::one>);
}
