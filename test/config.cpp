#include <async/concepts.hpp>

#include <groov/config.hpp>
#include <groov/path.hpp>
#include <groov/resolve.hpp>

#include <catch2/catch_test_macros.hpp>

namespace {
struct bus {
    struct sender {
        using is_sender = void;
    };

    template <auto> static auto read(auto...) -> async::sender auto {
        return sender{};
    }
    template <auto> static auto write(auto...) -> async::sender auto {
        return sender{};
    }
};
} // namespace

TEST_CASE("fields inside a register", "[config]") {
    using F = groov::field<"field", std::uint32_t, 0, 0>;
    using R = groov::reg<"reg", std::uint32_t, 0, F>;
    using X = groov::get_child<R, "field">;
    static_assert(std::same_as<F, X>);
}

TEST_CASE("registers in a group", "[config]") {
    using R = groov::reg<"reg", std::uint32_t, 0>;
    using G = groov::group<"group", bus, R>;
    using X = groov::get_child<G, "reg">;
    static_assert(std::same_as<R, X>);
}

TEST_CASE("subfields inside a field", "[config]") {
    using SubF = groov::field<"subfield", std::uint32_t, 0, 0>;
    using F = groov::field<"field", std::uint32_t, 0, 0, SubF>;
    using X = groov::get_child<F, "subfield">;
    static_assert(std::same_as<SubF, X>);
}

TEST_CASE("field can be extracted from register value", "[config]") {
    constexpr std::uint32_t value{0b11};
    using F = groov::field<"field", std::uint32_t, 0, 0>;
    static_assert(F::extract(value) == 1);
}

TEST_CASE("field can resolve a path", "[config]") {
    using namespace groov::literals;
    using F = groov::field<"field", std::uint32_t, 0, 0>;
    constexpr auto r = groov::resolve(F{}, "field"_f);
    static_assert(std::is_same_v<decltype(r), F const>);
}

TEST_CASE("field containing subfields can resolve a path", "[config]") {
    using namespace groov::literals;
    using SubF = groov::field<"subfield", std::uint32_t, 0, 0>;
    using F = groov::field<"field", std::uint32_t, 0, 0, SubF>;
    constexpr auto r = groov::resolve(F{}, "field.subfield"_f);
    static_assert(std::is_same_v<decltype(r), SubF const>);
}

TEST_CASE("field can resolve an unambiguous subpath", "[config]") {
    using namespace groov::literals;
    using SubF = groov::field<"subfield", std::uint32_t, 0, 0>;
    using F = groov::field<"field", std::uint32_t, 0, 0, SubF>;
    constexpr auto r = groov::resolve(F{}, "subfield"_f);
    static_assert(std::is_same_v<decltype(r), SubF const>);
}

TEST_CASE("register can resolve a path", "[config]") {
    using namespace groov::literals;
    using F = groov::field<"field", std::uint32_t, 0, 0>;
    using R = groov::reg<"reg", std::uint32_t, 0, F>;
    constexpr auto r = groov::resolve(R{}, "reg"_r);
    static_assert(std::is_same_v<decltype(r), R const>);
}

TEST_CASE("register can resolve an unambiguous subpath", "[config]") {
    using namespace groov::literals;
    using F = groov::field<"field", std::uint32_t, 0, 0>;
    using R = groov::reg<"reg", std::uint32_t, 0, F>;
    constexpr auto r = groov::resolve(R{}, "field"_f);
    static_assert(std::is_same_v<decltype(r), F const>);
}

TEST_CASE("register can resolve an unambiguous nested subpath", "[config]") {
    using namespace groov::literals;
    using SubF = groov::field<"subfield", std::uint32_t, 0, 0>;
    using F = groov::field<"field", std::uint32_t, 0, 0, SubF>;
    using R = groov::reg<"reg", std::uint32_t, 0, F>;
    constexpr auto r = groov::resolve(R{}, "subfield"_f);
    static_assert(std::is_same_v<decltype(r), SubF const>);
}

TEST_CASE("invalid path gives invalid resolution", "[config]") {
    using namespace groov::literals;
    using F = groov::field<"field", std::uint32_t, 0, 0>;
    using R = groov::reg<"reg", std::uint32_t, 0, F>;
    static_assert(std::is_same_v<groov::invalid_t,
                                 decltype(groov::resolve(R{}, "invalid"_f))>);
}

TEST_CASE("ambiguous subpath gives ambiguous resolution", "[config]") {
    using namespace groov::literals;
    using SubF = groov::field<"subfield", std::uint32_t, 0, 0>;
    using F0 = groov::field<"field0", std::uint32_t, 0, 0, SubF>;
    using F1 = groov::field<"field1", std::uint32_t, 0, 0, SubF>;
    using R = groov::reg<"reg", std::uint32_t, 0, F0, F1>;
    static_assert(std::is_same_v<groov::ambiguous_t,
                                 decltype(groov::resolve(R{}, "subfield"_f))>);
}

TEST_CASE("group can resolve a path", "[config]") {
    using namespace groov::literals;
    using F = groov::field<"field", std::uint32_t, 0, 0>;
    using R = groov::reg<"reg", std::uint32_t, 0, F>;
    using G = groov::group<"group", bus, R>;
    constexpr auto r = groov::resolve(G{}, "reg.field"_f);
    static_assert(std::is_same_v<decltype(r), F const>);
}
