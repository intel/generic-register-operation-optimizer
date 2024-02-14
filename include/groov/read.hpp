#pragma once

#include <async/concepts.hpp>
#include <async/then.hpp>
#include <async/when_all.hpp>

#include <groov/tree.hpp>
#include <groov/path.hpp>

#include <stdx/ct_string.hpp>
#include <stdx/tuple.hpp>
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

template <typename T> auto read(T) {
    using paths_by_register =
        boost::mpx::mp_gather_q<detail::register_for_path_q<T>,
                                typename T::paths_t>;
    using registers =
        boost::mp11::mp_transform_q<detail::register_for_paths_q<T>,
                                    paths_by_register>;
    using register_values =
        boost::mp11::mp_transform<reg_with_value, registers>;
    using field_masks =
        boost::mp11::mp_transform_q<detail::field_mask_for_paths_q<T>,
                                    paths_by_register>;

    return []<typename... Rs, typename... Ms>(boost::mp11::mp_list<Rs...>,
                                              boost::mp11::mp_list<Ms...>) {
        return async::when_all(detail::read<Rs, T, Ms>()...) |
               async::then([](typename Rs::type_t... values) {
                   return detail::read_result<paths_by_register, Rs...>{
                       values...};
               });
    }(register_values{}, field_masks{});
}
} // namespace groov
