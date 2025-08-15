#pragma once

#include <groov/boost_extra.hpp>
#include <groov/config.hpp>
#include <groov/make_spec.hpp>
#include <groov/read_spec.hpp>
#include <groov/resolve.hpp>

#include <stdx/tuple.hpp>
#include <stdx/type_traits.hpp>

#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/list.hpp>

namespace groov {
namespace detail {
struct no_extract_type {};

template <typename M1, typename M2>
using mask_overlap =
    std::integral_constant<std::remove_cvref_t<decltype(M1::value)>,
                           M1::value & M2::value>;
template <typename M> using nonzero_mask = std::bool_constant<M::value != 0>;

// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
template <typename R, typename F> struct field_proxy {
    using type_t = typename F::type_t;

    constexpr explicit field_proxy(R &reg) : r{reg} {}
    constexpr field_proxy(field_proxy &&) = delete;

    // NOLINTNEXTLINE(google-explicit-constructor)
    constexpr operator type_t() const { return F::extract(r.value); }

    // NOLINTNEXTLINE(misc-unconventional-assign-operator)
    constexpr auto operator=(type_t v) const && -> void {
        F::insert(r.value, v);
    }

    constexpr auto operator+=(type_t v) const && -> void {
        F::insert(r.value, static_cast<type_t>(*this) + v);
    }
    constexpr auto operator-=(type_t v) const && -> void {
        F::insert(r.value, static_cast<type_t>(*this) - v);
    }
    constexpr auto operator*=(type_t v) const && -> void {
        F::insert(r.value, static_cast<type_t>(*this) * v);
    }
    constexpr auto operator/=(type_t v) const && -> void {
        F::insert(r.value, static_cast<type_t>(*this) / v);
    }
    constexpr auto operator%=(type_t v) const && -> void {
        F::insert(r.value, static_cast<type_t>(*this) % v);
    }
    constexpr auto operator|=(type_t v) const && -> void {
        F::insert(r.value, static_cast<type_t>(*this) | v);
    }
    constexpr auto operator&=(type_t v) const && -> void {
        F::insert(r.value, static_cast<type_t>(*this) & v);
    }
    constexpr auto operator^=(type_t v) const && -> void {
        F::insert(r.value, static_cast<type_t>(*this) ^ v);
    }

    constexpr auto operator++() const && -> void {
        auto v = static_cast<type_t>(*this);
        F::insert(r.value, ++v);
    }
    constexpr auto operator--() const && -> void {
        auto v = static_cast<type_t>(*this);
        F::insert(r.value, --v);
    }
    constexpr auto operator++(int) const && -> type_t {
        auto v = static_cast<type_t>(*this);
        auto ret = v;
        F::insert(r.value, ++v);
        return ret;
    }
    constexpr auto operator--(int) const && -> type_t {
        auto v = static_cast<type_t>(*this);
        auto ret = v;
        F::insert(r.value, --v);
        return ret;
    }

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
    R &r;
};

template <stdx::has_trait<std::is_enum> E>
constexpr static auto enable_value() {
    static_assert(stdx::always_false_v<E>,
                  "Enum doesn't contain an ENABLE value");
}

template <stdx::has_trait<std::is_enum> E>
    requires requires { E::ENABLE; }
constexpr static auto enable_value() {
    return E::ENABLE;
}

template <stdx::has_trait<std::is_enum> E>
constexpr static auto disable_value() {
    static_assert(stdx::always_false_v<E>,
                  "Enum doesn't contain an DISABLE value");
}

template <stdx::has_trait<std::is_enum> E>
    requires requires { E::DISABLE; }
constexpr static auto disable_value() {
    return E::DISABLE;
}

template <typename T, typename V> constexpr auto convert_value(V const &v) {
    if constexpr (std::is_same_v<V, enable_t>) {
        static_assert(std::is_enum_v<T>,
                      "enable can only be used with enumeration fields that "
                      "contain an ENABLE value");
        return enable_value<T>();
    } else if constexpr (std::is_same_v<V, disable_t>) {
        static_assert(std::is_enum_v<T>,
                      "disable can only be used with enumeration fields that "
                      "contain a DISABLE value");
        return disable_value<T>();
    } else {
        return static_cast<T>(v);
    }
}
} // namespace detail

template <typename Group, typename Paths, typename Value>
struct write_spec : Group {
    using paths_t = Paths;
    using value_t = Value;
    [[no_unique_address]] value_t value;

