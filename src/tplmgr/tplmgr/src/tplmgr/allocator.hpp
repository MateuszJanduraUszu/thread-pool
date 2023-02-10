// allocator.hpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef _TPLMGR_ALLOCATOR_HPP_
#define _TPLMGR_ALLOCATOR_HPP_
#include <tplmgr/core.hpp>
#if _TPLMGR_PREPROCESSOR_GUARD
#include <tplmgr/utils.hpp>
#include <algorithm>
#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>

_TPLMGR_BEGIN
#ifdef __cpp_aligned_new
// STD types
using _STD align_val_t;
#endif // __cpp_aligned_new

// CONSTANT TEMPLATE _Default_new_alignof
template <class _Ty>
_INLINE_VARIABLE constexpr size_t _Default_new_alignof = (_STD max)(
    __STDCPP_DEFAULT_NEW_ALIGNMENT__, alignof(_Ty)); // choose default allocation alignment

// FUNCTION _Is_pow_of_2
extern _NODISCARD constexpr bool _Is_pow_of_2(const size_t _Val) noexcept;

// STRUCT allocator_traits
struct _TPLMGR_API allocator_traits {
    using value_type      = void;
    using size_type       = size_t;
    using difference_type = ptrdiff_t;
    using pointer         = void*;
    using const_pointer   = const void*;

    // tries to allocate _Size bytes of memory (alignment is optional)
    _NODISCARD static __declspec(allocator) pointer allocate(
        const size_type _Size, const size_type _Align) noexcept;

    // tries to deallocate _Size bytes of memory (alignment is optional)
    static void deallocate(pointer _Ptr, const size_type _Size, const size_type _Align) noexcept;
};

// CLASS TEMPLATE allocator
template <class _Ty>
class allocator { // class for thread-safe allocation/deallocation
public:
    using value_type      = _Ty;
    using size_type       = size_t;
    using difference_type = ptrdiff_t;
    using pointer         = _Ty*;
    using const_pointer   = const _Ty*;
    using reference       = _Ty&;
    using const_reference = const _Ty&;

    constexpr allocator() noexcept                 = default;
    constexpr allocator(const allocator&) noexcept = default;
    _CONSTEXPR20 ~allocator() noexcept             = default;

    template <class _Other>
    constexpr allocator(const allocator<_Other>&) noexcept {}

    template <class _Other>
    constexpr allocator& operator=(const allocator<_Other>&) noexcept {
        return *this;
    }

    _NODISCARD _CONSTEXPR20 __declspec(allocator) pointer allocate(const size_type _Count) noexcept {
        return static_cast<pointer>(
            allocator_traits::allocate(_Count * sizeof(_Ty)), _Default_new_alignof<_Ty>);
    }

    template <class _Other, class... _Types>
    void construct(_Other* const _Ptr, _Types&&... _Args) noexcept {
#if _HAS_CXX20
        _STD construct_at(_Ptr, _STD forward<_Types>(_Args)...);
#else // ^^^ _HAS_CXX20 ^^^ / vvv !_HAS_CXX20 vv
        ::new (_Ptr) _Ty(_STD forward<_Types>(_Args)...);
#endif // _HAS_CXX20
    }

    _CONSTEXPR20 void deallocate(pointer _Ptr, const size_type _Count) noexcept {
        allocator_traits::deallocate(_Ptr, _Count * sizeof(_Ty), _Default_new_alignof<_Ty>);
    }

    template <class _Other>
    void destroy(_Other* const _Ptr) noexcept {
#if _HAS_CXX17
        _STD destroy_at(_Ptr);
#else // ^^^ _HAS_CXX17 ^^^ / vvv !_HAS_CXX17 vvv
        _Ptr->~_Other();
#endif // _HAS_CXX17
    }

    _NODISCARD constexpr size_type max_size() const noexcept {
        return static_cast<size_type>(-1) / sizeof(value_type);
    }
};

template <>
class allocator<void> { // class for thread-safe allocation/deallocation (special case)
public:
    using value_type      = void;
    using size_type       = size_t;
    using difference_type = ptrdiff_t;
    using pointer         = void*;
    using const_pointer   = const void*;

    template <class _Other>
    struct rebind {
        using other = allocator<_Other>;
    };

    constexpr allocator() noexcept                 = default;
    constexpr allocator(const allocator&) noexcept = default;
    _CONSTEXPR20 ~allocator() noexcept             = default;

    template <class _Other>
    constexpr allocator(const allocator<_Other>&) noexcept {}

    template <class _Other>
    constexpr allocator& operator=(const allocator<_Other>&) noexcept {
        return *this;
    }

    _NODISCARD __declspec(allocator) pointer allocate(const size_type _Size) noexcept {
        return allocator_traits::allocate(_Size, __STDCPP_DEFAULT_NEW_ALIGNMENT__);
    }

    void deallocate(pointer _Ptr, const size_type _Size) noexcept {
        allocator_traits::deallocate(_Ptr, _Size, __STDCPP_DEFAULT_NEW_ALIGNMENT__);
    }

    _NODISCARD constexpr size_type max_size() noexcept {
        return static_cast<size_type>(-1);
    }
};
_TPLMGR_END

#endif // _TPLMGR_PREPROCESSOR_GUARD
#endif // _TPLMGR_ALLOCATOR_HPP_