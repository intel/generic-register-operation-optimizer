#pragma once

#define ENABLE_GROOV_TEST

#include <groov/resolve.hpp>

#include <async/concepts.hpp>
#include <async/just_result_of.hpp>

#include <stdx/ct_string.hpp>
#include <stdx/optional.hpp>

#include <memory>
#include <unordered_map>

namespace groov {
namespace test {
namespace detail {
template <typename T> constexpr int type_id_{};
template <typename T> constexpr void const *type_id = &type_id_<T>;

struct value {
    template <typename T>
    value(T &&t) : p{new derived<std::remove_cvref_t<T>>(std::forward<T>(t))} {}

    template <typename T> auto get() const -> std::optional<T> {
        if (auto ptr = p->get(type_id<T>); ptr) {
            return *static_cast<T const *>(ptr);
        }
        return {};
    }

    auto operator==(value const &rhs) const -> bool { return p->compare(rhs); }
    auto hash() const -> std::size_t { return p->hash(); }

    struct base {
        virtual ~base() {}
        virtual auto get(void const *) const -> void const * = 0;
        virtual auto compare(value const &) const -> bool = 0;
        virtual auto hash() const -> std::size_t = 0;
    };

    template <typename T> struct derived : base {
        constexpr derived(T t) : val{std::move(t)} {}

        auto get(void const *ti) const -> void const * override {
            if (ti != type_id<T>) {
                return nullptr;
            }
            return std::addressof(val);
        }

        auto compare(value const &v) const -> bool override {
            auto o = v.get<T>();
            return o.has_value() and *o == val;
        }

        auto hash() const -> std::size_t override {
            return std::hash<T>{}(val);
        }

        T val;
    };

    std::unique_ptr<base> p{};
};

template <typename T> auto get(value const &v) -> std::optional<T> {
    return v.template get<T>();
}
} // namespace detail
} // namespace test
} // namespace groov

template <> struct std::hash<groov::test::detail::value> {
    auto operator()(groov::test::detail::value const &v) const -> std::size_t {
        return v.hash();
    }
};

namespace groov {
namespace test {

template <stdx::ct_string Group> struct store {
    static auto reset() { state.clear(); }

    static void set_value(auto addr, auto value) {
        auto &entry = state[addr];
        if (entry.write_function) {
            entry.write_function(addr, value);
        } else {
            entry.value = value;
        }
    }

    template <typename T> static auto get_value(auto addr) -> std::optional<T> {
        auto &entry = state[addr];
        if (entry.read_function) {
            return entry.read_function(addr).template get<T>();
        } else {
            if (entry.value) {
                return entry.value->template get<T>();
            } else {
                return {};
            }
        }
    }

    template <typename F> static auto set_write_function(auto addr, F &&f) {
        return state[addr].write_function = std::forward<F>(f);
    }

    template <typename F> static auto set_read_function(auto addr, F &&f) {
        return state[addr].read_function = std::forward<F>(f);
    }

  private:
    struct store_value_t {
        std::function<void(detail::value, detail::value)> write_function;
        std::function<detail::value(detail::value)> read_function;
        std::optional<detail::value> value{};
    };

    static inline std::unordered_map<detail::value, store_value_t> state{};
};

namespace detail {
template <typename T> constexpr auto get_address(T value) {
    if constexpr (requires { value(); } and not requires { T::value; }) {
        return value();
    } else {
        return value;
    }
}
} // namespace detail

template <typename Group> void reset_store() { store<Group::name>::reset(); }

template <typename Group> void reset_store(Group) { reset_store<Group>(); }

template <typename Group, pathlike P, typename V> void set_value(P p, V value) {
    using Store = store<Group::name>;
    using Reg = decltype(Group::resolve(p));
    using reg_value_t = typename Reg::type_t;
    auto addr = detail::get_address(Reg::address);
    Store::set_value(addr, static_cast<reg_value_t>(value));
}

template <typename Group, pathlike P, typename V>
void set_value(Group, P p, V value) {
    set_value<Group>(p, value);
}

template <typename Group, pathlike P> auto get_value(P p) {
    using Store = store<Group::name>;
    using Reg = decltype(Group::resolve(p));
    using reg_value_t = typename Reg::type_t;
    auto addr = detail::get_address(Reg::address);
    return Store::template get_value<reg_value_t>(addr);
}

template <typename Group, pathlike P> auto get_value(Group, P p) {
    return get_value<Group>(p);
}

template <typename Group, pathlike P, typename F>
void set_write_function(P p, F &&f) {
    using Store = store<Group::name>;
    using Reg = decltype(Group::resolve(p));
    auto addr = detail::get_address(Reg::address);
    Store::set_write_function(addr, std::forward<F>(f));
}

template <typename Group, pathlike P, typename F>
void set_write_function(Group, P p, F &&f) {
    set_write_function<Group>(p, std::forward<F>(f));
}

template <typename Group, pathlike P, typename F>
void set_read_function(P p, F &&f) {
    using Store = store<Group::name>;
    using Reg = decltype(Group::resolve(p));
    auto addr = detail::get_address(Reg::address);
    Store::set_read_function(addr, std::forward<F>(f));
}

template <typename Group, pathlike P, typename F>
void set_read_function(Group, P p, F &&f) {
    set_read_function<Group>(p, std::forward<F>(f));
}

struct optional_policy {
    template <stdx::ct_string, auto> auto operator()(auto, auto value) {
        return value;
    }
};

template <stdx::ct_string Group, typename XPolicy = optional_policy>
struct bus {
    template <stdx::ct_string RegName, auto Mask>
    static auto read(auto addr) -> async::sender auto {
        using T = decltype(Mask);
        return async::just_result_of([=] {
            return XPolicy{}.template operator()<RegName, Mask>(
                addr, [&]() -> std::optional<T> {
                    return store<Group>::template get_value<T>(addr);
                }());
        });
    }

    template <stdx::ct_string, auto Mask, auto IdMask, auto IdValue>
    static auto write(auto addr, auto val) -> async::sender auto {
        using T = decltype(Mask);
        return async::just_result_of([=]() -> void {
            constexpr T mask = Mask | IdMask;
            T write_bits = val | IdValue;

            auto prev = store<Group>::template get_value<T>(addr).value_or(T{});
            store<Group>::set_value(addr, (prev & ~mask) | write_bits);
        });
    }
};

template <stdx::ct_string Group, typename Bus>
using test_bus = stdx::tt_pair<stdx::cts_t<Group>, Bus>;

template <stdx::ct_string Group>
using default_test_bus = stdx::tt_pair<stdx::cts_t<Group>, bus<Group>>;

template <typename... T> using make_test_bus_list = stdx::type_map<T...>;

} // namespace test
} // namespace groov
