// shared_queue.hpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef _TPLMGR_SHARED_QUEUE_HPP_
#define _TPLMGR_SHARED_QUEUE_HPP_
#include <tplmgr/core.hpp>
#if _TPLMGR_PREPROCESSOR_GUARD
#include <tplmgr/allocator.hpp>
#include <tplmgr/shared_lock.hpp>
#include <tplmgr/utils.hpp>
#include <cstddef>
#include <type_traits>

_TPLMGR_BEGIN
// STRUCT TEMPLATE _Unsynchronized_queue_node
template <class _Ty>
struct _Unsynchronized_queue_node {
    constexpr _Unsynchronized_queue_node() noexcept : _Next(nullptr), _Prev(nullptr), _Value() {}

    explicit constexpr _Unsynchronized_queue_node(const _Ty& _Val) noexcept
        : _Next(nullptr), _Prev(nullptr), _Value(_Val) {}

    explicit constexpr _Unsynchronized_queue_node(_Ty&& _Val) noexcept
        : _Next(nullptr), _Prev(nullptr), _Value(_STD move(_Val)) {}

    _CONSTEXPR20 ~_Unsynchronized_queue_node() noexcept {}

    _Unsynchronized_queue_node(const _Unsynchronized_queue_node&) = delete;
    _Unsynchronized_queue_node& operator=(const _Unsynchronized_queue_node&) = delete;

    _Unsynchronized_queue_node* _Next; // pointer to the next node
    _Unsynchronized_queue_node* _Prev; // pointer to the previous node
    _Ty _Value;
};

// STRUCT TEMPLATE _Unsynchronized_queue_storage
template <class _Ty>
struct _Unsynchronized_queue_storage {
    constexpr _Unsynchronized_queue_storage() noexcept : _First(nullptr), _Last(nullptr), _Size(0) {}

    _CONSTEXPR20 ~_Unsynchronized_queue_storage() noexcept {}

    _Unsynchronized_queue_storage(const _Unsynchronized_queue_storage&) = delete;
    _Unsynchronized_queue_storage& operator=(const _Unsynchronized_queue_storage&) = delete;

    _Unsynchronized_queue_node<_Ty>* _First; // pointer to the first node
    _Unsynchronized_queue_node<_Ty>* _Last; // pointer to the last node
    size_t _Size; // number of nodes
};

// CLASS TEMPLATE _Unsynchronized_queue
template <class _Ty>
class _Unsynchronized_queue { // thread-safe node-based queue
private:
    using _Alloc     = allocator<void>;
    using _Node_t    = _Unsynchronized_queue_node<_Ty>;
    using _Storage_t = _Unsynchronized_queue_storage<_Ty>;

public:
    using value_type      = _Ty;
    using size_type       = size_t;
    using difference_type = ptrdiff_t;
    using pointer         = _Ty*;
    using const_pointer   = const _Ty*;
    using reference       = _Ty&;
    using const_reference = const _Ty&;

    constexpr _Unsynchronized_queue() noexcept : _Mypair(_Ebco_default_init{}) {}

    _CONSTEXPR20 ~_Unsynchronized_queue() noexcept {
        _Clear();
    }

    _Unsynchronized_queue(const _Unsynchronized_queue&) = delete;
    _Unsynchronized_queue& operator=(const _Unsynchronized_queue&) = delete;

    _NODISCARD constexpr bool _Empty() const noexcept {
        return _Mypair._Val1._Size == 0;
    }

    _NODISCARD constexpr size_type _Size() const noexcept {
        return _Mypair._Val1._Size;
    }

    _NODISCARD constexpr size_type _Max_size() const noexcept {
        return static_cast<size_type>(-1) / sizeof(_Ty);
    }

    _NODISCARD constexpr bool _Full() const noexcept {
        return _Mypair._Val1._Size == _Max_size();
    }

    _NODISCARD constexpr value_type _Front() const noexcept {
        return !_Empty() ? _Mypair._Val1._First->_Value : value_type{};
    }

    constexpr void _Clear() noexcept {
        if (!_Empty()) {
            _Storage_t& _Storage = _Mypair._Val1;
            _Alloc& _Al          = _Mypair._Get_val2();
            _Node_t* _Next;
            for (_Node_t* _Node = _Storage._First; _Node != nullptr; _Node = _Next) {
                _Next = _Node->_Next;
                _Node->~_Unsynchronized_queue_node();
                _Al.deallocate(_Node, sizeof(_Node_t));
            }

            _Storage._First = nullptr;
            _Storage._Last  = nullptr;
            _Storage._Size  = 0;
        }
    }

