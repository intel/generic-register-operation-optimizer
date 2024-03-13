#include <groov/identity.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <limits>
#include <type_traits>

TEST_CASE("all specs except none fulfil identity_spec", "[identity]") {
    static_assert(not groov::identity_spec<groov::id::none>);
    static_assert(groov::identity_spec<groov::id::zero>);
    static_assert(groov::identity_spec<groov::id::one>);
    static_assert(groov::identity_spec<groov::id::any>);
}

TEST_CASE("zero id spec returns a zeroed identity", "[identity]") {
    constexpr auto i = groov::id::zero::identity<std::uint8_t, 7, 0>();
    static_assert(i == 0);
}

TEST_CASE("one id spec returns a identity with ones", "[identity]") {
    constexpr auto i = groov::id::one::identity<std::uint8_t, 7, 0>();
    static_assert(i == 0xffu);
}

TEST_CASE("any id spec returns a zeroed identity", "[identity]") {
    constexpr auto i = groov::id::any::identity<std::uint8_t, 7, 0>();
    static_assert(i == 0);
}

TEST_CASE("replace write function has no identity", "[identity]") {
    static_assert(groov::write_function<groov::w::replace>);
    static_assert(std::is_same_v<groov::w::replace::id_spec, groov::id::none>);
}

TEST_CASE("read_only write function has any identity", "[identity]") {
    static_assert(groov::write_function<groov::w::read_only>);
    static_assert(std::is_same_v<groov::w::read_only::id_spec, groov::id::any>);
}

TEST_CASE("write one to set/clear write function has zero identity",
          "[identity]") {
    static_assert(groov::write_function<groov::w::one_to_set>);
    static_assert(groov::write_function<groov::w::one_to_clear>);
    static_assert(
        std::is_same_v<groov::w::one_to_set::id_spec, groov::id::zero>);
    static_assert(
        std::is_same_v<groov::w::one_to_clear::id_spec, groov::id::zero>);
}

TEST_CASE("write zero to set/clear write function has one identity",
          "[identity]") {
    static_assert(groov::write_function<groov::w::zero_to_set>);
    static_assert(groov::write_function<groov::w::zero_to_clear>);
    static_assert(
        std::is_same_v<groov::w::zero_to_set::id_spec, groov::id::one>);
    static_assert(
        std::is_same_v<groov::w::zero_to_clear::id_spec, groov::id::one>);
}
