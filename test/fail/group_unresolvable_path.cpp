#include <groov/config.hpp>
#include <groov/path.hpp>
#include <groov/read_spec.hpp>

#include <cstdint>

// EXPECT: Unresolvable path\(s\) \[reg0.field1\] passed to group \[group\]

namespace {
struct bus {
    struct dummy_sender {
        using is_sender = void;
    };
    template <auto> static auto read(auto...) -> async::sender auto {
        return dummy_sender{};
    }
    template <auto...> static auto write(auto...) -> async::sender auto {
        return dummy_sender{};
    }
};

using F0 = groov::field<"field0", std::uint8_t, 0, 0>;

std::uint32_t data0{};
using R0 = groov::reg<"reg0", std::uint32_t, &data0, groov::w::replace, F0>;
std::uint32_t data1{};
using R1 = groov::reg<"reg1", std::uint32_t, &data1, groov::w::replace, F0>;

using G = groov::group<"group", bus, R0, R1>;
} // namespace

auto main() -> int {
    using namespace groov::literals;
    constexpr auto grp = G{};
    constexpr auto x = grp("reg0.field1"_f);
}