    _NODISCARD constexpr bool _Push(const _Ty& _Val) noexcept {
        _Alloc& _Al      = _Mypair._Get_val2();
        void* const _Raw = _Al.allocate(sizeof(_Node_t));
        if (!_Raw) { // allocation failed
            return false;
        }

        _Storage_t& _Storage = _Mypair._Val1;
        if (_Storage._Size == 0) { // allocate the first node
            _Storage._First = ::new (_Raw) _Node_t(_Val);
            _Storage._Last  = _Storage._First;
        } else { // allocate the next node
            _Storage._Last->_Next = ::new (_Raw) _Node_t(_Val);
            _Storage._Last        = _Storage._Last->_Next;
        }
        
        ++_Storage._Size;
        return true;
    }

    _NODISCARD constexpr bool _Push(_Ty&& _Val) noexcept {
        _Alloc& _Al      = _Mypair._Get_val2();
        void* const _Raw = _Al.allocate(sizeof(_Node_t));
        if (!_Raw) { // allocation failed
            return false;
        }

        _Storage_t& _Storage = _Mypair._Val1;
        if (_Storage._Size == 0) { // allocate the first node
            _Storage._First = ::new (_Raw) _Node_t(_STD move(_Val));
            _Storage._Last  = _Storage._First;
        } else { // allocate the next node
            _Storage._Last->_Next        = ::new (_Raw) _Node_t(_STD move(_Val));
            _Storage._Last->_Next->_Prev = _Storage._Last;
            _Storage._Last               = _Storage._Last->_Next;
        }
        
        ++_Storage._Size;
        return true;
    }

    constexpr _Ty _Pop() noexcept {
        _Storage_t& _Storage = _Mypair._Val1;
        switch (_Storage._Size) {
        case 0:
            return _Ty{};
        case 1:
        {
            _Alloc& _Al     = _Mypair._Get_val2();
            _Node_t* _First = _Storage._First;
            const _Ty _Val  = static_cast<_Ty&&>(_First->_Value);
            _First->~_Unsynchronized_queue_node();
            _Al.deallocate(_First, sizeof(_Node_t));
            _Storage._First = nullptr;
            _Storage._Last  = nullptr;
            _Storage._Size  = 0;
            return _Val;
        }
        default:
        {
            _Alloc& _Al          = _Mypair._Get_val2();
            _Node_t* _First      = _Storage._First;
            _First->_Next->_Prev = nullptr;
            _Storage._First      = _First->_Next;
            const _Ty _Val       = static_cast<_Ty&&>(_First->_Value);
            _First->~_Unsynchronized_queue_node();
            _Al.deallocate(_First, sizeof(_Node_t));
            --_Storage._Size;
            return _Val;
        }
        }
    }

private:
    _Ebco_pair<_Storage_t, _Alloc> _Mypair;
};

// CLASS TEMPLATE shared_queue
template <class _Ty>
class shared_queue { // queue that can be shared between multiple threads
private:
    using _Container = _Unsynchronized_queue<_Ty>;

public:
    using value_type      = typename _Container::value_type;
    using size_type       = typename _Container::size_type;
    using difference_type = typename _Container::difference_type;
    using pointer         = typename _Container::pointer;
    using const_pointer   = typename _Container::const_pointer;
    using reference       = typename _Container::reference;
    using const_reference = typename _Container::const_reference;

    constexpr shared_queue() noexcept : _Mycont(), _Mylock() {}

    _CONSTEXPR20 ~shared_queue() noexcept {}

    shared_queue(const shared_queue&) = delete;
    shared_queue& operator=(const shared_queue&) = delete;

    constexpr void clear() noexcept {
        lock_guard _Guard(_Mylock);
        _Mycont._Clear();
    }

    _NODISCARD constexpr bool empty() const noexcept {
        shared_lock_guard _Guard(_Mylock);
        return _Mycont._Empty();
    }

    _NODISCARD constexpr bool full() const noexcept {
        shared_lock_guard _Guard(_Mylock);
        return _Mycont._Full();
    }

    _NODISCARD constexpr size_type size() const noexcept {
        shared_lock_guard _Guard(_Mylock);
        return _Mycont._Size();
    }

    _NODISCARD constexpr size_type max_size() const noexcept {
        shared_lock_guard _Guard(_Mylock);
        return _Mycont._Max_size();
    }

    _NODISCARD constexpr value_type front() const noexcept {
        shared_lock_guard _Guard(_Mylock);
        return _Mycont._Front();
    }
    
    _NODISCARD constexpr bool push(const value_type& _Val) {
        lock_guard _Guard(_Mylock);
        return _Mycont._Push(_Val);
    }

    _NODISCARD constexpr bool push(value_type&& _Val) {
        lock_guard _Guard(_Mylock);
        return _Mycont._Push(_STD move(_Val));
    }

    constexpr value_type pop() noexcept {
        lock_guard _Guard(_Mylock);
        return _Mycont._Pop();
    }

private:
    _Container _Mycont;
    mutable shared_lock _Mylock;
};
_TPLMGR_END

#endif // _TPLMGR_PREPROCESSOR_GUARD
#endif // _TPLMGR_SHARED_QUEUE_HPP_