#include <groov/config.hpp>
#include <groov/identity.hpp>
#include <groov/path.hpp>
#include <groov/value_path.hpp>
#include <groov/write.hpp>
#include <groov/write_spec.hpp>

#include <async/concepts.hpp>
#include <async/just_result_of.hpp>

#include <cstdint>

// EXPECT: Attempting to write to a read-only field: field

namespace {
struct bus {
    struct dummy_sender {
        using is_sender = void;
    };
    template <auto> static auto read(auto...) -> async::sender auto {
        return dummy_sender{};
    }

    template <auto Mask, auto IdMask, auto IdValue>
    static auto write(auto addr, auto value) -> async::sender auto {
        return async::just_result_of([=] { *addr = (*addr & ~Mask) | value; });
    }
};

using F = groov::field<"field", std::uint8_t, 0, 0,
                       groov::read_only<groov::w::ignore>>;

std::uint32_t data{};
using R = groov::reg<"reg", std::uint32_t, &data, groov::w::replace, F>;
using G = groov::group<"group", bus, R>;
} // namespace

auto main() -> int {
    using namespace groov::literals;
    [[maybe_unused]] auto x = sync_write(G{}("reg.field"_f = 1));
}
