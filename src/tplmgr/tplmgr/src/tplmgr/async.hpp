// async.hpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef _TPLMGR_ASYNC_HPP_
#define _TPLMGR_ASYNC_HPP_
#include <tplmgr/core.hpp>
#if _TPLMGR_PREPROCESSOR_GUARD
#include <tplmgr/allocator.hpp>
#include <tplmgr/thread_pool.hpp>
#include <tplmgr/utils.hpp>
#include <tuple>
#include <type_traits>

_TPLMGR_BEGIN
// STD types
using _STD decay_t;
using _STD tuple;

// CLASS TEMPLATE _Task_invoker
template <class _Fn, class... _Types>
class _Task_invoker { // converts custom function type into thread start routine
public:
    using _Tuple = tuple<decay_t<_Fn>, decay_t<_Types>...>;
    using _Alloc = allocator<void>;

    static constexpr void _Get_invoker(void* const _Data) {
        _Tuple& _Packed = *static_cast<_Tuple*>(_Data);
        const _Fn _Func = _STD forward<_Fn>(_STD get<_Fn>(_Packed));
        _Func(_STD forward<_Types>(_STD get<_Types>(_Packed))...);
        _Alloc _Al;
        _Packed.~_Tuple(); // destroy packed data
        _Al.deallocate(_TPLMGR addressof(_Packed), sizeof(_Tuple));
    }

    static constexpr _Tuple* _Pack_data(_Fn&& _Func, _Types&&... _Args) {
        _Alloc _Al;
        void* const _Raw = _Al.allocate(sizeof(_Tuple));
        if (!_Raw) { // allocation failed
            return nullptr;
        }

        return ::new (_Raw) _Tuple(_STD forward<_Fn>(_Func), _STD forward<_Types>(_Args)...);
    }
};

// FUNCTION TEMPLATE async
template <class _Fn, class... _Types>
_NODISCARD_ATTR bool async(thread_pool& _Pool, _Fn&& _Func, _Types&&... _Args) noexcept {
    using _Invoker_t        = _Task_invoker<_Fn, _Types...>;
    using _Tuple_t          = typename _Invoker_t::_Tuple;
    const auto _Invoker     = &_Invoker_t::_Get_invoker;
    _Tuple_t* const _Packed = _Invoker_t::_Pack_data(
        _STD forward<_Fn>(_Func), _STD forward<_Types>(_Args)...);
    if (_Packed) { // allocation succeeded, try schedule a new task
        return _Pool.schedule_task(_Invoker, _Packed);
    } else { // allocation failed, do nothing
        return false;
    }
}

template <class _Fn, class... _Types>
_NODISCARD_ATTR bool async(
    thread_pool& _Pool, const task_priority _Priority, _Fn&& _Func, _Types&&... _Args) noexcept {
    using _Invoker_t        = _Task_invoker<_Fn, _Types...>;
    using _Tuple_t          = typename _Invoker_t::_Tuple;
    const auto _Invoker     = &_Invoker_t::_Get_invoker;
    _Tuple_t* const _Packed = _Invoker_t::_Pack_data(
        _STD forward<_Fn>(_Func), _STD forward<_Types>(_Args)...);
    if (_Packed) { // allocation succeeded, try schedule a new task
        return _Pool.schedule_task(_Invoker, _Packed, _Priority);
    } else { // allocation failed, do nothing
        return false;
    }
}
_TPLMGR_END

#endif // _TPLMGR_PREPROCESSOR_GUARD
#endif // _TPLMGR_ASYNC_HPP_