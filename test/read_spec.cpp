#include <groov/config.hpp>
#include <groov/path.hpp>
#include <groov/read_spec.hpp>
#include <groov/resolve.hpp>

#include <async/concepts.hpp>

#include <catch2/catch_test_macros.hpp>

namespace {
struct bus {
    struct sender {
        using is_sender = void;
    };

    template <auto> static auto read(auto...) -> async::sender auto {
        return sender{};
    }
    template <auto...> static auto write(auto...) -> async::sender auto {
        return sender{};
    }
};

using F0 = groov::field<"field0", std::uint8_t, 0, 0>;
using F1 = groov::field<"field1", std::uint8_t, 4, 1>;
using F2 = groov::field<"field2", std::uint8_t, 7, 5>;

std::uint32_t data0{};
using R0 =
    groov::reg<"reg0", std::uint32_t, &data0, groov::w::replace, F0, F1, F2>;
std::uint32_t data1{};
using R1 =
    groov::reg<"reg1", std::uint32_t, &data1, groov::w::replace, F0, F1, F2>;

using G = groov::group<"group", bus, R0, R1>;
constexpr auto grp = G{};
} // namespace

TEST_CASE("read_spec is made by combining group and paths", "[read_spec]") {
    using namespace groov::literals;
    auto spec = grp("reg0"_r);
    STATIC_REQUIRE(
        stdx::is_specialization_of_v<decltype(spec), groov::read_spec>);
}

TEST_CASE("valid paths are captured", "[read_spec]") {
    using namespace groov::literals;
    auto p = "reg0"_r;
    auto spec = grp(p);
    STATIC_REQUIRE(std::is_same_v<decltype(spec)::paths_t,
                                  boost::mp11::mp_list<decltype(p)>>);
}

TEST_CASE("multiple paths can be passed", "[read_spec]") {
    using namespace groov::literals;
    auto p = "reg0"_r;
    auto q = "reg1"_r;
    auto spec = grp(p, q);
    STATIC_REQUIRE(
        std::is_same_v<decltype(spec)::paths_t,
                       boost::mp11::mp_list<decltype(p), decltype(q)>>);
}

TEST_CASE("operator/ is overloaded to make read_spec", "[read_spec]") {
    using namespace groov::literals;
    auto p = "reg0"_r;
    auto spec = grp / p;
    STATIC_REQUIRE(std::is_same_v<decltype(spec)::paths_t,
                                  boost::mp11::mp_list<decltype(p)>>);
}
