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
    _Unsynchronized_queue_node() noexcept : _Next(nullptr), _Prev(nullptr), _Value() {}

    explicit _Unsynchronized_queue_node(const _Ty& _Val) noexcept
        : _Next(nullptr), _Prev(nullptr), _Value(_Val) {}

    explicit _Unsynchronized_queue_node(_Ty&& _Val) noexcept
        : _Next(nullptr), _Prev(nullptr), _Value(_STD move(_Val)) {}

    ~_Unsynchronized_queue_node() noexcept {}

    _Unsynchronized_queue_node(const _Unsynchronized_queue_node&) = delete;
    _Unsynchronized_queue_node& operator=(const _Unsynchronized_queue_node&) = delete;

    _Unsynchronized_queue_node* _Next; // pointer to the next node
    _Unsynchronized_queue_node* _Prev; // pointer to the previous node
    _Ty _Value;
};

// STRUCT TEMPLATE _Unsynchronized_queue_storage
template <class _Ty>
struct _Unsynchronized_queue_storage {
    _Unsynchronized_queue_storage() noexcept : _First(nullptr), _Last(nullptr), _Size(0) {}

    ~_Unsynchronized_queue_storage() noexcept {}

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

    _Unsynchronized_queue() noexcept : _Mypair(_Ebco_default_init{}) {}

    ~_Unsynchronized_queue() noexcept {
        _Clear();
    }

    _Unsynchronized_queue(const _Unsynchronized_queue&) = delete;
    _Unsynchronized_queue& operator=(const _Unsynchronized_queue&) = delete;

    bool _Empty() const noexcept {
        return _Mypair._Val1._Size == 0;
    }

    size_type _Size() const noexcept {
        return _Mypair._Val1._Size;
    }

    size_type _Max_size() const noexcept {
        return static_cast<size_type>(-1) / sizeof(_Ty);
    }

    bool _Full() const noexcept {
        return _Mypair._Val1._Size == _Max_size();
    }

    _Ty _Front() const noexcept {
        return !_Empty() ? _Mypair._Val1._First->_Value : value_type{};
    }

