#include <async/concepts.hpp>
#include <async/just.hpp>
#include <async/sync_wait.hpp>

#include <groov/identifier.hpp>
#include <groov/object.hpp>
#include <groov/read.hpp>

#include <catch2/catch_test_macros.hpp>

#include <concepts>
#include <cstdint>
#include <type_traits>

namespace {
std::uint32_t data{};

struct bus {
    static async::sender auto read(std::uint32_t *addr) {
        return async::just(*addr);
    }
};

using F0 = groov::field<"field0", std::uint8_t, 0, 0>;
using F1 = groov::field<"field1", std::uint8_t, 4, 1>;
using F2 = groov::field<"field2", std::uint8_t, 7, 5>;
using R = groov::reg<"reg", std::uint32_t, &data, F0, F1, F2>;
using G = groov::group<"group", bus, R>;
} // namespace

TEST_CASE("extract a field", "[read]") {
    auto const x = std::uint32_t{0b111'1010'1u};
    CHECK(groov::extract<G, "reg">(x) == x);
    CHECK(groov::extract<G, "reg", "field0">(x) == 1u);
    CHECK(groov::extract<G, "reg", "field1">(x) == 0b1010u);
    CHECK(groov::extract<G, "reg", "field2">(x) == 0b111u);
}

TEST_CASE("read a field", "[read]") {
    using namespace groov::literals;
    data = 0b10101u;
    auto r = groov::read<G>("reg.field1"_f);
    auto const result = async::sync_wait(r);

    static_assert(std::same_as<typename F1::type_t,
                               std::remove_cvref_t<decltype(get<0>(*result))>>);
    REQUIRE(result.has_value());
    CHECK(get<0>(*result) == 0b1010u);
}
