#pragma once

#include <groov/config.hpp>
#include <groov/path.hpp>
#include <groov/read_spec.hpp>
#include <groov/write_spec.hpp>

#include <async/compose.hpp>
#include <async/concepts.hpp>
#include <async/let_value.hpp>
#include <async/sync_wait.hpp>
#include <async/then.hpp>
#include <async/when_all.hpp>

#include <stdx/optional.hpp>
#include <stdx/static_assert.hpp>
#include <stdx/tuple_algorithms.hpp>
#include <stdx/type_traits.hpp>
#include <stdx/utility.hpp>

#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/list.hpp>

#include <optional>
#include <type_traits>
#include <utility>

namespace groov {
namespace detail {
template <typename Register, typename Group, typename Mask>
auto read() -> async::sender auto {
    using bus_t = typename Group::bus_t;
    return bus_t::template read<Register::name, Mask::value>(
        get_address<Register>());
}

template <typename Path> struct to_register;

template <typename Group, typename Path>
using to_register_t =
    boost::mp11::mp_eval_if_c<registerlike<resolve_t<Group, Path>>,
                              resolve_t<Group, Path>,
                              to_register<parent_t<Path>>::template fn, Group>;

template <typename Path>
    requires(Path::empty())
struct to_register<Path> {
    template <typename Group> using fn = void;
};
template <typename Path>
    requires(not Path::empty())
struct to_register<Path> {
    template <typename Group> using fn = to_register_t<Group, Path>;
};

template <typename Group> struct to_register_q {
    template <typename Path> using fn = to_register_t<Group, Path>;
};

template <typename Register> using to_fields = typename Register::children_t;

template <typename F, auto Mask> CONSTEVAL auto check_writeonly_field() {
    constexpr auto mask_overlap = F::template mask<decltype(Mask)> & Mask;
    if constexpr (registerlike<F>) {
        STATIC_ASSERT(not write_only_write_function<typename F::write_fn_t> or
                          not mask_overlap,
                      "Attempting to read from a write-only register: {}",
                      F::name);
    } else {
        STATIC_ASSERT(not write_only_write_function<typename F::write_fn_t> or
                          not mask_overlap,
                      "Attempting to read from a write-only field: {}",
                      F::name);
    }
}

template <typename Bus, typename L, typename Masks>
CONSTEVAL auto check_write_only() -> void {
    [[maybe_unused]] auto r = stdx::for_each(
        []<typename... Fs>(boost::mp11::mp_list<Fs...>, auto mask) {
            constexpr auto read_mask = transform_mask<Bus>(mask());
            (check_writeonly_field<Fs, read_mask>(), ...);
        },
        L{}, Masks{});
}

} // namespace detail

template <typename Group, typename Paths>
constexpr auto read(read_spec<Group, Paths> const &s) -> async::sender auto {
    using Spec = decltype(to_write_spec(s));

    using fields_per_reg_t = boost::mp11::mp_transform_q<
        detail::fields_for_reg_q<typename Spec::paths_t>,
        typename Spec::value_t>;

    using read_fields_per_reg_t =
        boost::mp11::mp_transform<detail::all_fields_t, fields_per_reg_t>;

    using register_values_t =
        boost::mp11::mp_apply<boost::mp11::mp_list, typename Spec::value_t>;

    using field_masks_t = boost::mp11::mp_transform_q<
        detail::field_mask_for_reg_q<typename Spec::paths_t>,
        register_values_t>;

    detail::check_write_only<
        typename Spec::bus_t, read_fields_per_reg_t,
        boost::mp11::mp_apply<stdx::tuple, field_masks_t>>();

    return []<typename... Rs, typename... Ms>(boost::mp11::mp_list<Rs...>,
                                              boost::mp11::mp_list<Ms...>) {
        return async::when_all(detail::read<Rs, Group, Ms>()...) |
               async::then(stdx::overload{
                   [](typename Rs::type_t... values) {
                       return Spec{{}, {Rs{{}, values}...}};
                   },
                   [](std::optional<typename Rs::type_t>... values) {
                       return stdx::transform(
                           [](auto... vs) { return Spec{{}, {Rs{{}, vs}...}}; },
                           values...);
                   }});
    }(register_values_t{}, field_masks_t{});
}

namespace _read {
struct pipeable {
  private:
    template <async::sender S>
    friend constexpr auto operator|(S &&s, pipeable) -> async::sender auto {
        return std::forward<S>(s) |
               async::let_value([]<typename Spec>(Spec &&spec) {
                   return read(std::forward<Spec>(spec));
               });
    }
};
} // namespace _read

constexpr auto read() { return async::compose(_read::pipeable{}); }

namespace _sync_read {
template <typename Behavior, async::sender S> [[nodiscard]] auto wait(S &&s) {
    static_assert(async::trivially_sync_waitable<S> or
                      std::is_same_v<Behavior, blocking>,
                  "sync_read() would block: if you really want this, use "
                  "sync_read<blocking>()");
    return get<0>(*(std::forward<S>(s) | async::sync_wait()));
}

template <typename Behavior> struct pipeable {
  private:
    template <async::sender S>
    friend constexpr auto operator|(S &&s, pipeable) -> decltype(auto) {
        return wait<Behavior>(std::forward<S>(s) | read());
    }
};
} // namespace _sync_read

template <typename Behavior = non_blocking, typename T>
[[nodiscard]] auto sync_read(T const &t) {
    return _sync_read::wait<Behavior>(read(t));
}

template <typename Behavior = non_blocking>
[[nodiscard]] auto sync_read() -> _sync_read::pipeable<Behavior> {
    return {};
}
} // namespace groov
