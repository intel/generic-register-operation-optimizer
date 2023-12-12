#pragma once

#include <async/concepts.hpp>
#include <async/then.hpp>
#include <async/when_all.hpp>

#include <groov/boost_extra.hpp>
#include <groov/object.hpp>
#include <groov/path.hpp>

#include <stdx/ct_string.hpp>
#include <stdx/tuple.hpp>
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

template <typename Group> struct register_for_path_q {
    template <pathlike P>
    using fn = typename Group::template child_t<root(P{})>;
};

template <typename Group> struct register_for_paths_q {
    template <typename L>
    using fn = typename register_for_path_q<Group>::template fn<
        boost::mp11::mp_front<L>>;
};

template <typename Group> struct field_mask_for_paths_q {
    template <typename L>
    using mask_t = typename Group::template child_t<root(
        boost::mp11::mp_front<L>{})>::type_t;

    template <typename L> constexpr static auto compute_mask() {
        mask_t<L> mask{};
        stdx::template_for_each<L>([&]<typename P>() {
            using object_t = resolve_t<Group, P>;
            mask |= object_t::mask;
        });
        return mask;
    }

    template <typename L>
    using fn = std::integral_constant<mask_t<L>, compute_mask<L>()>;
};

template <pathlike Path> struct path_match_q {
    template <pathlike P>
    using whole_register = std::bool_constant<
        boost::mp11::mp_size<P>::value == 1 and
        std::is_same_v<boost::mp11::mp_front<Path>, boost::mp11::mp_front<P>>>;

    template <pathlike P>
    using admissible = boost::mp11::mp_or<whole_register<P>,
                                          boost::mpx::mp_ends_with<P, Path>>;

    template <typename L> using fn = boost::mp11::mp_any_of<L, admissible>;
};

template <typename RegPaths, typename... Rs> class read_result {
    using type = stdx::tuple<Rs...>;

    struct no_extract_type {};
    using extract_type = stdx::conditional_t<
        sizeof...(Rs) == 1 and
            boost::mp11::mp_size<boost::mp11::mp_front<RegPaths>>::value == 1,
        typename resolve_t<
            boost::mp11::mp_front<type>,
            boost::mp11::mp_front<boost::mp11::mp_front<RegPaths>>>::type_t,
        no_extract_type>;

    type value{};

  public:
    template <typename... Vs>
    constexpr explicit read_result(Vs... vs) : value{Rs{{}, vs}...} {}

    template <pathlike Path> constexpr auto operator[](Path) const {
        using matches = boost::mp11::mp_copy_if_q<RegPaths, path_match_q<Path>>;
        static_assert(not boost::mp11::mp_empty<matches>::value,
                      "Invalid path passed to read_result lookup");
        static_assert(boost::mp11::mp_size<matches>::value == 1,
                      "Ambiguous path passed to read_result lookup");
        using index =
            boost::mp11::mp_find<RegPaths, boost::mp11::mp_front<matches>>;

        auto const &r = stdx::get<index::value>(value);
        using object_t = resolve_t<decltype(r), Path>;
        return object_t::extract(r.value);
    }

    // NOLINTNEXTLINE(google-explicit-constructor)
    constexpr operator extract_type() const
        requires(not std::is_same_v<extract_type, no_extract_type>)
    {
        auto const &r = stdx::get<0>(value);
        using object_t =
            resolve_t<decltype(r),
                      boost::mp11::mp_front<boost::mp11::mp_front<RegPaths>>>;
        return object_t::extract(r.value);
    }
};
} // namespace detail

template <typename T> auto read(T) {
    using paths_by_register =
        boost::mpx::mp_gather_q<detail::register_for_path_q<T>,
                                typename T::paths_t>;
    using registers =
        boost::mp11::mp_transform_q<detail::register_for_paths_q<T>,
                                    paths_by_register>;
    using register_values =
        boost::mp11::mp_transform<reg_with_value, registers>;
    using field_masks =
        boost::mp11::mp_transform_q<detail::field_mask_for_paths_q<T>,
                                    paths_by_register>;

    return []<typename... Rs, typename... Ms>(boost::mp11::mp_list<Rs...>,
                                              boost::mp11::mp_list<Ms...>) {
        return async::when_all(detail::read<Rs, T, Ms>()...) |
               async::then([](typename Rs::type_t... values) {
                   return detail::read_result<paths_by_register, Rs...>{
                       values...};
               });
    }(register_values{}, field_masks{});
}
} // namespace groov
