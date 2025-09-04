#include "dummy_bus.hpp"

#include <groov/config.hpp>
#include <groov/path.hpp>
#include <groov/read.hpp>

#include <async/concepts.hpp>
#include <async/schedulers/thread_scheduler.hpp>
#include <async/then.hpp>

#include <cstdint>

// EXPECT: sync_read\(\) would block

namespace {
struct read_bus : dummy_bus {
    template <stdx::ct_string, auto>
    static auto read(auto...) -> async::sender auto {
        return async::thread_scheduler{}.schedule() |
               async::then([] { return 42; });
    }
};

std::uint32_t data0{};
using R0 = groov::reg<"reg0", std::uint32_t, &data0, groov::w::replace>;

using G = groov::group<"group", read_bus, R0>;
} // namespace

auto main() -> int {
    using namespace groov::literals;
    constexpr auto grp = G{};
    [[maybe_unused]] auto const result = sync_read(grp / "reg0"_r);
}
