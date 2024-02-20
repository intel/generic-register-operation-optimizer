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

  public:
    template <pathlike P> constexpr auto operator[](P const &) const {
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
            auto const &r = stdx::get<index_t::value>(value);
            using F = resolve_t<decltype(r), P>;
            return F::extract(r.value);
        }
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
            F::insert(dest_reg.value, static_cast<typename F::type_t>(p.value));
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
