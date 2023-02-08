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
// Only Microsoft Visual C++ compiler is supported.
#ifndef _MSC_VER
#error Requires the Microsoft Visual C++ compiler.
#endif // _MSC_VER

// Only 32/64-bit platforms are supported.
#if !defined(_M_IX86) && !defined(_M_X64)
#error Requires 32/64-bit platform.
#endif // !defined(_M_IX86) && !defined(_M_X64)

#ifdef TPLMGR_EXPORTS
#define _TPLMGR_API __declspec(dllexport)
#else // ^^^ TPLMGR_EXPORTS ^^^ / vvv !TPLMGR_EXPORTS vvv
#define _TPLMGR_API __declspec(dllimport)
#endif // TPLMGR_EXPORTS

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
#ifndef __STDCPP_DEFAULT_NEW_ALIGNMENT__
#ifdef _M_X64
#define __STDCPP_DEFAULT_NEW_ALIGNMENT__ 16
#else // ^^^ _M_X64 ^^^ / vvv _M_IX86 vvv
#define __STDCPP_DEFAULT_NEW_ALIGNMENT__ 8
#endif // _M_X64
#endif // __STDCPP_DEFAULT_NEW_ALIGNMENT__

// TPLMGR namespace
#define _TPLMGR_BEGIN namespace tplmgr {
#define _TPLMGR_END   }
#define _TPLMGR       ::tplmgr::

// C++17 Inline Variables, see P0386R2
#ifdef __cpp_inline_variables
#define _INLINE_VARIABLE inline
#else // ^^^ __cpp_inline_variables ^^^ / vvv !__cpp_inline_variables vvv
#define _INLINE_VARIABLE
#endif // __cpp_inline_variables

#endif // _TPLMGR_PREPROCESSOR_GUARD
#endif // _TPLMGR_CORE_HPP_