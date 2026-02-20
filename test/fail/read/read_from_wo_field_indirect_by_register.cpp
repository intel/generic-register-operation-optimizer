#include "../dummy_bus.hpp"

#include <groov/config.hpp>
#include <groov/identity.hpp>
#include <groov/path.hpp>
#include <groov/read.hpp>

#include <async/concepts.hpp>
#include <async/just.hpp>

#include <cstdint>

// indirect read from a field that is marked write-only by reading from the
// containing register

// Note: even though indirect reading from WO fields is generally OK, in this
// case it is dangerous. Because we are reading the whole register, the
// resulting write_spec just contains the register path without any information
// about the fields. This means the following could occur:

/*
// After read, x contains "reg" and a value with indeterminate WO bits
auto x = sync_read(G{}("reg"_f));

// This writes back the indeterminate bits!
sync_write(x);

// And there is no way for us to prevent this since to groov this is just like:
sync_write(G{}("reg"_f = value))
// where we are obviously and correctly intending to write all the bits.
*/

// In a sense, failing on this is protecting us from the danger of "lazy" code.
// If we actually want to achieve the safe thing here, we could specify all the
// non-WO fields to read. Then x would contain only the correct paths, and
// writing back would be correct (as long as it doesn't incur a RMW).

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

std::uint32_t data{};
using R = groov::reg<"reg", std::uint32_t, &data, groov::w::replace, F0>;
using G = groov::group<"group", read_bus, R>;
} // namespace

auto main() -> int {
    using namespace groov::literals;
    [[maybe_unused]] auto x = sync_read(G{}("reg"_f));
}
