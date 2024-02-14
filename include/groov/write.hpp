#pragma once

#include <groov/read.hpp>

#include <async/just.hpp>

template <typename...> struct undef;

namespace groov {
namespace detail {
template <typename Register, typename ValueTree, typename Mask, typename T>
auto write(T value) -> async::sender auto {
    using bus_t = typename ValueTree::bus_t;
    return bus_t::template write<Mask::value>(Register::address, value);
}
}

/*
undef<
    groov::tree<
        {{"tree"}}, 
        (anonymous namespace)::bus, 
        boost::mp11::mp_list<
            groov::value_path<
                groov::path<{{"reg0"}}>, 
                unsigned int
            >
        >, 
        groov::reg<{{"reg0"}}, unsigned int, &(anonymous namespace)::data0, groov::field<{{"field0"}}, unsigned char, 0, 0>, groov::field<{{"field1"}}, unsigned char, 4, 1>, groov::field<{{"field2"}}, unsigned char, 7, 5>>, 
        groov::reg<{{"reg1"}}, unsigned int, &(anonymous namespace)::data1, groov::field<{{"field0"}}, unsigned char, 0, 0>, groov::field<{{"field1"}}, unsigned char, 4, 1>, groov::field<{{"field2"}}, unsigned char, 7, 5>>
    >
>
*/

template <typename T> auto write(T) {
    using paths_by_register =
        boost::mpx::mp_gather_q<detail::register_for_path_q<T>,
                                typename T::paths_t>;
    using registers =
        boost::mp11::mp_transform_q<detail::register_for_paths_q<T>,
                                    paths_by_register>;
    undef<registers> q{};

    using register_values =
        boost::mp11::mp_transform<reg_with_value, registers>;
    using field_masks =
        boost::mp11::mp_transform_q<detail::field_mask_for_paths_q<T>,
                                    paths_by_register>;    
    return []<typename... Rs, typename... Ms>(boost::mp11::mp_list<Rs...>,
                                              boost::mp11::mp_list<Ms...>) {
        return async::when_all(detail::write<Rs, T, Ms>()...);
    }(register_values{}, field_masks{});
    
        //undef<T> q{};
    return async::just(0);
}   
}

/*

#include <async/concepts.hpp>
#include <async/let_value.hpp>

#include <groov/tree.hpp>
#include <groov/path.hpp>

#include <stdx/type_traits.hpp>

namespace groov {
namespace detail {
template <typename Group> struct writer {
    template <typename T>
    auto operator()(T const &t) const -> async::sender auto {
        using Reg = typename Group::template register_for<T>;
        using data_t = typename Reg::type_t;
        using bus_t = typename Group::bus_t;

        auto const insert = [&]<typename Obj, typename V>(V const &v,
                                                          auto &data) {
            using F = resolve_t<Obj, V>;
            F::insert(data, v.value);
            return F::mask;
        };
        auto const compute_mask = [&]<typename Obj, typename V>() {
            using F = resolve_t<Obj, V>;
            return F::mask;
        };

        if constexpr (stdx::is_specialization_of_v<
                          std::remove_cvref_t<decltype(t.value)>,
                          stdx::tuple>) {
            using Obj = resolve_t<Group, T>;
            return t.value.apply([&]<typename... Vs>(Vs const &...vs) {
                constexpr auto mask =
                    (data_t{} | ... |
                     compute_mask.template operator()<Obj, Vs>());
                data_t data{};
                (insert.template operator()<Obj>(vs, data), ...);
                return bus_t::template write<mask>(Reg::address, data);
            });
        } else {
            constexpr auto mask = compute_mask.template operator()<Group, T>();
            data_t data{};
            insert.template operator()<Group>(t, data);
            return bus_t::template write<mask>(Reg::address, data);
        }
    }

    auto operator()() const {
        return async::let_value([this](auto v) { return this->operator()(v); });
    }
};
} // namespace detail

template <typename Group> constexpr auto write = detail::writer<Group>{};
} // namespace groov

*/
