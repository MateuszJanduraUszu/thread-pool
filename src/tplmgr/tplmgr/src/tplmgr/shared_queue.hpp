// shared_queue.hpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef _TPLMGR_SHARED_QUEUE_HPP_
#define _TPLMGR_SHARED_QUEUE_HPP_
#include <tplmgr/core.hpp>
#if _TPLMGR_PREPROCESSOR_GUARD
#include <tplmgr/shared_lock.hpp>
#include <list>
#include <utility>

_TPLMGR_BEGIN
// STD types
using _STD list;

// CLASS TEMPLATE shared_queue
template <class _Ty>
class shared_queue { // queue that can be shared between multiple threads
private:
    using _Container = list<_Ty>;

public:
    using value_type      = typename _Container::value_type;
    using size_type       = typename _Container::size_type;
    using difference_type = typename _Container::difference_type;

    shared_queue() noexcept : _Mycont(), _Mylock() {}

    ~shared_queue() noexcept {}

    shared_queue(const shared_queue&) = delete;
    shared_queue& operator=(const shared_queue&) = delete;

    void clear() noexcept {
        lock_guard _Guard(_Mylock);
        _Mycont.clear();
    }

    _NODISCARD bool empty() const noexcept {
        shared_lock_guard _Guard(_Mylock);
        return _Mycont.empty();
    }

    _NODISCARD bool full() const noexcept {
        shared_lock_guard _Guard(_Mylock);
        return _Mycont.size() == _Mycont.max_size();
    }

    _NODISCARD size_type size() const noexcept {
        shared_lock_guard _Guard(_Mylock);
        return _Mycont.size();
    }

    _NODISCARD size_type max_size() const noexcept {
        shared_lock_guard _Guard(_Mylock);
        return _Mycont.max_size();
    }

    _NODISCARD value_type top() const noexcept {
        shared_lock_guard _Guard(_Mylock);
        return !_Mycont.empty() ? _Mycont.front() : value_type{};
    }
    
    void push(const value_type& _Val) {
        lock_guard _Guard(_Mylock);
        if (_Mycont.size() < _Mycont.max_size()) {
            _Mycont.push_back(_Val);
        }
    }

    void push(value_type&& _Val) {
        lock_guard _Guard(_Mylock);
        if (_Mycont.size() < _Mycont.max_size()) {
            _Mycont.push_back(_STD move(_Val));
        }
    }

    value_type pop() noexcept {
        lock_guard _Guard(_Mylock);
        if (_Mycont.empty()) {
            return value_type{};
        }

        const value_type _Val = static_cast<value_type&&>(_Mycont.front());
        _Mycont.pop_front();
        return _Val;
    }

private:
    _Container _Mycont;
    mutable shared_lock _Mylock;
};
_TPLMGR_END

#endif // _TPLMGR_PREPROCESSOR_GUARD
#endif // _TPLMGR_SHARED_QUEUE_HPP_