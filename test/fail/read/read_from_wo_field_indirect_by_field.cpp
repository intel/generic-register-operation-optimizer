#include "../dummy_bus.hpp"

#include <groov/config.hpp>
#include <groov/identity.hpp>
#include <groov/path.hpp>
#include <groov/read.hpp>

#include <async/concepts.hpp>
#include <async/just.hpp>

#include <cstdint>

// indirect read from a field that is marked write-only by reading from a
// different field in the same register

// EXPECT: Attempting to read from a write-only field: field_wo

namespace {
struct read_bus : dummy_bus {
    template <stdx::ct_string, auto>
    static auto read(auto...) -> async::sender auto {
        return async::just(42);
    }
};

using F0 = groov::field<"field_wo", std::uint8_t, 0, 0,
                        groov::write_only<groov::w::replace>>;
using F1 = groov::field<"field_rw", std::uint32_t, 1, 1>;

std::uint32_t data{};
using R = groov::reg<"reg", std::uint32_t, &data, groov::w::replace, F0, F1>;
using G = groov::group<"group", read_bus, R>;
} // namespace

auto main() -> int {
    using namespace groov::literals;
    [[maybe_unused]] auto x = sync_read(G{}("reg.field_rw"_f));
}
