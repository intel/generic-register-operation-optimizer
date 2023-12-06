#include <async/concepts.hpp>
#include <async/just.hpp>
#include <async/sync_wait.hpp>

#include <groov/object.hpp>
#include <groov/path.hpp>
#include <groov/read.hpp>

#include <cstdint>

// EXPECT: Invalid path passed to read_result lookup

namespace {
struct bus {
    template <auto> static auto read(auto) -> async::sender auto {
        return async::just(42);
    }
};

using F0 = groov::field<"field0", std::uint8_t, 0, 0>;
using F1 = groov::field<"field1", std::uint8_t, 1, 1>;

std::uint32_t data0{};
using R0 = groov::reg<"reg0", std::uint32_t, &data0, F0, F1>;

using G = groov::group<"group", bus, R0>;
} // namespace

auto main() -> int {
    using namespace groov::literals;
    constexpr auto grp = G{};
    auto r = groov::read(grp / "reg0.field0"_f);
    auto const result = get<0>(*async::sync_wait(r));
    return result["reg0.field1"_f];
}
