#include <async/concepts.hpp>
#include <async/just_result_of.hpp>
#include <async/sync_wait.hpp>

#include <groov/identifier.hpp>
#include <groov/object.hpp>
#include <groov/read.hpp>

#include <cstdint>

// EXPECT: Invalid path in read result lookup

namespace {
struct bus {
    static async::sender auto read(std::uint32_t *addr) {
        return async::just_result_of([=] { return *addr; });
    }
};

using F0 = groov::field<"field0", std::uint8_t, 0, 0>;

std::uint32_t data0{};
using R0 = groov::reg<"reg0", std::uint32_t, &data0, F0>;
std::uint32_t data1{};
using R1 = groov::reg<"reg1", std::uint32_t, &data1, F0>;

using G = groov::group<"group", bus, R0, R1>;
} // namespace

auto main() -> int {
    using namespace groov::literals;
    constexpr auto grp = G{};
    auto r = groov::read(grp / "reg0.field0"_f, grp / "reg1.field0"_f);
    auto const result = get<0>(*async::sync_wait(r));
    return result["field1"_f];
}
