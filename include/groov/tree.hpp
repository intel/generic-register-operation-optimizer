#pragma once

#include <groov/concepts.hpp>
#include <groov/path.hpp>
#include <groov/boost_extra.hpp>

#include <stdx/ct_string.hpp>
#include <stdx/type_traits.hpp>

#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/list.hpp>

#include <cstddef>
#include <iterator>
#include <limits>
#include <type_traits>

template <typename...> struct undef;

namespace groov {
template <stdx::ct_string Name, named... Ts> struct named_container {
  private:
    template <stdx::ct_string N> struct has_name_q {
        template <named T>
        using fn = std::is_same<detail::name_of<T>, detail::name_t<N>>;
    };

  public:
    constexpr static auto name = Name;
    using name_t = detail::name_t<Name>;
    using children_t = boost::mp11::mp_list<Ts...>;

    template <stdx::ct_string S>
    using child_t = boost::mp11::mp_front<
        boost::mp11::mp_copy_if_q<children_t, has_name_q<S>>>;
};

namespace detail {
template <typename T, pathlike P> constexpr auto recursive_resolve(P);

template <typename L, pathlike P>
constexpr auto resolve_matches([[maybe_unused]] P p) {
    if constexpr (boost::mp11::mp_empty<L>::value) {
        return invalid_t{};
    } else if constexpr (boost::mp11::mp_size<L>::value > 1) {
        return ambiguous_t{};
    } else {
        return recursive_resolve<boost::mp11::mp_front<L>>(p);
    }
}

template <typename T, pathlike P> constexpr auto recursive_resolve(P p) {
    constexpr auto r = root(p);
    using children_t = typename T::children_t;
    if constexpr (r == T::name) {
        auto const leftover_path = without_root(p);
        if constexpr (std::empty(leftover_path)) {
            return T{};
        } else {
            using matches =
                boost::mp11::mp_copy_if_q<children_t,
                                          resolves_q<decltype(leftover_path)>>;
            return resolve_matches<matches>(leftover_path);
        }
    } else {
        using matches = boost::mp11::mp_copy_if_q<children_t, resolves_q<P>>;
        return resolve_matches<matches>(p);
    }
}
} // namespace detail

template <stdx::ct_string Name, typename T, std::size_t Msb, std::size_t Lsb,
          fieldlike... SubFields>
struct field : named_container<Name, SubFields...> {
    using type_t = T;
    constexpr static auto mask = ((1u << (Msb - Lsb + 1u)) - 1u) << Lsb;

    constexpr static auto extract(std::unsigned_integral auto value) {
        return static_cast<type_t>((value & mask) >> Lsb);
    }

    constexpr static void insert(type_t& dest, std::unsigned_integral auto value) {
        dest = (dest & ~mask) | ((value << Lsb) & mask);
    }

    template <pathlike P> constexpr static auto resolve(P p) {
        return detail::recursive_resolve<field>(p);
    }
};

template <stdx::ct_string Name, std::unsigned_integral T, auto Address,
          fieldlike... Fields>
struct reg : named_container<Name, Fields...> {
    using type_t = T;
    constexpr static auto mask = std::numeric_limits<type_t>::max();

    using address_t = decltype(Address);
    constexpr static auto address = Address;

    constexpr static auto extract(std::unsigned_integral auto value) {
        return value;
    }

    constexpr static auto insert(type_t & dest, std::unsigned_integral auto value) -> void {
        dest = value;
    }

    
    template <pathlike P> constexpr static auto resolve(P p) {
        return detail::recursive_resolve<reg>(p);
    }
};

template <typename Reg> struct reg_with_value : Reg {
    typename Reg::type_t value;
};

namespace detail {
template <typename L> struct any_resolves_q {
    template <pathlike P> using fn = boost::mp11::mp_any_of_q<L, resolves_q<P>>;
};

template <typename T>
concept valued = requires {
    typename T::value_t;
};

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


template <typename Tree, typename Value>
struct value_tree : Tree {
    using value_t = Value;
    [[no_unique_address]] value_t value;

    constexpr value_tree(value_t new_value) : value{new_value} {}

    template<typename R>
    constexpr static auto mask_for_v = []() -> typename R::value_t {
        // gather each 
    }();
};

template <stdx::ct_string Name, typename...> struct tree;

template <stdx::ct_string Name, typename Bus, registerlike... Registers>
    requires(... and bus_for<Bus, Registers>)
struct group : named_container<Name, Registers...> {
    using bus_t = Bus;

    template <pathlike P> constexpr static auto resolve(P p) {
        return detail::recursive_resolve<group>(p);
    }

    template <typename... Ps>
    constexpr auto to_tree() const -> tree<Name, Bus, boost::mp11::mp_list<Ps...>, Registers...> {
        using L = boost::mp11::mp_list<Ps...>;
        static_assert(boost::mp11::mp_is_set<L>::value,
                      "Duplicate path passed to group");
        static_assert(
            boost::mp11::mp_all_of_q<
                L, detail::any_resolves_q<typename group::children_t>>::value,
            "Unresolvable path passed to group");
        stdx::template_for_each<L>([]<typename P>() {
            using rest = boost::mp11::mp_remove<L, P>;
            static_assert(boost::mp11::mp_none_of_q<rest, resolves_q<P>>::value,
                          "Redundant path passed to group");
        });
        return {};
    }

    template <pathlike... Ps>
        requires(sizeof...(Ps) > 0) and (... and not detail::valued<Ps>)
    constexpr auto operator()(Ps const &...) const {
        return to_tree<Ps...>();
    }

    template <pathlike... Ps>
        requires(sizeof...(Ps) > 0) and (... and detail::valued<Ps>)
    constexpr auto operator()(Ps const & ... ps) const {
        using L = 
            boost::mp11::mp_list<Ps...>;
            
        using paths_by_register = 
            boost::mpx::mp_gather_q<detail::register_for_path_q<group>, L>;

        using registers =  
            boost::mp11::mp_transform_q<detail::register_for_paths_q<group>, paths_by_register>;  
              
        using register_values_t =
            boost::mp11::mp_transform<reg_with_value, registers>;

        auto register_values = boost::mp11::mp_apply<stdx::tuple, register_values_t>{};

        constexpr auto insert = [] <detail::valued P> (P const& p, [[maybe_unused]] auto& values) {
            using matches = boost::mp11::mp_copy_if_q<register_values_t, resolves_q<P>>;
            using R = decltype(detail::resolve_matches<matches>(p));
            auto& dest_reg = stdx::get<R>(values);
            using F = resolve_t<R, typename P::path>;
            
            F::insert(dest_reg.value, p.value);
        };
        
        (insert(ps, register_values), ...);
        
        using tree_t = decltype(to_tree<typename Ps::path_t...>());
        
        return value_tree<tree_t, decltype(register_values)> {register_values};
    }
};

template <stdx::ct_string Name, typename C>
using get_child = typename C::template child_t<Name>;

template <stdx::ct_string Name, typename Bus, typename Paths,
          named... Registers>
struct tree<Name, Bus, Paths, Registers...>
    : group<Name, Bus, Registers...> {
    using paths_t = Paths;
};

template <stdx::ct_string Name, typename Bus, typename... Registers,
          stdx::ct_string... Parts>
constexpr auto operator/(group<Name, Bus, Registers...> g, path<Parts...> p) {
    return g(p);
}
} // namespace groov
