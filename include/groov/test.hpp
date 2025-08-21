#pragma once

#define ENABLE_GROOV_TEST
#include <async/just_result_of.hpp>

#include <stdx/ct_string.hpp>
#include <stdx/utility.hpp>

#include <functional>
#include <map>
#include <optional>

namespace groov::test {

template <stdx::ct_string Group> struct storage_type {
    template <typename T> static auto error() {
        static_assert(
            stdx::always_false_v<T>,
            "Did you forget to provide a storage_type specialization? "
            "Something like template<> struct storage_type<\"group\"> { "
            " using type = make_store_type<addr_t, value_t>; };");
        return;
    }

    using type = decltype(error<void>());
};

template <stdx::ct_string Group> struct store {
  private:
    using storage_t = typename storage_type<Group>::type;
    static inline storage_t storage;

  public:
    static inline void reset() { storage.clear(); }

    static void set_value(auto addr, auto value) {
        auto &entry = storage[addr];
        if (entry.write_function) {
            entry.write_function(addr, value);
        } else {
            entry.value = value;
        }
    }

    static auto get_value(auto addr) {
        auto &entry = storage[addr];
        if (entry.read_function) {
            return entry.read_function(addr);
        } else {
            // TODO: Add X detection. The question, what to do? Throw? Handler?
            using value_t = typename decltype(entry.value)::value_type;
            return entry.value.value_or(value_t{});
        }
    }

    template <typename F> static auto set_write_function(auto addr, F &&f) {
        return storage[addr].write_function = std::forward<F>(f);
    }

    template <typename F> static auto set_read_function(auto addr, F &&f) {
        return storage[addr].read_function = std::forward<F>(f);
    }
};

namespace detail {
template <stdx::ct_string Group> struct test_bus {
    template <auto Mask> static auto read(auto addr) -> async::sender auto {
        return async::just_result_of(
            [=] { return store<Group>::get_value(addr); });
    }

    template <auto Mask, auto IdMask, auto IdValue>
    static auto write(auto addr, auto value) -> async::sender auto {
        return async::just_result_of([=] {
            auto prev = store<Group>::get_value(addr) & ~(Mask | IdMask);
            store<Group>::set_value(addr, prev | value | IdValue);
        });
    }
};

template <typename Addr, typename Value> struct test_value_t {
    std::optional<Value> value{};
    std::function<void(Addr, Value)> write_function;
    std::function<Value(Addr)> read_function;
};
} // namespace detail

template <stdx::ct_string Group, typename Bus>
using test_bus = stdx::tt_pair<stdx::cts_t<Group>, Bus>;

template <stdx::ct_string Group>
using default_test_bus =
    stdx::tt_pair<stdx::cts_t<Group>, detail::test_bus<Group>>;

template <typename... T> using make_test_bus_list = stdx::type_map<T...>;

template <typename Addr, typename Value>
using make_store_type = std::map<Addr, detail::test_value_t<Addr, Value>>;

} // namespace groov::test
