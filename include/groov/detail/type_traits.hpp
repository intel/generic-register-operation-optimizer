#include <concepts>
#include <type_traits>

namespace groov {
namespace detail {
template <template <typename...> class Template, typename T>
struct is_instance_of_template : std::false_type {};

template <template <typename...> class Template, typename... Args>
struct is_instance_of_template<Template, Template<Args...>> : std::true_type {};

template <template <typename...> class Template, typename T>
constexpr bool is_instance_of_template_v =
    is_instance_of_template<Template, T>::value;
} // namespace detail
} // namespace groov