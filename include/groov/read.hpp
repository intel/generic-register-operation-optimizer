#pragma once

#include <async/concepts.hpp>
#include <async/then.hpp>

#include <groov/identifier.hpp>
#include <groov/object.hpp>

namespace groov {
template <typename Object> constexpr auto extract(auto value) {
    return Object::extract(value);
}

template <typename Object, stdx::ct_string Child, stdx::ct_string... Fields>
constexpr auto extract(auto value) {
    using child_t = get_child<Child, Object>;
    return extract<child_t, Fields...>(value);
}

template <typename Group, stdx::ct_string Reg, stdx::ct_string... Fields>
auto read(path<Reg, Fields...>) -> async::sender auto {
    using bus_t = typename Group::bus_t;
    using register_t = get_child<Reg, Group>;
    return bus_t::read(register_t::address) | async::then([](auto value) {
               return extract<register_t, Fields...>(value);
           });
}
} // namespace groov
