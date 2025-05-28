#include <groov/identity.hpp>
#include <groov/path.hpp>
#include <groov/value_path.hpp>
#include <groov/write.hpp>
#include <groov/write_spec.hpp>

#include <async/just_result_of.hpp>
#include <async/sync_wait.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>

namespace {
struct bus {
    static inline int num_writes{};

    template <auto Mask, auto IdMask, auto IdValue>
    static auto write(auto addr, auto value) -> async::sender auto {
        return async::just_result_of([=] {
            CHECK(Mask == 0b10);
            CHECK((Mask | IdMask) == 0xffu);
            ++num_writes;
            auto prev = *addr & ~(Mask | IdMask);
            *addr = prev | value | IdValue;
        });
    }

    struct sender {
        using is_sender = void;
    };

    template <auto> static auto read(auto...) -> async::sender auto {
        return sender{};
    }
};

template <typename T> auto sync_write(T const &t) {
    return groov::write(t) | async::sync_wait();
}

struct custom_field_func {
    struct id_spec {
        template <std::unsigned_integral T, std::size_t Msb, std::size_t Lsb>
        constexpr static auto identity() -> T {
            static_assert(Msb == 7);
            static_assert(Lsb == 2);
            return 0b0110'0000u;
        }
    };
};

using F0 = groov::field<"reserved0", std::uint8_t, 0, 0, groov::w::ignore>;
using F1 = groov::field<"field", std::uint8_t, 1, 1>;
using F2 = groov::field<"reserved1", std::uint8_t, 7, 2, custom_field_func>;

std::uint32_t data{};
using R =
    groov::reg<"reg", std::uint32_t, &data, groov::w::replace, F0, F1, F2>;

using G = groov::group<"group", bus, R>;
constexpr auto grp = G{};
} // namespace

TEST_CASE("write a register with a custom write function field",
          "[write_functions]") {
    using namespace groov::literals;
    bus::num_writes = 0;
    data = 0b0100'0000u;
    sync_write(grp("reg.field"_f = 1));
    CHECK(bus::num_writes == 1);
    CHECK(data == 0b0110'0010);
}
