#pragma once

#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/list.hpp>

namespace boost::mp11 {
namespace detail {
template <typename L1, typename L2> struct mp_ends_with_t {
    constexpr static auto drop_len =
        boost::mp11::mp_size<L1>::value - boost::mp11::mp_size<L2>::value;
    using type = std::is_same<L2, boost::mp11::mp_drop_c<L1, drop_len>>;
};

template <typename L1, typename L2>
    requires(boost::mp11::mp_size<L1>::value < boost::mp11::mp_size<L2>::value)
struct mp_ends_with_t<L1, L2> {
    using type = std::false_type;
};
} // namespace detail
template <typename L1, typename L2>
using mp_ends_with = typename detail::mp_ends_with_t<L1, L2>::type;

namespace detail {
template <template <typename> typename, typename> struct mp_gather_t;
}

template <template <typename> typename F, typename L>
using mp_gather = typename detail::mp_gather_t<F, L>::type;

namespace detail {
template <template <typename> typename F, template <typename...> typename List>
struct mp_gather_t<F, List<>> {
    using type = List<>;
};

template <template <typename> typename F, template <typename...> typename List,
          typename... Ts>
struct mp_gather_t<F, List<Ts...>> {
    using value_t = F<boost::mp11::mp_front<List<Ts...>>>;
    template <typename T> using match = std::is_same<F<T>, value_t>;
    using P = boost::mp11::mp_partition<List<Ts...>, match>;

    using type =
        boost::mp11::mp_push_front<mp_gather<F, boost::mp11::mp_second<P>>,
                                   boost::mp11::mp_first<P>>;
};
} // namespace detail
} // namespace boost::mp11
