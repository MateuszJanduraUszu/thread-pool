// utils.hpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef _TPLMGR_UTILS_HPP_
#define _TPLMGR_UTILS_HPP_
#include <tplmgr/core.hpp>
#if _TPLMGR_PREPROCESSOR_GUARD
#include <cstdint>
#include <type_traits>

_TPLMGR_BEGIN
// STD types
using _STD conditional_t;
using _STD false_type;
using _STD enable_if_t;
using _STD is_nothrow_constructible;
using _STD is_nothrow_default_constructible;
using _STD is_nothrow_move_constructible;
using _STD is_same;
using _STD remove_reference_t;
using _STD true_type;

// FUNCTION TEMPLATE addressof
template <class _Ty>
#if _HAS_CXX17_FEATURES
constexpr
#endif // _HAS_CXX17_FEATURES
    _Ty* addressof(_Ty& _Obj) noexcept {
#if _HAS_BUILTIN(__builtin_addressof)
    return __builtin_addressof(_Obj);
#else // ^^^ _HAS_BUILTIN(__builtin_addressof) ^^^ / vvv !_HAS_BUILTIN(__builtin_addressof) vvv
    return reinterpret_cast<_Ty*>(
        &const_cast<char&>(reinterpret_cast<const volatile char&>(_Obj)));
#endif // _HAS_BUILTIN(__builtin_addressof)
}

template <class _Ty>
const _Ty* addressof(const _Ty&&) = delete;

// FUNCTION TEMPLATE exchange
template <class _Ty, class _Other = _Ty>
#if _HAS_CXX20_FEATURES
constexpr
#endif // _HAS_CXX20_FEATURES
    _Ty exchange(_Ty& _Obj, _Other&& _New_val) noexcept(_STD is_nothrow_move_constructible_v<_Ty>
        && _STD is_nothrow_assignable_v<_Ty&, _Other&&>) {
    const _Ty _Old_val = static_cast<_Ty&&>(_Obj);
    _Obj               = static_cast<_Other&&>(_New_val);
    return _Old_val;
}

#if _HAS_CXX17_FEATURES
// CONSTANT TEMPLATE _Conjunction_v
template <class... _Traits>
inline constexpr bool _Conjunction_v = _STD conjunction_v<_Traits...>;
#else // ^^^ _HAS_CXX17_FEATURES ^^^ / vvv !_HAS_CXX17_FEATURES vvv
// STRUCT TEMPLATE _Conjunction
template <class... _Traits>
struct _Conjunction : true_type {};

template <class _Trait>
struct _Conjunction<_Trait> : _Trait {};

template <class _Trait, class... _Traits>
struct _Conjunction<_Trait, _Traits...> : conditional_t<_Trait::value, _Conjunction<_Traits...>, _Trait> {};

// CONSTANT TEMPLATE _Conjunction_v
template <class... _Traits>
constexpr bool _Conjunction_v = _Conjunction<_Traits...>::value;
#endif // _HAS_CXX17_FEATURES

#if _HAS_CXX17_FEATURES
// CONSTANT TEMPLATE _Disjunction_v
template <class... _Traits>
inline constexpr bool _Disjunction_v = _STD disjunction_v<_Traits...>;
#else // ^^^ _HAS_CXX17_FEATURES ^^^ / vvv !_HAS_CXX17_FEATURES vvv
// STRUCT TEMPLATE _Disjunction
template <class... _Traits>
struct _Disjunction : false_type {};

template <class _Trait>
struct _Disjunction<_Trait> : _Trait {};

template <class _Trait, class... _Traits>
struct _Disjunction<_Trait, _Traits...> : conditional_t<_Trait::value, _Trait, _Disjunction<_Traits...>> {};

// CONSTANT TEMPLATE _Disjunction_v
template <class... _Traits>
constexpr bool _Disjunction_v = _Disjunction<_Traits...>::value;
#endif // _HAS_CXX17_FEATURES

