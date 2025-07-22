#include <groov/config.hpp>
#include <groov/path.hpp>
#include <groov/value_path.hpp>
#include <groov/write.hpp>
#include <groov/write_spec.hpp>

#include <async/concepts.hpp>
#include <async/schedulers/thread_scheduler.hpp>
#include <async/then.hpp>

#include <cstdint>

// EXPECT: sync_write\(\) would block

namespace {
struct bus {
    struct dummy_sender {
        using is_sender = void;
    };
    template <auto> static auto read(auto...) -> async::sender auto {
        return dummy_sender{};
    }
    template <auto...> static auto write(auto...) -> async::sender auto {
        return async::thread_scheduler{}.schedule();
    }
};

std::uint32_t data0{};
using R0 = groov::reg<"reg0", std::uint32_t, &data0, groov::w::replace>;

using G = groov::group<"group", bus, R0>;
} // namespace

auto main() -> int {
    using namespace groov::literals;
    constexpr auto grp = G{};
    [[maybe_unused]] auto const result = sync_write(grp("reg0"_r = 0));
}
