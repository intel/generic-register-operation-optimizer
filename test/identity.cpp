#include <groov/identity.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <limits>
#include <type_traits>

TEST_CASE("all specs except none fulfil identity_spec", "[identity]") {
    STATIC_REQUIRE(not groov::identity_spec<groov::id::none>);
    STATIC_REQUIRE(groov::identity_spec<groov::id::zero>);
    STATIC_REQUIRE(groov::identity_spec<groov::id::one>);
    STATIC_REQUIRE(groov::identity_spec<groov::id::any>);
}

TEST_CASE("zero id spec returns a zeroed identity", "[identity]") {
    constexpr auto i = groov::id::zero::identity<std::uint8_t, 7, 0>();
    STATIC_REQUIRE(i == 0);
}

TEST_CASE("one id spec returns a identity with ones", "[identity]") {
    constexpr auto i = groov::id::one::identity<std::uint8_t, 7, 0>();
    STATIC_REQUIRE(i == 0xffu);
}

TEST_CASE("any id spec returns a zeroed identity", "[identity]") {
    constexpr auto i = groov::id::any::identity<std::uint8_t, 7, 0>();
    STATIC_REQUIRE(i == 0);
}

TEST_CASE("replace write function has no identity", "[identity]") {
    STATIC_REQUIRE(groov::write_function<groov::w::replace>);
    STATIC_REQUIRE(std::is_same_v<groov::w::replace::id_spec, groov::id::none>);
}

TEST_CASE("read_only write function has any identity", "[identity]") {
    STATIC_REQUIRE(groov::write_function<groov::w::read_only>);
    STATIC_REQUIRE(
        std::is_same_v<groov::w::read_only::id_spec, groov::id::any>);
}

TEST_CASE("write one to set/clear write function has zero identity",
          "[identity]") {
    STATIC_REQUIRE(groov::write_function<groov::w::one_to_set>);
    STATIC_REQUIRE(groov::write_function<groov::w::one_to_clear>);
    STATIC_REQUIRE(
        std::is_same_v<groov::w::one_to_set::id_spec, groov::id::zero>);
    STATIC_REQUIRE(
        std::is_same_v<groov::w::one_to_clear::id_spec, groov::id::zero>);
}

TEST_CASE("write zero to set/clear write function has one identity",
          "[identity]") {
    STATIC_REQUIRE(groov::write_function<groov::w::zero_to_set>);
    STATIC_REQUIRE(groov::write_function<groov::w::zero_to_clear>);
    STATIC_REQUIRE(
        std::is_same_v<groov::w::zero_to_set::id_spec, groov::id::one>);
    STATIC_REQUIRE(
        std::is_same_v<groov::w::zero_to_clear::id_spec, groov::id::one>);
}
