#pragma once

#include <groov/config.hpp>
#include <groov/make_spec.hpp>
#include <groov/resolve.hpp>

#include <boost/mp11/list.hpp>

namespace groov {
template <typename Group, typename Paths> struct read_spec : Group {
    using paths_t = Paths;
};

template <typename G, pathlike... Ps>
    requires(... and not valued<Ps>)
constexpr auto tag_invoke(make_spec_t, G, Ps...) {
    using L = boost::mp11::mp_list<Ps...>;
    detail::check_valid_config<G, L>();
    return read_spec<G, L>{};
}
} // namespace groov
