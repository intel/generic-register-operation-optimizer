#include "dummy_bus.hpp"

#include <groov/config.hpp>
#include <groov/identity.hpp>
#include <groov/path.hpp>
#include <groov/read.hpp>

#include <async/concepts.hpp>
#include <async/just.hpp>

#include <cstdint>

// EXPECT: Read from register reg containing write-only bits

namespace {
struct read_bus : dummy_bus {
    template <stdx::ct_string, auto>
    static auto read(auto...) -> async::sender auto {
        return async::just(42);
    }
};

using F = groov::field<"field", std::uint8_t, 0, 0, groov::w::replace>;

std::uint32_t data{};
using R = groov::reg<"reg", std::uint32_t, &data,
                     groov::write_only<groov::w::replace>, F>;
using G = groov::group<"group", read_bus, R>;
} // namespace

auto main() -> int {
    using namespace groov::literals;
    [[maybe_unused]] auto x = sync_read(G{}("reg.field"_f));
}
