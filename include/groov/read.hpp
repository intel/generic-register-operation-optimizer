#pragma once

#include <async/concepts.hpp>
#include <async/then.hpp>
#include <async/when_all.hpp>

#include <groov/config.hpp>
#include <groov/path.hpp>
#include <groov/read_spec.hpp>
#include <groov/write_spec.hpp>

#include <stdx/tuple_algorithms.hpp>
#include <stdx/type_traits.hpp>

#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/list.hpp>

namespace groov {
namespace detail {
template <typename Register, typename Group, typename Mask>
auto read() -> async::sender auto {
    using bus_t = typename Group::bus_t;
    return bus_t::template read<Mask::value>(Register::address);
}
} // namespace detail

template <typename Group, typename Paths>
auto read(read_spec<Group, Paths> const &s) {
    using Spec = decltype(to_write_spec(s));

    using register_values_t =
        boost::mp11::mp_apply<boost::mp11::mp_list, typename Spec::value_t>;
    using field_masks_t = boost::mp11::mp_transform_q<
        detail::field_mask_for_reg_q<typename Spec::paths_t>,
        register_values_t>;

    return []<typename... Rs, typename... Ms>(boost::mp11::mp_list<Rs...>,
                                              boost::mp11::mp_list<Ms...>) {
        return async::when_all(detail::read<Rs, Group, Ms>()...) |
               async::then([](typename Rs::type_t... values) {
                   return Spec{{}, {Rs{{}, values}...}};
               });
    }(register_values_t{}, field_masks_t{});
}
} // namespace groov
