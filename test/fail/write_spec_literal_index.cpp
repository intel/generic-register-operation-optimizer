#include <groov/config.hpp>
#include <groov/path.hpp>
#include <groov/resolve.hpp>
#include <groov/value_path.hpp>
#include <groov/write_spec.hpp>

#include <async/concepts.hpp>

// EXPECT: Trying to index into a write_spec with a string literal

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

using F = groov::field<"field", std::uint8_t, 0, 0>;

std::uint32_t data0{};
using R = groov::reg<"reg", std::uint32_t, &data0, groov::w::replace, F>;

using G = groov::group<"group", bus, R>;
constexpr auto grp = G{};
} // namespace

auto main() -> int {
    using namespace groov::literals;
    auto const spec = grp("reg"_r = 5);
    auto x = spec["reg"];
}
