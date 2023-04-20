// stack.hpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef _TPLMGR_STACK_HPP_
#define _TPLMGR_STACK_HPP_
#include <tplmgr/core.hpp>
#if _TPLMGR_PREPROCESSOR_GUARD
#include <tplmgr/allocator.hpp>
#include <tplmgr/utils.hpp>
#include <cstddef>
#include <type_traits>

_TPLMGR_BEGIN
// STRUCT TEMPLATE _Stack_node
template <class _Ty>
struct _Stack_node {
    _Stack_node() noexcept : _Next(nullptr), _Value() {}

    explicit _Stack_node(const _Ty& _Val) noexcept : _Next(nullptr), _Value(_Val) {}

    explicit _Stack_node(_Ty&& _Val) noexcept : _Next(nullptr), _Value(_STD move(_Val)) {}

    ~_Stack_node() noexcept {}

    _Stack_node(const _Stack_node&) = delete;
    _Stack_node& operator=(const _Stack_node&) = delete;

    _Stack_node* _Next; // pointer to the next node
    _Ty _Value; // the stored value
};

template <class _Ty>
struct _Stack_storage {
    _Stack_storage() noexcept : _Bottom(nullptr), _Top(nullptr), _Size(0) {}

    ~_Stack_storage() noexcept {}

    _Stack_storage(const _Stack_storage&) = delete;
    _Stack_storage& operator=(const _Stack_storage&) = delete;

    _Stack_node<_Ty>* _Bottom; // pointer to the first node
    _Stack_node<_Ty>* _Top; // pointer to the last node
    size_t _Size; // number of nodes
};

// CLASS TEMPLATE _Stack
template <class _Ty>
class _Stack { // iterable node-based stack
private:
    // Note: This "stack" is not a standard stack container.
    //       It is used as a thread's event callback container, so it needs to be iterable.
    //       There is no need to access a specific element. Its primary use is to
    //       iterate from bottom to top as a continuous array.
    using _Alloc     = allocator<void>;
    using _Node_t    = _Stack_node<_Ty>;
    using _Storage_t = _Stack_storage<_Ty>;

public:
    using value_type      = _Ty;
    using size_type       = size_t;
    using difference_type = ptrdiff_t;
    using pointer         = _Ty*;
    using const_pointer   = const _Ty*;
    using reference       = _Ty&;
    using const_reference = const _Ty&;

    _Stack() noexcept : _Mypair(_Ebco_default_init{}) {}

    _Stack(_Stack&& _Other) noexcept : _Mypair(_Ebco_default_init{}) {
        _Mypair._Val1._Bottom        = _Other._Mypair._Val1._Bottom;
        _Mypair._Val1._Top           = _Other._Mypair._Val1._Top;
        _Mypair._Val1._Size          = _Other._Mypair._Val1._Size;
        _Other._Mypair._Val1._Bottom = nullptr;
        _Other._Mypair._Val1._Top    = nullptr;
        _Other._Mypair._Val1._Size   = 0;
    }

    ~_Stack() noexcept {
        _Clear();
    }
    
    _Stack& operator=(_Stack&& _Other) noexcept {
        if (this != _TPLMGR addressof(_Other)) {
            _Mypair._Val1._Bottom        = _Other._Mypair._Val1._Bottom;
            _Mypair._Val1._Top           = _Other._Mypair._Val1._Top;
            _Mypair._Val1._Size          = _Other._Mypair._Val1._Size;
            _Other._Mypair._Val1._Bottom = nullptr;
            _Other._Mypair._Val1._Top    = nullptr;
            _Other._Mypair._Val1._Size   = 0;
        }

        return *this;
    }

    _Stack(const _Stack&) = delete;
    _Stack& operator=(const _Stack&) = delete;

    bool _Empty() const noexcept {
        return _Mypair._Val1._Size == 0;
    }

    size_type _Size() const noexcept {
        return _Mypair._Val1._Size;
    }

    _Node_t* _Bottom() noexcept {
        return _Mypair._Val1._Bottom;
    }

    const _Node_t* _Bottom() const noexcept {
        return _Mypair._Val1._Bottom;
    }

