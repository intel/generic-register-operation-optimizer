#include <groov/config.hpp>
#include <groov/identity.hpp>

#include <cstdint>

// EXPECT: constraints not satisfied

namespace {
using F = groov::field<"field", std::uint8_t, 0, 0,
                       groov::write_only<groov::w::ignore>>;
} // namespace

auto main() -> int {}