    void _Clear() noexcept {
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

    struct _Released_storage {
        _Node_t* _First;
        _Node_t* _Last;
        size_type _Size;
    };

    _NODISCARD_ATTR _Released_storage _Release() noexcept {
        if (_Empty()) {
            return _Released_storage{nullptr, nullptr, 0};
        }
        
        _Storage_t& _Storage      = _Mypair._Val1;
        _Released_storage _Result = {_Storage._First, _Storage._Last, _Storage._Size};
        _Storage._First           = nullptr;
        _Storage._Last            = nullptr;
        _Storage._Size            = 0;
        return _Result;
    }

    void _Assign(_Node_t* const _First, _Node_t* const _Last, const size_type _Size) noexcept {
        if (!_Empty()) {
            _Clear();
        }

        _Storage_t& _Storage = _Mypair._Val1;
        _Storage._First      = _First;
        _Storage._Last       = _Last;
        _Storage._Size       = _Size;
    }

    _NODISCARD_ATTR bool _Push(const _Ty& _Val) noexcept {
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

    _NODISCARD_ATTR bool _Push(_Ty&& _Val) noexcept {
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

    template <class _Pr>
    _NODISCARD_ATTR bool _Push_with_priority(const _Ty& _Val, _Pr _Pred) noexcept {
        _Alloc& _Al      = _Mypair._Get_val2();
        void* const _Raw = _Al.allocate(sizeof(_Node_t));
        if (!_Raw) { // allocation failed
            return false;
        }

        _Storage_t& _Storage = _Mypair._Val1;
        switch (_Storage._Size) {
        case 0: // allocate the first node, discard prediction
            _Storage._First = ::new (_Raw) _Node_t(_Val);
            _Storage._Last  = _Storage._First;
            break;
        case 1: // allocate the second node, use prediction
        {
            _Node_t*& _First = _Storage._First;
            _Node_t*& _Last  = _Storage._Last;
            if (_Pred(_Val, _First->_Value)) { // insert before the first node
                _First->_Prev        = ::new (_Raw) _Node_t(_Val);
                _First->_Prev->_Next = _First;
                _Last                = _First;
                _First               = _First->_Prev;
                _First->_Next        = _Last;
            } else { // insert after the first node
                _First->_Next        = ::new (_Raw) _Node_t(_Val);
                _First->_Next->_Prev = _First;
                _Last                = _First->_Next;
                _Last->_Prev         = _First;
            }

            break;
        }
        default: // allocate the next node, use prediction
        {
            _Node_t* _Node = _Storage._Last;
            while (_Node->_Prev && _Pred(_Val, _Node->_Value)) { // find the matching node
                _Node = _Node->_Prev;
            }

            if (_Pred(_Val, _Node->_Value)) { // insert before the selected node
                if (_Node->_Prev) { // insert a new node between a pair of existing nodes
                    _Node_t* const _Temp = _Node->_Prev;
                    _Node->_Prev         = ::new (_Raw) _Node_t(_Val);
                    _Node->_Prev->_Next  = _Node;
                    _Node->_Prev->_Prev  = _Temp;
                } else { // allocate a new node
                    // Note: If _Node->_Prev is a null-pointer, it means that _Node is the first node.
                    _Node->_Prev        = ::new (_Raw) _Node_t(_Val);
                    _Node->_Prev->_Next = _Node;
                    _Storage._First     = _Node->_Prev;
                }
            } else { // insert after the selected node
                if (_Node->_Next) { // insert a new node between a pair of existing nodes
                    _Node_t* const _Temp       = _Node->_Next;
                    _Node->_Next               = ::new (_Raw) _Node_t(_Val);
                    _Node->_Next->_Next        = _Temp;
                    _Node->_Next->_Next->_Prev = _Node->_Next;
                    _Node->_Next->_Prev        = _Node;
                } else { // allocate a new node
                    // Note: If _Node->_Next is a null-pointer, it means that _Node is the last node.
                    _Node->_Next        = ::new (_Raw) _Node_t(_Val);
                    _Node->_Next->_Prev = _Node;
                    _Storage._Last      = _Node->_Next;
                }
            }

            break;
        }
        }

        ++_Storage._Size;
        return true;
    }

    template <class _Pr>
    _NODISCARD_ATTR bool _Push_with_priority(_Ty&& _Val, _Pr _Pred) noexcept {
        _Alloc& _Al      = _Mypair._Get_val2();
        void* const _Raw = _Al.allocate(sizeof(_Node_t));
        if (!_Raw) { // allocation failed
            return false;
        }

        _Storage_t& _Storage = _Mypair._Val1;
        switch (_Storage._Size) {
        case 0: // allocate the first node, discard prediction
            _Storage._First = ::new (_Raw) _Node_t(_STD move(_Val));
            _Storage._Last  = _Storage._First;
            break;
        case 1: // allocate the second node, use prediction
        {
            _Node_t*& _First = _Storage._First;
            _Node_t*& _Last  = _Storage._Last;
            if (_Pred(_Val, _First->_Value)) { // insert before the first node
                _First->_Prev        = ::new (_Raw) _Node_t(_STD move(_Val));
                _First->_Prev->_Next = _First;
                _Last                = _First;
                _First               = _First->_Prev;
                _First->_Next        = _Last;
            } else { // insert after the first node
                _First->_Next        = ::new (_Raw) _Node_t(_STD move(_Val));
                _First->_Next->_Prev = _First;
                _Last                = _First->_Next;
                _Last->_Prev         = _First;
            }

            break;
        }
        default: // allocate the next node, use prediction
        {
            _Node_t* _Node = _Storage._Last;
            while (_Node->_Prev && _Pred(_Val, _Node->_Value)) { // find the matching node
                _Node = _Node->_Prev;
            }

            if (_Pred(_Val, _Node->_Value)) { // insert before the selected node
                if (_Node->_Prev) { // insert a new node between a pair of existing nodes
                    _Node_t* const _Temp = _Node->_Prev;
                    _Node->_Prev         = ::new (_Raw) _Node_t(_STD move(_Val));
                    _Node->_Prev->_Next  = _Node;
                    _Node->_Prev->_Prev  = _Temp;
                } else { // allocate a new node
                    // Note: If _Node->_Prev is a null-pointer, it means that _Node is the first node.
                    _Node->_Prev        = ::new (_Raw) _Node_t(_STD move(_Val));
                    _Node->_Prev->_Next = _Node;
                    _Storage._First     = _Node->_Prev;
                }
            } else { // insert after the selected node
                if (_Node->_Next) { // insert a new node between a pair of existing nodes
                    _Node_t* const _Temp       = _Node->_Next;
                    _Node->_Next               = ::new (_Raw) _Node_t(_STD move(_Val));
                    _Node->_Next->_Next        = _Temp;
                    _Node->_Next->_Next->_Prev = _Node->_Next;
                    _Node->_Next->_Prev        = _Node;
                } else { // allocate a new node
                    // Note: If _Node->_Next is a null-pointer, it means that _Node is the last node.
                    _Node->_Next        = ::new (_Raw) _Node_t(_STD move(_Val));
                    _Node->_Next->_Prev = _Node;
                    _Storage._Last      = _Node->_Next;
                }
            }

            break;
        }
        }

        ++_Storage._Size;
        return true;
    }

    _Ty _Pop() noexcept {
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

    shared_queue() noexcept : _Mycont(), _Mylock() {}

    shared_queue(shared_queue&& _Other) noexcept : _Mycont(), _Mylock() {
        _Released_storage _Other_storage = _Other._Release();
        _Assign(_Other_storage);
    }

    ~shared_queue() noexcept {}

    shared_queue& operator=(shared_queue&& _Other) noexcept {
        if (this != _TPLMGR addressof(_Other)) {
            _Released_storage _Other_storage = _Other._Release();
            _Assign(_Other_storage);
        }

        return *this;
    }

    shared_queue(const shared_queue&) = delete;
    shared_queue& operator=(const shared_queue&) = delete;

    void clear() noexcept {
        lock_guard _Guard(_Mylock);
        _Mycont._Clear();
    }

    bool empty() const noexcept {
        shared_lock_guard _Guard(_Mylock);
        return _Mycont._Empty();
    }

    bool full() const noexcept {
        shared_lock_guard _Guard(_Mylock);
        return _Mycont._Full();
    }

    size_type size() const noexcept {
        shared_lock_guard _Guard(_Mylock);
        return _Mycont._Size();
    }

    size_type max_size() const noexcept {
        shared_lock_guard _Guard(_Mylock);
        return _Mycont._Max_size();
    }

    value_type front() const noexcept {
        shared_lock_guard _Guard(_Mylock);
        return _Mycont._Front();
    }
    
    _NODISCARD_ATTR bool push(const value_type& _Val) noexcept {
        lock_guard _Guard(_Mylock);
        return _Mycont._Push(_Val);
    }

    _NODISCARD_ATTR bool push(value_type&& _Val) noexcept {
        lock_guard _Guard(_Mylock);
        return _Mycont._Push(_STD move(_Val));
    }

    template <class _Pr>
    _NODISCARD_ATTR bool push_with_priority(const value_type& _Val, _Pr _Pred) noexcept {
        lock_guard _Guard(_Mylock);
        return _Mycont._Push_with_priority(_Val, _Pred);
    }

    template <class _Pr>
    _NODISCARD_ATTR bool push_with_priority(value_type&& _Val, _Pr _Pred) noexcept {
        lock_guard _Guard(_Mylock);
        return _Mycont._Push_with_priority(_STD move(_Val), _Pred);
    }

    value_type pop() noexcept {
        lock_guard _Guard(_Mylock);
        return _Mycont._Pop();
    }

private:
    using _Released_storage = typename _Container::_Released_storage;

    _NODISCARD_ATTR _Released_storage _Release() noexcept {
        lock_guard _Guard(_Mylock);
        return _Mycont._Release();
    }

    void _Assign(_Released_storage& _Storage) noexcept {
        lock_guard _Guard(_Mylock);
        _Mycont._Assign(_Storage._First, _Storage._Last, _Storage._Size);
    }

    _Container _Mycont;
    mutable shared_lock _Mylock;
};
_TPLMGR_END

#endif // _TPLMGR_PREPROCESSOR_GUARD
#endif // _TPLMGR_SHARED_QUEUE_HPP_