#pragma once

#include <async/concepts.hpp>
#include <async/just_result_of.hpp>

#include <memory>
#include <optional>
#include <unordered_map>

namespace groov {
namespace test {
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
} // namespace test
} // namespace groov

template <> struct std::hash<groov::test::value> {
    auto operator()(groov::test::value const &v) const -> std::size_t {
        return v.hash();
    }
};

namespace groov {
namespace test {
struct optional_policy {
    template <auto> auto operator()(auto, auto value) { return value; }
};

template <typename XPolicy = optional_policy> struct bus {
    static auto reset() { state.clear(); }

    template <auto Mask> static auto read(auto addr) -> async::sender auto {
        using T = decltype(Mask);
        return async::just_result_of([=] {
            return XPolicy{}.template operator()<Mask>(
                addr, [&]() -> std::optional<T> {
                    if (auto i = state.find(addr); i != std::cend(state)) {
                        return i->second.template get<T>();
                    }
                    return {};
                }());
        });
    }

    template <auto Mask, auto IdMask, auto IdValue>
    static auto write(auto addr, auto val) -> async::sender auto {
        using T = decltype(Mask);
        return async::just_result_of([=]() -> void {
            constexpr T mask = Mask | IdMask;
            T write_bits = val | IdValue;

            auto i = state.find(addr);
            if (i != std::cend(state)) {
                auto prev = i->second.template get<T>().value();
                prev &= ~mask;
                i->second = prev | write_bits;
            } else {
                state.emplace(addr, write_bits);
            }
        });
    }

  private:
    static inline std::unordered_map<value, value> state{};
};
} // namespace test
} // namespace groov
