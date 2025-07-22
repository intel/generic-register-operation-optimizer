#pragma once

#include <groov/config.hpp>
#include <groov/identity.hpp>

#include <async/concepts.hpp>
#include <async/let_value.hpp>
#include <async/sync_wait.hpp>
#include <async/when_all.hpp>

#include <stdx/compiler.hpp>
#include <stdx/static_assert.hpp>
#include <stdx/tuple_algorithms.hpp>

#include <boost/mp11/algorithm.hpp>

#include <limits>
#include <type_traits>
#include <utility>

template <typename...> struct undef;
template <auto...> struct undef_v;

namespace groov {
namespace detail {
template <typename Register, typename Bus, auto Mask, auto IdMask, auto IdValue,
          typename V>
auto write(V value) -> async::sender auto {
    return Bus::template write<Mask, IdMask, IdValue>(Register::address, value);
}

template <typename Reg, typename ObjList>
using compute_mask_t = bitwise_accum_t<ObjList, mask_q, Reg>;

template <typename Reg, typename ObjList>
using compute_id_mask_t = bitwise_accum_t<ObjList, id_mask_q, Reg>;

template <typename Reg, typename ObjList>
using compute_id_value_t = bitwise_accum_t<ObjList, id_value_q, Reg>;

template <typename Reg, typename ObjList>
using compute_id_value_t = bitwise_accum_t<ObjList, id_value_q, Reg>;

template <typename Reg>
using compute_reg_id_mask_t =
    std::integral_constant<typename Reg::type_t, Reg::unused_mask>;

template <typename Reg>
using compute_reg_id_value_t =
    std::integral_constant<typename Reg::type_t, Reg::unused_identity>;

template <typename F> CONSTEVAL auto check_readonly_field() {
    STATIC_ASSERT(not read_only_write_function<typename F::write_fn_t>,
                  "Attempting to write to a read-only field: {}", F::name);
}

template <typename L> CONSTEVAL auto check_read_only() -> void {
    [[maybe_unused]] auto r = stdx::for_each(
        []<typename... Fs>(boost::mp11::mp_list<Fs...>) {
            (check_readonly_field<Fs>(), ...);
        },
        L{});
}
} // namespace detail

template <typename Spec> auto write(Spec const &s) -> async::sender auto {
    using fields_per_reg_t = boost::mp11::mp_transform_q<
        detail::fields_for_reg_q<typename Spec::paths_t>,
        typename Spec::value_t>;

    using written_fields_per_reg_t =
        boost::mp11::mp_transform<detail::all_fields_t, fields_per_reg_t>;
    detail::check_read_only<written_fields_per_reg_t>();

    using all_fields_per_reg_t = boost::mp11::mp_transform<
        detail::all_fields_t,
        boost::mp11::mp_transform<boost::mp11::mp_list,
                                  typename Spec::value_t>>;

    using unwritten_fields_per_reg_t =
        boost::mp11::mp_transform<boost::mp11::mp_set_difference,
                                  all_fields_per_reg_t,
                                  written_fields_per_reg_t>;

    using field_masks_t = boost::mp11::mp_transform<detail::compute_mask_t,
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

    using reg_identity_masks_t =
        boost::mp11::mp_transform<detail::compute_reg_id_mask_t,
                                  typename Spec::value_t>;
    using reg_identity_values_t =
        boost::mp11::mp_transform<detail::compute_reg_id_value_t,
                                  typename Spec::value_t>;

    return stdx::transform(
               []<typename R, typename Mask, typename IdMask, typename IdValue,
                  typename RegIdMask, typename RegIdValue>(
                   R const &r, Mask, IdMask, IdValue, RegIdMask, RegIdValue) {
                   return detail::write<R, typename Spec::bus_t, Mask::value,
                                        (IdMask::value | RegIdMask::value),
                                        (IdValue::value | RegIdValue::value)>(
                       r.value);
               },
               s.value, field_masks_t{}, identity_masks_t{},
               identity_values_t{}, reg_identity_masks_t{},
               reg_identity_values_t{})
        .apply([]<typename... Ws>(Ws &&...ws) {
            return async::when_all(std::forward<Ws>(ws)...);
        });
}

namespace _write {
struct pipeable {
  private:
    template <async::sender S>
    friend constexpr auto operator|(S &&s, pipeable) -> async::sender auto {
        return std::forward<S>(s) |
               async::let_value([=]<typename Spec>(Spec &&spec) {
                   return write(std::forward<Spec>(spec));
               });
    }
};
} // namespace _write

constexpr auto write() -> _write::pipeable { return {}; }

namespace _sync_write {
template <typename Behavior, async::sender S> [[nodiscard]] auto wait(S &&s) {
    static_assert(async::trivially_sync_waitable<S> or
                      std::is_same_v<Behavior, blocking>,
                  "sync_write() would block: if you really want this, use "
                  "sync_write<blocking>()");
    return std::forward<S>(s) | async::sync_wait();
}

template <typename Behavior> struct pipeable {
  private:
    template <async::sender S>
    friend constexpr auto operator|(S &&s, pipeable) -> decltype(auto) {
        return wait<Behavior>(std::forward<S>(s) | write());
    }
};
} // namespace _sync_write

template <typename Behavior = non_blocking, typename T>
[[nodiscard]] auto sync_write(T const &t) {
    return _sync_write::wait<Behavior>(write(t));
}

template <typename Behavior = non_blocking>
[[nodiscard]] auto sync_write() -> _sync_write::pipeable<Behavior> {
    return {};
}
} // namespace groov
