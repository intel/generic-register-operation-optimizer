#include <concepts>

#include <stdx/tuple.hpp>
#include <tuple>

#include <groov/detail/type_traits.hpp>

namespace groov {
    template <typename T>
    concept tag = not std::integral<T> and not std::floating_point<T>;

    template <typename T>
    concept register_tag = tag<T>;

    template<register_tag R>
    struct register_id_t;

    template <typename T>
    concept register_id = detail::is_instance_of_template_v<register_id_t, T>;

    template <typename T>
    concept field_tag = tag<T>;

    template<field_tag F>
    struct field_id_t;

    template <typename T>
    concept field_id = detail::is_instance_of_template_v<field_id_t, T>;

    template<register_id R, field_id F>
    struct field_path_t;

    template <typename T>
    concept field_path = detail::is_instance_of_template_v<field_path_t, T>;

    template<typename I, typename T>
    struct with_value_t;

    template <typename T>
    concept field_with_value = detail::is_instance_of_template_v<with_value_t, T> and field_id<typename T::id_t>;
    
    template <typename T>
    concept field_maybe_with_value = field_id<T> or field_with_value<T>;

    template<register_tag R>
    struct register_id_t {
        template <field_maybe_with_value F, field_maybe_with_value... Fs>
        constexpr auto operator()(F f, Fs... fs) {
            // FIXME: use stdx::tuple
            return std::make_tuple(*this / f, (*this / fs)...);
        }
    };
    
    template<field_tag F>
    struct field_id_t {
        template <typename T>
        constexpr auto operator=(T value) -> with_value_t<field_id_t, T> {
            return {*this, value};
        }
    };

    template<register_id R, field_id F>
    struct field_path_t {
        template <typename T>
        constexpr auto operator=(T value) -> with_value_t<field_path_t, T> {
            return {*this, value};
        }
    };

    template<typename I, typename T>
    struct with_value_t {
        using id_t = I;
        using value_t = T;

        [[no_unique_address]] id_t id;
        value_t value;
    };

    template <register_id R, field_id F>
    constexpr auto operator/(R, F) -> field_path_t<R, F> {
        return {};
    }

    template <register_id R, field_id F, typename T>
    constexpr auto operator/(R, with_value_t<F, T> w) -> with_value_t<field_path_t<R, F>, T> {
        return {field_path_t<R, F>{}, w.value};
    }



    namespace literals {
        template <class T, T... chars>
        struct id_string {};

        template <class T, T... chars>
        constexpr auto operator""_r() -> register_id_t<id_string<T, chars...>> {
            return {};
        }

        template <class T, T... chars>
        constexpr auto operator""_f() -> field_id_t<id_string<T, chars...>> {
            return {};
        }
    }
}