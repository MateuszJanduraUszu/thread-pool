// core.hpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef _TPLMGR_CORE_HPP_
#define _TPLMGR_CORE_HPP_

// Each header should be protected against C++/CLI and non-Windows platform.
#ifndef _TPLMGR_PREPROCESSOR_GUARD
#if defined(_M_CEE) || !defined(_WIN32)
#define _TPLMGR_PREPROCESSOR_GUARD 0
#else // ^^^ defined(_M_CEE) || !defined(_WIN32) ^^^ / vvv !defined(_M_CEE) && defined(_WIN32) vvv
#define _TPLMGR_PREPROCESSOR_GUARD 1
#endif // defined(_M_CEE) || !defined(_WIN32)
#endif // _TPLMGR_PREPROCESSOR_GUARD

#if _TPLMGR_PREPROCESSOR_GUARD
// The implementation may use features from newer C++ version if possible.
#define _HAS_CXX11_FEATURES __cplusplus >= 201103L
#define _HAS_CXX14_FEATURES __cplusplus >= 201402L
#define _HAS_CXX17_FEATURES __cplusplus >= 201703L
#define _HAS_CXX20_FEATURES __cplusplus >= 202002L
#define _HAS_CXX23_FEATURES __cplusplus > 202002L

// Only C++11 and later is supported.
#if !_HAS_CXX11_FEATURES
#error Requires at least C++11 support.
#endif // !_HAS_CXX11_FEATURES

// Only 32/64-bit platforms are supported.
#if !defined(_M_IX86) && !defined(_M_X64)
#error Requires 32/64-bit platform.
#endif // !defined(_M_IX86) && !defined(_M_X64)

#ifdef _BUILD_TPLMGR
#define _TPLMGR_API __declspec(dllexport)
#else // ^^^ _BUILD_TPLMGR ^^^ / vvv !_BUILD_TPLMGR vvv
#define _TPLMGR_API __declspec(dllimport)
#endif // _BUILD_TPLMGR

// Use the __cdecl for 32-bit platforms and the __stdcall for 64-bit platforms.
#ifndef __STDCALL_OR_CDECL
#ifdef _M_X64
#define __STDCALL_OR_CDECL __stdcall
#else // ^^^ _M_X64 ^^^ / vvv _M_IX86 vvv
#define __STDCALL_OR_CDECL __cdecl
#endif // _M_X64
#endif // __STDCALL_OR_CDECL

// The __has_builtin() macro may be undefined.
#ifndef _HAS_BUILTIN
#ifdef __has_builtin
#define _HAS_BUILTIN(_Builtin) __has_builtin(_Builtin)
#else // ^^^ __has_builtin ^^^ / vvv !__has_builtin vvv
#define _HAS_BUILTIN(_Builtin) 0
#endif // __has_builtin
#endif // _HAS_BUILTIN

// The __STDCPP_DEFAULT_NEW_ALIGNMENT__ is undefined if aligned new is not supported.
#ifndef _DEFAULT_NEW_ALIGNMENT
#ifdef __STDCPP_DEFAULT_NEW_ALIGNMENT__
#define _DEFAULT_NEW_ALIGNMENT __STDCPP_DEFAULT_NEW_ALIGNMENT__
#else // ^^^ __STDCPP_DEFAULT_NEW_ALIGNMENT__ ^^^ / vvv !__STDCPP_DEFAULT_NEW_ALIGNMENT__ vvv
#ifdef _M_X64
#define _DEFAULT_NEW_ALIGNMENT 8
#else // ^^^ _M_X64 ^^^ / vvv _M_IX86 vvv
#define _DEFAULT_NEW_ALIGNMENT 4
#endif // _M_X64
#endif // __STDCPP_DEFAULT_NEW_ALIGNMENT__
#endif // _DEFAULT_NEW_ALIGNMENT

#if defined(_MSC_VER) || defined(__clang__)
#define _MSVC_ALLOCATOR __declspec(allocator)
#else // ^^^ defined(_MSC_VER) || defined(__clang__) ^^^ / vvv !defined(_MSC_VER) && !defined(__clang__) vvv
#define _MSVC_ALLOCATOR
#endif // defined(_MSC_VER) || defined(__clang__)

// TPLMGR namespace
#define _TPLMGR_BEGIN namespace tplmgr {
#define _TPLMGR_END   }
#define _TPLMGR       ::tplmgr::

// C++17 "nodiscard" Attribute, see P0189R1
#if _HAS_CXX17_FEATURES
#define _NODISCARD_ATTR [[nodiscard]]
#else // ^^^ _HAS_CXX17_FEATURES ^^^ / vvv !_HAS_CXX17_FEATURES vvv
#define _NODISCARD_ATTR
#endif // _HAS_CXX17_FEATURES

// C++17 Inline Variables, see P0386R2
#ifdef __cpp_inline_variables
#define _INLINE_VARIABLE inline
#else // ^^^ __cpp_inline_variables ^^^ / vvv !__cpp_inline_variables vvv
#define _INLINE_VARIABLE
#endif // __cpp_inline_variables

// C++20 Constexpr Destructors, see P0315R4
#if _HAS_CXX20_FEATURES
#define _CONSTEXPR_DTOR constexpr
#else // ^^^ _HAS_CXX20_FEATURES ^^^ / vvv !_HAS_CXX20_FEATURES vvv
#define _CONSTEXPR_DTOR
#endif // _HAS_CXX20_FEATURES

// C++20 Constexpr Dynamic Allocation, see P0784R7
#ifdef __cpp_constexpr_dynamic_alloc
#define _CONSTEXPR_DYNAMIC_ALLOC constexpr
#else // ^^^ __cpp_constexpr_dynamic_alloc ^^^ / vvv !__cpp_constexpr_dynamic_alloc vvv
#define _CONSTEXPR_DYNAMIC_ALLOC
#endif // __cpp_constexpr_dynamic_alloc

#endif // _TPLMGR_PREPROCESSOR_GUARD
#endif // _TPLMGR_CORE_HPP_