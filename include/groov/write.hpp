#pragma once

#include <groov/config.hpp>
#include <groov/identity.hpp>

#include <async/concepts.hpp>
#include <async/just.hpp>
#include <async/let_value.hpp>
#include <async/sync_wait.hpp>
#include <async/when_all.hpp>

#include <stdx/compiler.hpp>
#include <stdx/concepts.hpp>
#include <stdx/static_assert.hpp>
#include <stdx/tuple.hpp>
#include <stdx/tuple_algorithms.hpp>

#include <boost/mp11/algorithm.hpp>

#include <type_traits>
#include <utility>

namespace groov {
namespace detail {
template <typename Register, typename Bus, auto Mask, auto IdMask, auto IdValue,
          typename V>
auto write(V value) -> async::sender auto {
    constexpr auto write_mask = transform_mask<Bus>(Mask);
    constexpr auto id_mask = write_mask & IdMask;
    STATIC_ASSERT(write_mask == (Mask | id_mask) or
                      not is_write_only<Register>::value,
                  "Write to register {} would incur RMW on write-only bits",
                  Register::name);

    return Bus::template write<Register::name, Mask, id_mask, IdValue>(
        get_address<Register>(), value);
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
    std::integral_constant<typename Reg::type_t, Reg::unused_identity_value>;

template <typename F> CONSTEVAL auto check_readonly_field() {
    if constexpr (registerlike<F>) {
        STATIC_ASSERT(not read_only_write_function<typename F::write_fn_t>,
                      "Attempting to write to a read-only register: {}",
                      F::name);
    } else {
        STATIC_ASSERT(not read_only_write_function<typename F::write_fn_t>,
                      "Attempting to write to a read-only field: {}", F::name);
    }
}

template <typename Bus, typename L> CONSTEVAL auto check_read_only() -> void {
    [[maybe_unused]] auto r = stdx::for_each(
        []<typename... Fs>(boost::mp11::mp_list<Fs...>) {
            (check_readonly_field<Fs>(), ...);
        },
        L{});
}

template <typename F, auto Mask> CONSTEVAL auto check_rmw_field() {
    constexpr auto mask_overlap = F::template mask<decltype(Mask)> & Mask;
    STATIC_ASSERT(not write_only_write_function<typename F::write_fn_t> or
                      not mask_overlap,
                  "Write would incur RMW on a write-only field: {}", F::name);
}

template <typename Bus, typename L, typename Masks>
CONSTEVAL auto check_rmw() -> void {
    [[maybe_unused]] auto r = stdx::for_each(
        []<typename... Fs>(boost::mp11::mp_list<Fs...>, auto mask) {
            [[maybe_unused]] constexpr auto write_mask =
                transform_mask<Bus>(mask());
            (check_rmw_field<Fs, write_mask>(), ...);
        },
        L{}, Masks{});
}

template <typename T>
concept write_spec_like =
    requires { typename std::remove_cvref_t<T>::is_write_spec; };
} // namespace detail

template <detail::write_spec_like Spec>
auto write(Spec const &s) -> async::sender auto {
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

    using field_masks_t = boost::mp11::mp_transform<detail::compute_mask_t,
                                                    typename Spec::value_t,
                                                    written_fields_per_reg_t>;

    detail::check_read_only<typename Spec::bus_t, fields_per_reg_t>();
    detail::check_rmw<typename Spec::bus_t, unwritten_fields_per_reg_t,
                      field_masks_t>();

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

template <detail::write_spec_like Spec, typename... Args>
    requires(sizeof...(Args) > 0)
auto write(Spec const &s, Args &&...args) -> async::sender auto {
    return write(s) |
           async::let_value([... as = std::forward<Args>(args)](auto &&...rs) {
               return async::just(FWD(rs)..., std::move(as)...);
           });
}

namespace _write {
template <typename... Ts> struct pipeable {
    stdx::tuple<std::decay_t<Ts>...> passthrough_values{};

  private:
    template <async::sender S, stdx::same_as_unqualified<pipeable> P>
    friend constexpr auto operator|(S &&s, P &&p) -> async::sender auto {
        return std::forward<S>(s) |
               async::let_value(
                   [values = std::forward<P>(p).passthrough_values]<
                       detail::write_spec_like Spec>(Spec &&spec) {
                       return std::move(values).apply([&](auto &&...vs) {
                           return write(std::forward<Spec>(spec), FWD(vs)...);
                       });
                   });
    }
};
} // namespace _write

template <typename... Args>
    requires(... and (not detail::write_spec_like<Args>))
constexpr auto write(Args &&...args) {
    return async::compose(
        _write::pipeable<Args...>{std::forward<Args>(args)...});
}

namespace _sync_write {
template <typename T> struct [[nodiscard]] async_write_result : T {};
template <typename T> async_write_result(T) -> async_write_result<T>;

template <typename Behavior, async::sender S> auto wait(S &&s) {
    static_assert(async::trivially_sync_waitable<S> or
                      std::is_same_v<Behavior, blocking>,
                  "sync_write() would block: if you really want this, use "
                  "sync_write<blocking>()");
    if constexpr (std::is_same_v<Behavior, blocking>) {
        return async_write_result{std::forward<S>(s) | async::sync_wait()};
    } else {
        return std::forward<S>(s) | async::sync_wait();
    }
}

template <typename Behavior, typename... Ts> struct pipeable {
    stdx::tuple<std::decay_t<Ts>...> passthrough_values{};

  private:
    template <async::sender S, stdx::same_as_unqualified<pipeable> P>
    friend constexpr auto operator|(S &&s, P &&p) -> decltype(auto) {
        return wait<Behavior>(
            std::forward<S>(s) |
            std::forward<P>(p).passthrough_values.apply(
                [](auto &&...args) { return write(FWD(args)...); }));
    }
};
} // namespace _sync_write

template <typename Behavior = non_blocking, typename... Ts>
auto sync_write(Ts &&...ts) {
    return _sync_write::wait<Behavior>(write(std::forward<Ts>(ts)...));
}

template <typename Behavior = non_blocking, typename... Args>
    requires(... and (not detail::write_spec_like<Args>))
auto sync_write(Args &&...args) -> _sync_write::pipeable<Behavior, Args...> {
    return {std::forward<Args>(args)...};
}
} // namespace groov
