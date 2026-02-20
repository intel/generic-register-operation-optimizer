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

// write to a register that is marked read-only

// EXPECT: Attempting to write to a read-only register: reg

namespace {
struct write_bus : dummy_bus {
    template <stdx::ct_string, auto Mask, auto IdMask, auto IdValue>
    static auto write(auto addr, auto value) -> async::sender auto {
        return async::just_result_of([=] { *addr = (*addr & ~Mask) | value; });
    }
};

std::uint32_t data{};
using R =
    groov::reg<"reg", std::uint32_t, &data, groov::read_only<groov::w::ignore>>;
using G = groov::group<"group", write_bus, R>;
} // namespace

auto main() -> int {
    using namespace groov::literals;
    [[maybe_unused]] auto x = sync_write(G{}("reg"_f = 1));
}