  private:
    using extract_type = stdx::conditional_t<
        boost::mp11::mp_size<paths_t>::value == 1,
        typename resolve_t<boost::mp11::mp_front<value_t>,
                           boost::mp11::mp_front<paths_t>>::type_t,
        detail::no_extract_type>;

    template <pathlike P> constexpr static auto find_index() -> std::size_t {
        using actual_field_masks_t =
            boost::mp11::mp_transform_q<detail::field_mask_for_reg_q<paths_t>,
                                        value_t>;
        using lookup_field_masks_t = boost::mp11::mp_transform_q<
            detail::field_mask_for_reg_q<boost::mp11::mp_list<P>>, value_t>;
        using masks_t = boost::mp11::mp_transform<
            detail::mask_overlap, actual_field_masks_t, lookup_field_masks_t>;
        using matches = boost::mp11::mp_copy_if<masks_t, detail::nonzero_mask>;
        if constexpr (boost::mp11::mp_empty<matches>::value) {
            static_assert(stdx::always_false_v<P>,
                          "Invalid path passed to write_spec[]");
        } else if constexpr (boost::mp11::mp_size<matches>::value > 1) {
            static_assert(stdx::always_false_v<P>,
                          "Ambiguous path passed to write_spec[]");
        } else {
            using index_t =
                boost::mp11::mp_find_if<masks_t, detail::nonzero_mask>;
            return index_t::value;
        }
        return {};
    }

  public:
    template <pathlike P> constexpr auto operator[](P const &) {
        constexpr auto idx = find_index<P>();
        auto &r = stdx::get<idx>(value);
        using R = decltype(r);
        return detail::field_proxy<R, resolve_t<R, P>>{r};
    }

    template <pathlike P> constexpr auto operator[](P const &) const {
        constexpr auto idx = find_index<P>();
        auto &r = stdx::get<idx>(value);
        using R = decltype(r);
        using F = resolve_t<R, P>;
        return F::extract(r.value);
    }

    template <std::size_t N>
    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    constexpr auto operator[](char const (&)[N]) const {
        static_assert(stdx::always_false_v<write_spec>,
                      "Trying to index into a write_spec with a string "
                      "literal: did you forget to use the UDL?");
    }

    // NOLINTNEXTLINE(google-explicit-constructor)
    constexpr operator extract_type() const
        requires(not std::is_same_v<extract_type, detail::no_extract_type>)
    {
        using P = boost::mp11::mp_first<paths_t>;
        using R = boost::mp11::mp_first<value_t>;
        auto const &r = stdx::get<R>(value);
        using F = resolve_t<R, P>;
        return F::extract(r.value);
    }
};

template <typename Group, typename Paths, valued_pathlike... Ps>
constexpr auto to_write_spec(read_spec<Group, Paths>, Ps const &...ps) {
    using paths_by_register =
        boost::mpx::mp_gather_q<detail::register_for_path_q<Group>, Paths>;
    using registers =
        boost::mp11::mp_transform_q<detail::register_for_paths_q<Group>,
                                    paths_by_register>;
    using register_values_t =
        boost::mp11::mp_transform<reg_with_value, registers>;
    using register_data_t =
        boost::mp11::mp_apply<stdx::tuple, register_values_t>;

    auto w = write_spec<Group, Paths, register_data_t>{};
    [[maybe_unused]] constexpr auto insert =
        []<valued_pathlike P>(P const &p, [[maybe_unused]] auto &values) {
            using matches =
                boost::mp11::mp_copy_if_q<register_values_t, resolves_q<P>>;
            using R = boost::mp11::mp_first<matches>;
            auto &dest_reg = stdx::get<R>(values);
            using F = resolve_t<R, typename P::path_t>;

            F::insert(dest_reg.value,
                      detail::convert_value<typename F::type_t>(p.value));
        };
    (insert(ps, w.value), ...);
    return w;
}

template <typename G, valued_pathlike... Ps>
constexpr auto tag_invoke(make_spec_t, G, Ps const &...ps) {
    using L = boost::mp11::mp_list<typename Ps::path_t...>;
    detail::check_valid_config<G, L>();
    return to_write_spec(read_spec<G, L>{}, ps...);
}
} // namespace groov
