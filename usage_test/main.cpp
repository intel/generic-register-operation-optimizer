#include <groov/groov.hpp>

#include <async/concepts.hpp>
#include <async/just.hpp>
#include <async/just_result_of.hpp>
#include <async/sync_wait.hpp>

#include <cassert>
#include <cstdint>

namespace {
struct bus {
    template <stdx::ct_string, auto Mask, auto IdMask, auto IdValue>
    static auto write(auto addr, auto value) -> async::sender auto {
        return async::just_result_of([=] { *addr = (*addr & ~Mask) | value; });
    }

    template <stdx::ct_string, auto Mask>
    static auto read(auto addr) -> async::sender auto {
        return async::just_result_of([=] { return *addr; });
    }
};

using F0 = groov::field<"field0", std::uint8_t, 0, 0>;
using F1 = groov::field<"field1", std::uint8_t, 4, 1>;
using F2 = groov::field<"field2", std::uint8_t, 7, 5>;

std::uint32_t data{};
using R =
    groov::reg<"reg", std::uint32_t, &data, groov::w::replace, F0, F1, F2>;

using G = groov::group<"group", bus, R>;
constexpr auto grp = G{};
} // namespace

#if __STDC_HOSTED__ == 0
extern "C" auto main() -> int;
#endif

auto main() -> int {
    using namespace groov::literals;
    data = 0xa5u;
    [[maybe_unused]] auto result = async::just(grp / "reg"_r) //
                                   | groov::read()            //
                                   | async::then([](auto spec) {
                                         spec["reg"_r] ^= 0xff;
                                         return spec;
                                     })             //
                                   | groov::write() //
                                   | async::sync_wait();
    assert(data == 0x5au);
}
