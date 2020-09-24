#pragma once
#include <type_traits>

namespace vera {

/**
 * pack
 */
template<class... TS> struct pack {
  constexpr static auto size = sizeof...(TS);
};

/**
 * unpack
 * 
 * Definition: unpack<T, pack<A, B, ...>, C, D<E, F>, ...> = T<A, B, C, D<E, F>, ...>
 */
template<class... TS>
struct unpack_helper:
public unpack_helper<pack<>, TS...> {};

template<class... TS>
struct unpack_helper<pack<TS...>> {
  using type = pack<TS...>;
};

template<class U, class... TS, class... Rest>
struct unpack_helper<pack<TS...>, U, Rest...>:
public unpack_helper<pack<TS..., U>, Rest...> {};

template<class... TS, class... Packed, class... Rest>
struct unpack_helper<pack<TS...>, pack<Packed...>, Rest...>:
public unpack_helper<pack<TS..., Packed...>, Rest...> {};

template<class... TS>
using unpack = typename unpack_helper<TS...>::type;

/**
 * append
 * 
 * Definition: append<T<A, B, ...>, C, D, ...> = T<A, B, ..., C, D, ...>,
 */
template<class, class>
struct append_helper;

template<template<class...> class T, class... TS, class... Append>
struct append_helper<T<TS...>, pack<Append...>> {
  using type = T<TS..., Append...>;
};

template<class T, class... TS>
using append = typename append_helper<T, unpack<TS...>>::type;

/**
 * prepend
 * 
 * Definition: prepend<T<A, B, ...>, C, D, ...> = T<C, D, ..., A, B, ...>,
 */
template<class, class>
struct prepend_helper;

template<template<class...> class T, class... TS, class... Prepend>
struct prepend_helper<T<TS...>, pack<Prepend...>> {
  using type = T<Prepend..., TS...>;
};

template<class T, class... TS>
using prepend = typename prepend_helper<T, unpack<TS...>>::type;

/**
 * forward
 * 
 * Definition: forward<T, U<TS...>> = T<TS...>
 */
template<template<class...> class, class>
struct forward_helper;

template<template<class...> class T, template<class...> class U, class... TS>
struct forward_helper<T, U<TS...>> {
  using type = T<TS...>;
};

template<template<class...> class T, class U>
using forward = typename forward_helper<T, U>::type;

/**
 * extract
 * 
 * Usage: extract<T<TS...>> = pack<TS...>
 */
template<class T>
struct extract_helper;

template<template<class...> class T, class... TS>
struct extract_helper<T<TS...>> {
  using type = pack<TS...>;
};

template<class T>
using extract = typename extract_helper<T>::type;

/**
 * inject
 * 
 * Definition: inject<T, A, pack<B, C, ...>, D<E, F>, ...> = T<A, B, C, D<E, F>, ...>
 */
template<template<class...> class T, class... TS>
using inject = forward<T, unpack<TS...>>;

/**
 * placeholder
 */
struct placeholder {};

/**
 * bind
 * 
 * Usage: bind<T, A, placeholder, B, ...>::type<C> = T<A, C, B>
 *        bind<T, A>::type<B> = T<A, B>
 */
template<class, class>
struct bind_helper;

template<class T, class... TS, class P>
struct bind_helper<pack<T, TS...>, P> {
  using type = prepend<typename bind_helper<pack<TS...>, P>::type, T>;
};

template<class... TS, class P, class... PS>
struct bind_helper<pack<placeholder, TS...>, pack<P, PS...>> {
  using type = prepend<typename bind_helper<pack<TS...>, pack<PS...>>::type, P>;
};

template<class... PS>
struct bind_helper<pack<>, pack<PS...>> {
  using type = pack<PS...>;
};

template<class... TS>
struct bind_helper<pack<placeholder, TS...>, pack<>>;

template<template<class...> class T, class... TS>
struct bind {
  template<class... PS>
  using type = forward<T, typename bind_helper<unpack<TS...>, unpack<PS...>>::type>;
};

/**
 * negation
 */
template<template<class...> class P>
struct negation {
  template<class... TS>
  struct type {
    constexpr static auto value = !P<TS...>::value;
  };
};

/**
 * conjuction
 */
template<template<class...> class... PS>
struct conjunction {
  template<class... TS>
  struct type {
    constexpr static auto value = (PS<TS...>::value && ...) && true;
  };
};

/**
 * disjunction
 */
template<template<class...> class... PS>
struct disjunction {
  template<class... TS>
  struct type {
    constexpr static auto value = (PS<TS...>::value || ...) || false;
  };
};

/**
 * transform
 */
template<template<class> class F, class>
struct transform_helper {
  using type = pack<>;
};

template<template<class> class F, class... TS>
struct transform_helper<F, pack<TS...>> {
  using type = pack<typename F<TS>::type...>;
};

template<template<class> class F, class... TS>
using transform = typename transform_helper<F, unpack<TS...>>::type;

/**
 * filter
 */
template<template<class> class, class>
struct filter_helper;

template<template<class> class P, class T, class... TS>
struct filter_helper<P, pack<T, TS...>> {
  using type = std::conditional_t<
    P<T>::value,
    prepend<typename filter_helper<P, pack<TS...>>::type, T>,
    typename filter_helper<P, pack<TS...>>::type
  >;
};

template<template<class> class P>
struct filter_helper<P, pack<>> {
  using type = pack<>;
};

template<template<class> class P, class... TS>
using filter = typename filter_helper<P, unpack<TS...>>::type;

/**
 * contains
 */
template<class, class...>
struct contains_helper
: std::false_type {};

template<class T, class U, class... TS>
struct contains_helper<T, U, TS...>
: std::conditional_t<
  std::is_same_v<T, U>,
  std::true_type,
  contains_helper<T, TS...>
> {};

template<class T, class... TS>
constexpr auto contains = inject<contains_helper, T, TS...>::value;

/**
 * distinct
 */
template<class...>
struct distinct_helper {
  using type = pack<>;
};

template<class T, class... TS>
struct distinct_helper<T, TS...> {
  using type = std::conditional_t<
    contains<T, TS...>,
    typename distinct_helper<TS...>::type,
    prepend<typename distinct_helper<TS...>::type, T>
  >;
};

template<class... TS>
using distinct = typename inject<distinct_helper, TS...>::type;

/**
 * callable
 * 
 * Unifies callable signatures to ReturnType(ArgumentTypes...)
 */
template<class T>
struct callable_helper:
public callable_helper<decltype(&T::operator())> {};

template<class T, class... TS>
struct callable_helper<T(TS...)> {
  using type = T(TS...);
};

template<class T, class U, class... TS>
struct callable_helper<T(U::*)(TS...)> {
  using type = T(TS...);
};

template<class T, class U, class... TS>
struct callable_helper<T(U::*)(TS...) const> {
  using type = T(TS...);
};

template<class T>
using callable = typename callable_helper<T>::type;

/**
 * callable_return_type
 */
template<class T>
struct callable_return_type_helper
: public callable_return_type_helper<callable<T>> {
};

template<class T, class... TS>
struct callable_return_type_helper<T(TS...)> {
  using type = T;
};

template<class T>
using callable_return_type = typename callable_return_type_helper<T>::type;

/**
 * callable_args
 * 
 * Definition: callable_args<R(TS...)> = pack<TS...>
 */
template<class T>
struct callable_args_helper:
public callable_args_helper<callable<T>> {};

template<class T, class... TS>
struct callable_args_helper<T(TS...)> {
  using type = pack<TS...>;
};

template<class T>
using callable_args = typename callable_args_helper<T>::type;


}