// CONSTANT TEMPLATE _Is_any_of_v
template <class _Ty, class... _Types>
_INLINE_VARIABLE constexpr bool _Is_any_of_v = _Disjunction_v<is_same<_Ty, _Types>...>;

// STRUCT _Ebco_default_init
struct _Ebco_default_init { // tag for _Ebco_pair constructor (default initialization)
    _Ebco_default_init() = default;
};

// STRUCT _Ebco_custom_init
struct _Ebco_custom_init { // tag for _Ebco_pair constructor (custom initialization)
    _Ebco_custom_init() = default;
};

// CLASS TEMPLATE _Ebco_pair
template <class _Ty1, class _Ty2, bool = _STD is_empty_v<_Ty2>
#if _HAS_CXX14_FEATURES
    && !_STD is_final_v<_Ty2>
#endif // _HAS_CXX14_FEATURES
>
class _Ebco_pair final : public _Ty2 { // store _Ty1 and derive from _Ty2 (EBCO active)
public:
    _Ty1 _Val1;

    template <class... _Types,
        enable_if_t<!_Is_any_of_v<_Ebco_pair, remove_reference_t<_Types>...>, int> = 0>
    constexpr _Ebco_pair(_Ebco_default_init, _Types&&... _Args) noexcept(
        _Conjunction_v<is_nothrow_constructible<_Ty1, _Types...>,
            is_nothrow_default_constructible<_Ty2>>) : _Val1(_STD forward<_Types>(_Args)...), _Ty2() {}

    template <class _Other, class... _Types,
        enable_if_t<!_STD is_same_v<remove_reference_t<_Other>, _Ebco_pair>
            && !_Is_any_of_v<_Ebco_pair, remove_reference_t<_Types>...>, int> = 0>
    constexpr _Ebco_pair(_Ebco_custom_init, _Other&& _Val, _Types&&... _Args) noexcept(
        _Conjunction_v<is_nothrow_constructible<_Ty1, _Types...>, is_nothrow_move_constructible<_Ty2>>)
        : _Val1(_STD forward<_Types>(_Args)...), _Ty2(_STD forward<_Other>(_Val)) {}

    constexpr _Ty2& _Get_val2() noexcept {
        return *this;
    }

    constexpr const _Ty2& _Get_val2() const noexcept {
        return *this;
    }
};

template <class _Ty1, class _Ty2>
class _Ebco_pair<_Ty1, _Ty2, false> final { // store _Ty1 and _Ty2 (EBCO inactive)
public:
    _Ty1 _Val1;
    _Ty2 _Val2;

    template <class... _Types,
        enable_if_t<!_Is_any_of_v<_Ebco_pair, remove_reference_t<_Types>...>, int> = 0>
    constexpr _Ebco_pair(_Ebco_default_init, _Types&&... _Args) noexcept(
        _Conjunction_v<is_nothrow_constructible<_Ty1, _Types...>,
            is_nothrow_default_constructible<_Ty2>>) : _Val1(_STD forward<_Types>(_Args)...), _Val2() {}

    template <class _Other, class... _Types,
        enable_if_t<!_STD is_same_v<remove_reference_t<_Other>, _Ebco_pair>
            && !_Is_any_of_v<_Ebco_pair, remove_reference_t<_Types>...>, int> = 0>
    constexpr _Ebco_pair(_Ebco_custom_init, _Other&& _Val, _Types&&... _Args) noexcept(
        _Conjunction_v<is_nothrow_constructible<_Ty1, _Types...>, is_nothrow_move_constructible<_Ty2>>)
        : _Val1(_STD forward<_Types>(_Args)...), _Val2(_STD forward<_Other>(_Val)) {}

    constexpr _Ty2& _Get_val2() noexcept {
        return _Val2;
    }

    constexpr const _Ty2& _Get_val2() const noexcept {
        return _Val2;
    }
};
_TPLMGR_END

#endif // _TPLMGR_PREPROCESSOR_GUARD
#endif // _TPLMGR_UTILS_HPP_