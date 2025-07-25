#pragma once

#include <groov/config.hpp>
#include <groov/path.hpp>
#include <groov/read_spec.hpp>
#include <groov/write_spec.hpp>

#include <async/concepts.hpp>
#include <async/let_value.hpp>
#include <async/sync_wait.hpp>
#include <async/then.hpp>
#include <async/when_all.hpp>

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
constexpr auto read(read_spec<Group, Paths> const &s) -> async::sender auto {
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

constexpr auto read() -> _read::pipeable { return {}; }

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
