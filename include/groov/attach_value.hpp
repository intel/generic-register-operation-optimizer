#pragma once

#include <stdx/type_traits.hpp>

#include <utility>

namespace groov {
constexpr inline struct attach_value_t {
    template <typename...> struct failure_t {};

    template <typename... Ts>
        requires true
    constexpr auto operator()(Ts &&...ts) const
        noexcept(noexcept(tag_invoke(std::declval<attach_value_t>(),
                                     std::forward<Ts>(ts)...)))
            -> decltype(tag_invoke(*this, std::forward<Ts>(ts)...)) {
        return tag_invoke(*this, std::forward<Ts>(ts)...);
    }

    template <typename... Ts>
    constexpr auto operator()(Ts &&...) const -> failure_t<Ts...> {
        static_assert(stdx::always_false_v<Ts...>,
                      "No function call for attach_value: did you include the "
                      "value type?");
        return {};
    }
} attach_value{};
} // namespace groov