    _Node_t* _Top() noexcept {
        return _Mypair._Val1._Top;
    }

    const _Node_t* _Top() const noexcept {
        return _Mypair._Val1._Top;
    }

    void _Clear() noexcept {
        if (!_Empty()) {
            _Storage_t& _Storage = _Mypair._Val1;
            _Alloc& _Al          = _Mypair._Get_val2();
            _Node_t* _Next;
            for (_Node_t* _Node = _Storage._Bottom; _Node != nullptr; _Node = _Next) {
                _Next = _Node->_Next;
                _Node->~_Stack_node();
                _Al.deallocate(_Node, sizeof(_Node_t));
            }

            _Storage._Bottom = nullptr;
            _Storage._Top    = nullptr;
            _Storage._Size   = 0;
        }
    }

    _NODISCARD_ATTR bool _Push(const _Ty& _Val) noexcept {
        _Alloc& _Al      = _Mypair._Get_val2();
        void* const _Raw = _Al.allocate(sizeof(_Node_t));
        if (!_Raw) { // allocation failed
            return false;
        }

        _Storage_t& _Storage = _Mypair._Val1;
        if (_Storage._Size == 0) { // allocate the first node
            _Storage._Bottom = ::new (_Raw) _Node_t(_Val);
            _Storage._Top    = _Storage._Bottom;
        } else { // allocate the next node
            _Storage._Top->_Next = ::new (_Raw) _Node_t(_Val);
            _Storage._Top        = _Storage._Top->_Next;
        }

        ++_Storage._Size;
        return true;
    }

    _NODISCARD_ATTR bool _Push(_Ty&& _Val) noexcept {
        _Alloc& _Al      = _Mypair._Get_val2();
        void* const _Raw = _Al.allocate(sizeof(_Node_t));
        if (!_Raw) { // allocation failed
            return false;
        }

        _Storage_t& _Storage = _Mypair._Val1;
        if (_Storage._Size == 0) { // allocate the first node
            _Storage._Bottom = ::new (_Raw) _Node_t(_STD move(_Val));
            _Storage._Top    = _Storage._Bottom;
        } else { // allocate the next node
            _Storage._Top->_Next = ::new (_Raw) _Node_t(_STD move(_Val));
            _Storage._Top        = _Storage._Top->_Next;
        }

        ++_Storage._Size;
        return true;
    }

    void _Pop() noexcept {
        _Storage_t& _Storage = _Mypair._Val1;
        switch (_Storage._Size) {
        case 0:
            break;
        case 1:
        {
            _Alloc& _Al    = _Mypair._Get_val2();
            _Node_t* _Node = _Storage._Bottom; // same as _Storage._Top
            _Node->~_Stack_node();
            _Al.deallocate(_Node, sizeof(_Node_t));
            _Storage._Bottom = nullptr;
            _Storage._Top    = nullptr;
            _Storage._Size   = 0;
            break;
        }
        case 2:
        {
            _Alloc& _Al    = _Mypair._Get_val2();
            _Node_t* _Node = _Storage._Top;
            _Node->~_Stack_node();
            _Al.deallocate(_Node, sizeof(_Node_t));
            _Storage._Bottom->_Next = nullptr;
            _Storage._Top           = _Storage._Bottom;
            _Storage._Size          = 1;
            break;
        }
        default:
        {
            _Alloc& _Al    = _Mypair._Get_val2();
            _Node_t* _Node = _Storage._Bottom;
            for (size_type _Which = 0; _Which < _Storage._Size - 2; ++_Which) { // find penultimate node
                _Node = _Node->_Next;
            }

            _Node_t* _Top = _Node->_Next; // _Node->_Next is same as _Storage._Top
            _Node->_Next  = nullptr;
            _Top->~_Stack_node();
            _Al.deallocate(_Top, sizeof(_Node_t));
            _Storage._Top = _Node;
            --_Storage._Size;
            break;
        }
        }
    }

private:
    _Ebco_pair<_Storage_t, _Alloc> _Mypair;
};
_TPLMGR_END

#endif // _TPLMGR_PREPROCESSOR_GUARD
#endif // _TPLMGR_STACK_HPP_