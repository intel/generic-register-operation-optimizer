#include "dummy_bus.hpp"

#include <groov/config.hpp>
#include <groov/identity.hpp>
#include <groov/path.hpp>
#include <groov/value_path.hpp>
#include <groov/write.hpp>
#include <groov/write_spec.hpp>

#include <async/concepts.hpp>
#include <async/just_result_of.hpp>

#include <cstdint>

// EXPECT: Attempting to write to a read-only field: field_ro

namespace {
struct write_bus : dummy_bus {
    template <stdx::ct_string, auto Mask, auto IdMask, auto IdValue>
    static auto write(auto addr, auto value) -> async::sender auto {
        return async::just_result_of([=] { *addr = (*addr & ~Mask) | value; });
    }
};

using F0 = groov::field<"field_ro", std::uint8_t, 0, 0,
                        groov::read_only<groov::w::ignore>>;
using F1 = groov::field<"field_rw", std::uint32_t, 31, 1>;

std::uint32_t data{};
using R = groov::reg<"reg", std::uint32_t, &data, groov::w::replace, F0, F1>;
using G = groov::group<"group", write_bus, R>;
} // namespace

auto main() -> int {
    using namespace groov::literals;
    [[maybe_unused]] auto x = sync_write(G{}("reg"_r = 0xffff'ffff));
}
