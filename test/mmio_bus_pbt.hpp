#pragma once

#include <async/just.hpp>
#include <async/just_result_of.hpp>
#include <async/sync_wait.hpp>
#include <async/then.hpp>

#include <groov/mmio_bus.hpp>
#include <groov/path.hpp>

namespace test {
uint64_t volatile reg64{};
auto addr64 = reinterpret_cast<std::uintptr_t>(&reg64);

template <typename T, T mask, T id_mask, T id_value>
void bus_write(std::uintptr_t addr64, T write_value) {
    groov::mmio_bus<>::write<mask, id_mask, id_value>(addr64, write_value) |
        async::sync_wait();
}
} // namespace test
