#ifndef HFSM_STATE_MACHINE_MPL_H_
#define HFSM_STATE_MACHINE_MPL_H_

#include <tuple>
#include <type_traits>

namespace hfsm {
namespace mpl {

template<typename...>
using mp_void = void;

template<bool B>
using mp_bool = std::integral_constant<bool, B>;

template<typename... T>
struct mp_list {};

// mp_size_impl
namespace detail {

template<typename L>
struct mp_size_impl;

template<template<typename...> class L, typename... T>
struct mp_size_impl<L<T...>> {
    using type = std::integral_constant<std::size_t, sizeof...(T)>;
};

} // namespace detail

template<typename L>
using mp_size = typename detail::mp_size_impl<L>::type;

// mp_transform_impl
namespace detail {

template<template<typename...> class F, typename L>
struct mp_transform_impl;

template<template<typename...> class F, template<typename...> class L, typename... T>
struct mp_transform_impl<F, L<T...>> {
    using type = L<F<T>...>;
};

} // namespace detail

template<template<typename...> class F, typename L>
using mp_transform = typename detail::mp_transform_impl<F, L>::type;

// mp_apply_impl
namespace detail {

template<template<typename...> class F, typename L>
struct mp_apply_impl;

template<template<typename...> class F, template<typename...> class L, typename... T>
struct mp_apply_impl<F, L<T...>> {
    using type = F<T...>;
};

} // namespace detail

template<template<typename...> class F, typename L>
using mp_apply = typename detail::mp_apply_impl<F, L>::type;

namespace detail {

template<typename... L>
struct mp_append_impl;

template<template<typename...> class L, typename... T>
struct mp_append_impl<L<T...>> {
    using type = L<T...>;
};

template<template<typename...> class L1, typename... T1,
         template<typename...> class L2, typename... T2,
         typename... L>
struct mp_append_impl<L1<T1...>, L2<T2...>, L...> {
    using type = typename mp_append_impl<L1<T1..., T2...>, L...>::type;
};

} // namespace detail

template<typename... L>
using mp_append = typename detail::mp_append_impl<L...>::type;

// mp_find_impl
namespace detail {

template<typename L, typename T>
struct mp_find_impl;

template<template<typename...> class L, typename T>
struct mp_find_impl<L<>, T> {
    using type = std::integral_constant<std::size_t, 0>;
};

constexpr std::size_t cx_find_index(bool const* first, bool const* last) {
    std::size_t m = 0;
    while (first != last && !*first) {
        ++m;
        ++first;
    }
    return m;
}

template<template<typename...> class L, typename... U, typename T>
struct mp_find_impl<L<U...>, T> {
    static constexpr bool _v[] = {std::is_same<T, U>::value...};
    using type = std::integral_constant<std::size_t, cx_find_index(_v, _v + sizeof...(U))>;
};

} // namespace detail

template<typename L, typename T>
using mp_find = typename detail::mp_find_impl<L, T>::type;

// mp_find_if_impl
namespace detail {

template<typename L, template<typename...> class P>
struct mp_find_if_impl;

template<template<typename...> class L, template<typename...> class P>
struct mp_find_if_impl<L<>, P> {
    using type = std::integral_constant<std::size_t, 0>;
};

template<template<typename...> class L, typename... T, template<typename...> class P>
struct mp_find_if_impl<L<T...>, P> {
    static constexpr bool _v[] = {P<T>::value...};
    using type = std::integral_constant<std::size_t, cx_find_index(_v, _v + sizeof...(T))>;
};

} // namespace detail

template<typename L, template<typename...> class P>
using mp_find_if = typename detail::mp_find_if_impl<L, P>::type;

// mp_set_contains_impl
namespace detail {

template<typename L, typename T>
struct mp_set_contains_impl;

template<template<typename...> class L, typename T>
struct mp_set_contains_impl<L<>, T> : std::false_type {};

template<template<typename...> class L, typename U1, typename... U, typename T>
struct mp_set_contains_impl<L<U1, U...>, T>
    : std::conditional_t<std::is_same<U1, T>::value, std::true_type, mp_set_contains_impl<L<U...>, T>> {};

} // namespace detail

template<typename L, typename T>
using mp_set_contains = typename detail::mp_set_contains_impl<L, T>::type;

// mp_set_push_back_impl
namespace detail {

template<typename L, typename... T>
struct mp_set_push_back_impl;

template<template<typename...> class L, typename... U>
struct mp_set_push_back_impl<L<U...>> {
    using type = L<U...>;
};

template<template<typename...> class L, typename... U, typename T1, typename... T>
struct mp_set_push_back_impl<L<U...>, T1, T...> {
    using S = std::conditional_t<mp_set_contains<L<U...>, T1>::value, L<U...>, L<U..., T1>>;
    using type = typename mp_set_push_back_impl<S, T...>::type;
};

} // namespace detail

template<typename L, typename... T>
using mp_set_push_back = typename detail::mp_set_push_back_impl<L, T...>::type;

// mp_set_union_impl
namespace detail {

template<typename... L>
struct mp_set_union_impl;

template<template<typename...> class L, typename... T>
struct mp_set_union_impl<L<T...>> {
    using type = L<T...>;
};

template<template<typename...> class L1, typename... U1,
         template<typename...> class L2, typename... U2,
         typename... L>
struct mp_set_union_impl<L1<U1...>, L2<U2...>, L...> {
    using S = mp_set_push_back<L1<U1...>, U2...>;
    using type = typename mp_set_union_impl<S, L...>::type;
};

} // namespace detail

template<typename... L>
using mp_set_union = typename detail::mp_set_union_impl<L...>::type;

// mp_unique_impl
namespace detail {

template<typename L>
struct mp_unique_impl;

template<template<typename...> class L, typename... T>
struct mp_unique_impl<L<T...>> {
    using type = mp_set_push_back<L<>, T...>;
};

} // namespace detail

template<typename L>
using mp_unique = typename detail::mp_unique_impl<L>::type;

template<std::size_t I = 0, typename Tuple, typename Pred, typename Func>
auto tuple_visit_if(Tuple&, Pred&&, Func&&) -> std::enable_if_t<I == std::tuple_size<Tuple>::value, bool> {
    return false;
}

template<std::size_t I = 0, typename Tuple, typename Pred, typename Func>
auto tuple_visit_if(Tuple& tuple, Pred&& pred, Func&& func)
    -> std::enable_if_t < I<std::tuple_size<Tuple>::value, bool> {
    auto& value = std::get<I>(tuple);

    if (pred(value)) {
        func(value);
        return true;
    }

    return tuple_visit_if<I + 1>(tuple, std::forward<Pred>(pred), std::forward<Func>(func));
}

} // namespace mpl
} // namespace hfsm

#endif /* HFSM_STATE_MACHINE_MPL_H_ */
