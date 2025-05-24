#include <groov/config.hpp>
#include <groov/path.hpp>
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
} // namespace

TEST_CASE("fields inside a register", "[config]") {
    using F = groov::field<"field", std::uint32_t, 0, 0>;
    using R = groov::reg<"reg", std::uint32_t, 0, groov::w::replace, F>;
    using X = groov::get_child<R, "field">;
    STATIC_REQUIRE(std::same_as<F, X>);
}

TEST_CASE("registers in a group", "[config]") {
    using R = groov::reg<"reg", std::uint32_t, 0>;
    using G = groov::group<"group", bus, R>;
    using X = groov::get_child<G, "reg">;
    STATIC_REQUIRE(std::same_as<R, X>);
}

TEST_CASE("subfields inside a field", "[config]") {
    using SubF = groov::field<"subfield", std::uint32_t, 0, 0>;
    using F =
        groov::field<"field", std::uint32_t, 0, 0, groov::w::replace, SubF>;
    using X = groov::get_child<F, "subfield">;
    STATIC_REQUIRE(std::same_as<SubF, X>);
}

TEST_CASE("field can be extracted from register value", "[config]") {
    constexpr std::uint32_t value{0b11};
    using F = groov::field<"field", std::uint32_t, 0, 0>;
    STATIC_REQUIRE(F::extract(value) == 1);
}

TEST_CASE("field can resolve a path", "[config]") {
    using namespace groov::literals;
    using F = groov::field<"field", std::uint32_t, 0, 0>;
    constexpr auto r = groov::resolve(F{}, "field"_f);
    STATIC_REQUIRE(std::is_same_v<decltype(r), F const>);
}

TEST_CASE("field containing subfields can resolve a path", "[config]") {
    using namespace groov::literals;
    using SubF = groov::field<"subfield", std::uint32_t, 0, 0>;
    using F =
        groov::field<"field", std::uint32_t, 0, 0, groov::w::replace, SubF>;
    constexpr auto r = groov::resolve(F{}, "field.subfield"_f);
    STATIC_REQUIRE(std::is_same_v<decltype(r), SubF const>);
}

TEST_CASE("field can resolve an unambiguous subpath", "[config]") {
    using namespace groov::literals;
    using SubF = groov::field<"subfield", std::uint32_t, 0, 0>;
    using F =
        groov::field<"field", std::uint32_t, 0, 0, groov::w::replace, SubF>;
    constexpr auto r = groov::resolve(F{}, "subfield"_f);
    STATIC_REQUIRE(std::is_same_v<decltype(r), SubF const>);
}

TEST_CASE("register can resolve a path", "[config]") {
    using namespace groov::literals;
    using F = groov::field<"field", std::uint32_t, 0, 0>;
    using R = groov::reg<"reg", std::uint32_t, 0, groov::w::replace, F>;
    constexpr auto r = groov::resolve(R{}, "reg"_r);
    STATIC_REQUIRE(std::is_same_v<decltype(r), R const>);
}

TEST_CASE("register can resolve an unambiguous subpath", "[config]") {
    using namespace groov::literals;
    using F = groov::field<"field", std::uint32_t, 0, 0>;
    using R = groov::reg<"reg", std::uint32_t, 0, groov::w::replace, F>;
    constexpr auto r = groov::resolve(R{}, "field"_f);
    STATIC_REQUIRE(std::is_same_v<decltype(r), F const>);
}

TEST_CASE("register can resolve an unambiguous nested subpath", "[config]") {
    using namespace groov::literals;
    using SubF = groov::field<"subfield", std::uint32_t, 0, 0>;
    using F =
        groov::field<"field", std::uint32_t, 0, 0, groov::w::replace, SubF>;
    using R = groov::reg<"reg", std::uint32_t, 0, groov::w::replace, F>;
    constexpr auto r = groov::resolve(R{}, "subfield"_f);
    STATIC_REQUIRE(std::is_same_v<decltype(r), SubF const>);
}

TEST_CASE("invalid path gives invalid resolution", "[config]") {
    using namespace groov::literals;
    using F = groov::field<"field", std::uint32_t, 0, 0>;
    using R = groov::reg<"reg", std::uint32_t, 0, groov::w::replace, F>;
    STATIC_REQUIRE(std::is_same_v<groov::invalid_t,
                                  decltype(groov::resolve(R{}, "invalid"_f))>);
}

TEST_CASE("ambiguous subpath gives ambiguous resolution", "[config]") {
    using namespace groov::literals;
    using SubF = groov::field<"subfield", std::uint32_t, 0, 0>;
    using F0 =
        groov::field<"field0", std::uint32_t, 0, 0, groov::w::replace, SubF>;
    using F1 =
        groov::field<"field1", std::uint32_t, 1, 1, groov::w::replace, SubF>;
    using R = groov::reg<"reg", std::uint32_t, 0, groov::w::replace, F0, F1>;
    STATIC_REQUIRE(std::is_same_v<groov::ambiguous_t,
                                  decltype(groov::resolve(R{}, "subfield"_f))>);
}

TEST_CASE("group can resolve a path", "[config]") {
    using namespace groov::literals;
    using F = groov::field<"field", std::uint32_t, 0, 0>;
    using R = groov::reg<"reg", std::uint32_t, 0, groov::w::replace, F>;
    using G = groov::group<"group", bus, R>;
    constexpr auto r = groov::resolve(G{}, "reg.field"_f);
    STATIC_REQUIRE(std::is_same_v<decltype(r), F const>);
}

TEST_CASE("all fields inside a register with no fields", "[config]") {
    using namespace groov::literals;
    using R = groov::reg<"reg", std::uint32_t, 0>;
    STATIC_REQUIRE(
        std::is_same_v<groov::detail::all_fields_t<boost::mp11::mp_list<R>>,
                       boost::mp11::mp_list<R>>);
}

TEST_CASE("all fields inside a register with fields", "[config]") {
    using namespace groov::literals;
    using F0 = groov::field<"field0", std::uint32_t, 0, 0>;
    using F1 = groov::field<"field1", std::uint32_t, 1, 1>;
    using R = groov::reg<"reg", std::uint32_t, 0, groov::w::replace, F0, F1>;
    STATIC_REQUIRE(
        std::is_same_v<groov::detail::all_fields_t<boost::mp11::mp_list<R>>,
                       boost::mp11::mp_list<F0, F1>>);
}

TEST_CASE("all fields inside a register with fields and subfields",
          "[config]") {
    using namespace groov::literals;
    using SubF00 = groov::field<"subfield", std::uint32_t, 0, 0>;
    using SubF01 = groov::field<"subfield", std::uint32_t, 1, 1>;
    using F0 = groov::field<"field0", std::uint32_t, 1, 0, groov::w::replace,
                            SubF00, SubF01>;
    using F1 = groov::field<"field1", std::uint32_t, 2, 2>;
    using R = groov::reg<"reg", std::uint32_t, 0, groov::w::replace, F0, F1>;
    STATIC_REQUIRE(
        std::is_same_v<groov::detail::all_fields_t<boost::mp11::mp_list<R>>,
                       boost::mp11::mp_list<SubF00, SubF01, F1>>);
}
