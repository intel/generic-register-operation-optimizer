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
template <typename Object> constexpr auto extract(auto value) {
    return Object::extract(value);
}

template <typename Object, stdx::ct_string Child, stdx::ct_string... Fields>
constexpr auto extract(auto value) {
    using child_t = get_child<Child, Object>;
    return extract<child_t, Fields...>(value);
}

template <typename Object, stdx::ct_string... Parts>
constexpr auto extract(path<Parts...>, auto value) {
    return extract<Object, Parts...>(value);
}

namespace detail {
template <typename T> using get_path = typename T::path_t;

template <typename T> using register_of = boost::mp11::mp_front<get_path<T>>;
template <typename T>
using register_type_of = typename get_child<register_of<T>::value, T>::type;

template <typename Path> struct path_match {
    template <typename G>
    using register_match =
        boost::mp11::mp_bool<boost::mp11::mp_size<get_path<G>>::value == 1 and
                             std::same_as<boost::mp11::mp_front<Path>,
                                          boost::mp11::mp_front<get_path<G>>>>;

    template <typename G>
    using trailing_match = boost::mp11::mp_bool<
        (boost::mp11::mp_size<get_path<G>>::value > 1) and
        boost::mp11::mp_ends_with<get_path<G>, Path>::value>;

    template <typename G>
    using fn = boost::mp11::mp_or<trailing_match<G>, register_match<G>>;
};

template <typename...> struct read_result;

template <typename G> struct read_result<G> {
  private:
    using type = register_type_of<G>;
    using fullpath_t = get_path<G>;

  public:
    template <typename Path>
        requires(path_match<Path>::template fn<G>::value)
    constexpr auto operator[]([[maybe_unused]] Path p) const {
        if constexpr (path_match<Path>::template register_match<G>::value) {
            return extract<G>(p, value);
        } else {
            return extract<G>(fullpath_t{}, value);
        }
    }

    template <typename Path> constexpr auto operator[](Path) const {
        static_assert(stdx::always_false_v<Path>,
                      "Invalid path in read result lookup");
    }

    type value{};

  private:
    using extract_type = decltype(extract<G>(fullpath_t{}, value));

  public:
    // NOLINTNEXTLINE(google-explicit-constructor)
    constexpr operator extract_type() const {
        return extract<G>(fullpath_t{}, value);
    }
};

template <typename... Gs>
    requires(sizeof...(Gs) > 1)
struct read_result<Gs...> {
  private:
    using groups = boost::mp11::mp_list<Gs...>;
    using G = boost::mp11::mp_front<groups>;
    using type = register_type_of<G>;

  public:
    template <typename Path>
    constexpr auto operator[]([[maybe_unused]] Path p) const {
        using L = boost::mp11::mp_copy_if_q<groups, path_match<Path>>;
        static_assert(not boost::mp11::mp_empty<L>::value,
                      "Invalid path in read result lookup");
        using group_t = boost::mp11::mp_front<L>;
        return read_result<group_t>{value}[p];
    }

    type value{};
};

template <typename...> struct multi_read_result;

template <template <typename...> typename List, typename... Lists>
struct multi_read_result<List<Lists...>> {
  private:
    template <typename L>
    using to_read_result = boost::mp11::mp_apply<read_result, L>;
    using results_tuple_t = stdx::tuple<to_read_result<Lists>...>;

    template <typename Path> struct any_path_match {
        template <typename G>
        using match = typename path_match<Path>::template fn<G>;
        template <typename L> using fn = boost::mp11::mp_any_of<L, match>;
    };

  public:
    template <typename Path> constexpr auto operator[](Path p) const {
        using L =
            boost::mp11::mp_copy_if_q<List<Lists...>, any_path_match<Path>>;
        static_assert(not boost::mp11::mp_empty<L>::value,
                      "Invalid path in read result lookup");
        static_assert(boost::mp11::mp_size<L>::value == 1,
                      "Ambiguous path in read result lookup");
        using result_t = to_read_result<boost::mp11::mp_front<L>>;
        return get<result_t>(results)[p];
    }

    results_tuple_t results{};
};

template <typename List> auto read(List) -> async::sender auto {
    using T = boost::mp11::mp_front<List>;
    constexpr auto reg_name = register_of<T>::value;
    using register_t = get_child<reg_name, T>;
    using bus_t = typename T::bus_t;
    return bus_t::read(register_t::address);
}

template <typename List>
using register_type_for_read = register_type_of<boost::mp11::mp_front<List>>;
} // namespace detail

template <typename... Ts>
    requires(sizeof...(Ts) > 0)
auto read(Ts...) -> async::sender auto {
    using chunks_by_register =
        boost::mp11::mp_gather<detail::register_of,
                               boost::mp11::mp_list<Ts...>>;

    if constexpr (boost::mp11::mp_size<chunks_by_register>::value == 1u) {
        using list_t = boost::mp11::mp_front<chunks_by_register>;
        return detail::read(list_t{}) | async::then([](auto value) {
                   return [&]<typename... Ps>(boost::mp11::mp_list<Ps...>) {
                       return detail::read_result<Ps...>{value};
                   }(list_t{});
               });
    } else {
        return []<typename... Lists>(boost::mp11::mp_list<Lists...>) {
            return async::when_all(detail::read(Lists{})...) |
                   async::then(
                       [](detail::register_type_for_read<Lists>... values) {
                           return detail::multi_read_result<
                               boost::mp11::mp_list<Lists...>>{values...};
                       });
        }(chunks_by_register{});
    }
}
} // namespace groov
