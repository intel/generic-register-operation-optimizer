#pragma once

#include <async/concepts.hpp>
#include <async/when_all.hpp>

#include <groov/config.hpp>

#include <stdx/tuple_algorithms.hpp>

#include <boost/mp11/algorithm.hpp>

namespace groov {
namespace detail {
template <typename Register, typename Tree, typename Mask, typename V>
auto write(V value) -> async::sender auto {
    using bus_t = typename Tree::bus_t;
    return bus_t::template write<Mask::value>(Register::address, value);
}
} // namespace detail

template <typename Spec> auto write(Spec const &s) {
    using field_masks_t = boost::mp11::mp_transform_q<
        detail::field_mask_for_reg_q<typename Spec::paths_t>,
        typename Spec::value_t>;

    return stdx::transform(
               []<typename R, typename M>(R const &r, M) {
                   return detail::write<R, Spec, M>(r.value);
               },
               s.value, field_masks_t{})
        .apply([]<typename... Ws>(Ws &&...ws) {
            return async::when_all(std::forward<Ws>(ws)...);
        });
}
} // namespace groov
