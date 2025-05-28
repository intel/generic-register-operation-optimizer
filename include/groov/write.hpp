#pragma once

#include <groov/config.hpp>

#include <async/concepts.hpp>
#include <async/let_value.hpp>
#include <async/when_all.hpp>

#include <stdx/tuple_algorithms.hpp>

#include <boost/mp11/algorithm.hpp>

namespace groov {
namespace detail {
template <typename Register, typename Spec, typename Mask, typename IdMask,
          typename IdValue, typename V>
auto write(V value) -> async::sender auto {
    using bus_t = typename Spec::bus_t;
    return bus_t::template write<Mask::value, IdMask::value, IdValue::value>(
        Register::address, value);
}

template <typename Reg, typename ObjList>
using compute_mask_t = bitwise_accum_t<ObjList, mask_q, Reg>;

template <typename Reg, typename ObjList>
using compute_id_mask_t = bitwise_accum_t<ObjList, id_mask_q, Reg>;

template <typename Reg, typename ObjList>
using compute_id_value_t = bitwise_accum_t<ObjList, id_value_q, Reg>;
} // namespace detail

constexpr inline struct write_t {
    template <typename Spec>
    auto operator()(Spec const &s) const -> async::sender auto {
        using fields_per_reg_t = boost::mp11::mp_transform_q<
            detail::fields_for_reg_q<typename Spec::paths_t>,
            typename Spec::value_t>;

        using written_fields_per_reg_t =
            boost::mp11::mp_transform<detail::all_fields_t, fields_per_reg_t>;

        using all_fields_per_reg_t = boost::mp11::mp_transform<
            detail::all_fields_t,
            boost::mp11::mp_transform<boost::mp11::mp_list,
                                      typename Spec::value_t>>;

        using unwritten_fields_per_reg_t =
            boost::mp11::mp_transform<boost::mp11::mp_set_difference,
                                      all_fields_per_reg_t,
                                      written_fields_per_reg_t>;

        using field_masks_t =
            boost::mp11::mp_transform<detail::compute_mask_t,
                                      typename Spec::value_t,
                                      written_fields_per_reg_t>;

        using identity_masks_t =
            boost::mp11::mp_transform<detail::compute_id_mask_t,
                                      typename Spec::value_t,
                                      unwritten_fields_per_reg_t>;

        using identity_values_t =
            boost::mp11::mp_transform<detail::compute_id_value_t,
                                      typename Spec::value_t,
                                      unwritten_fields_per_reg_t>;

        return stdx::transform(
                   []<typename R, typename Mask, typename IdMask,
                      typename IdValue>(R const &r, Mask, IdMask, IdValue) {
                       return detail::write<R, Spec, Mask, IdMask, IdValue>(
                           r.value);
                   },
                   s.value, field_masks_t{}, identity_masks_t{},
                   identity_values_t{})
            .apply([]<typename... Ws>(Ws &&...ws) {
                return async::when_all(std::forward<Ws>(ws)...);
            });
    }

  private:
    template <async::sender S>
    friend constexpr auto operator|(S &&s, write_t self) -> async::sender auto {
        return std::forward<S>(s) |
               async::let_value([=]<typename Spec>(Spec &&spec) {
                   return self(std::forward<Spec>(spec));
               });
    }
} write{};
} // namespace groov
