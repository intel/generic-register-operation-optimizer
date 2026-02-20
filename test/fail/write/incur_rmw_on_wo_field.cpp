#include "../dummy_bus.hpp"

#include <groov/config.hpp>
#include <groov/identity.hpp>
#include <groov/path.hpp>
#include <groov/value_path.hpp>
#include <groov/write.hpp>
#include <groov/write_spec.hpp>

#include <async/concepts.hpp>
#include <async/just_result_of.hpp>

#include <cstdint>

// indirect read from a field that is marked write-only by writing to a
// different field in the same register, incurring read-modify-write

// EXPECT: Write would incur RMW on a write-only field: field1

namespace {
struct write_bus : dummy_bus {
    template <stdx::ct_string, auto Mask, auto IdMask, auto IdValue>
    static auto write(auto addr, auto value) -> async::sender auto {
        return async::just_result_of([=] { *addr = (*addr & ~Mask) | value; });
    }
};

using F0 = groov::field<"field0", std::uint8_t, 0, 0, groov::w::replace>;
using F1 = groov::field<"field1", std::uint8_t, 1, 1,
                        groov::write_only<groov::w::replace>>;

std::uint32_t data{};
using R = groov::reg<"reg", std::uint32_t, &data, groov::w::replace, F0, F1>;
using G = groov::group<"group", write_bus, R>;
} // namespace

auto main() -> int {
    using namespace groov::literals;
    [[maybe_unused]] auto x = sync_write(G{}("reg.field0"_f = 1));
}
