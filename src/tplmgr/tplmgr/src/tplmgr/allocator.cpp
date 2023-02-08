// allocator.cpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#include <build/tplmgr_pch.hpp>
#include <tplmgr/allocator.hpp>
#if _TPLMGR_PREPROCESSOR_GUARD

_TPLMGR_BEGIN
// FUNCTION _Is_pow_of_2
_NODISCARD constexpr bool _Is_pow_of_2(const size_t _Val) noexcept {
    return _Val > 0 && (_Val & (_Val - 1)) == 0;
}

// FUNCTION allocator_traits::allocate
_NODISCARD __declspec(allocator) allocator_traits::pointer allocator_traits::allocate(
    const size_type _Size, const size_type _Align) noexcept {
    if (_Size == 0) { // no allocation
        return nullptr;
    }

#ifdef __cpp_aligned_new
    if (!_Is_pow_of_2(_Align)) { // alignment must be a power of 2
        return nullptr;
    }

    return ::operator new(_Size, align_val_t{_Align}, _STD nothrow);
#else // ^^^ __cpp_aligned_new ^^^ / vvv !__cpp_aligned_new vvv
    (void) _Align; // alignment not used
    return ::operator new(_Size, _STD nothrow);
#endif // __cpp_aligned_new
}

// FUNCTION allocator_traits::deallocate
void allocator_traits::deallocate(
    pointer _Ptr, const size_type _Size, const size_type _Align) noexcept {
    if (!_Ptr || _Size == 0) { // no deallocation
        return;
    }

#ifdef __cpp_aligned_new
    if (!_Is_pow_of_2(_Align)) { // alignment must be a power of 2
        return;
    }

    ::operator delete(_Ptr, _Size, align_val_t{_Align});
#else // ^^^ __cpp_aligned_new ^^^ / vvv !__cpp_aligned_new vvv
    (void) _Align; // alignment not used
    ::operator delete(_Ptr, _Size);
#endif // __cpp_aligned_new
}
_TPLMGR_END

#endif // _TPLMGR_PREPROCESSOR_GUARD