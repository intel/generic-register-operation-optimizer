#include <groov/config.hpp>
#include <groov/identity.hpp>

#include <cstdint>

// EXPECT: constraints not satisfied

namespace {
using F = groov::field<"field", std::uint8_t, 0, 0,
                       groov::read_only<groov::w::replace>>;
} // namespace

auto main() -> int {}
