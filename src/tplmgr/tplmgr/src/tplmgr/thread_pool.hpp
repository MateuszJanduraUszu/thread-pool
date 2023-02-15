// thread_pool.hpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef _TPLMGR_THREAD_POOL_HPP_
#define _TPLMGR_THREAD_POOL_HPP_
#include <tplmgr/core.hpp>
#if _TPLMGR_PREPROCESSOR_GUARD
#include <tplmgr/allocator.hpp>
#include <tplmgr/thread.hpp>
#include <tplmgr/utils.hpp>
#include <cstddef>
#include <type_traits>
#include <utility>

_TPLMGR_BEGIN
// STRUCT _Thread_list_node
struct _Thread_list_node {
    _Thread_list_node() noexcept;
    ~_Thread_list_node() noexcept;

    _Thread_list_node* _Next; // pointer to the next node
    _Thread_list_node* _Prev; // pointer to the previous node
    thread _Thread;
};

// STRUCT _Thread_list_storage
struct _Thread_list_storage {
    _Thread_list_storage() noexcept;
    ~_Thread_list_storage() noexcept;

    _Thread_list_node* _First; // pointer to the first node
    _Thread_list_node* _Last; // pointer to the last node
    size_t _Size; // number of nodes
};

// CLASS _Thread_list
class _Thread_list { // base container for the thread objects
public:
    _Thread_list() noexcept;
    ~_Thread_list() noexcept;

    _Thread_list(const _Thread_list&) = delete;
    _Thread_list& operator=(const _Thread_list&) = delete;

    explicit _Thread_list(const size_t _Count) noexcept;

    // returns the number of threads
    _NODISCARD const size_t _Size() const noexcept;

    // tries to hire _Count additional threads
    _NODISCARD bool _Grow(size_t _Count) noexcept;

    // tries to dismiss _Count existing threads
    _NODISCARD bool _Reduce(size_t _Count) noexcept;

    // dismisses all threads
    void _Release() noexcept;

    // returns a pointer to the selected thread
    _NODISCARD thread* _Select_thread(size_t _Which) noexcept;

    // returns a pointer to the first waiting thread
    _NODISCARD thread* _Select_any_waiting_thread() noexcept;

    // returns a pointer to the thread with the fewest pending threads
    _NODISCARD thread* _Select_thread_with_fewest_pending_tasks() noexcept;

    template <class _Fn, class... _Types>
    void _For_each_thread(_Fn&& _Func, _Types&&... _Args) noexcept {
        _Thread_list_storage& _Storage = _Mypair._Val1;
        if (_Storage._Size > 0) {
            _Thread_list_node* _Node = _Storage._First;
            while (_Node) {
                (void) _Func(_Node->_Thread, _STD forward<_Types>(_Args)...);
                _Node = _Node->_Next;
            }
        }
    }

private:
    using _Alloc = allocator<void>;

    // allocates a thread list node
    _NODISCARD static bool _Allocate_node(_Thread_list_node** const _Node, _Alloc& _Al) noexcept;

    // deallocates one node
    void _Free_node(_Thread_list_node* _Node) noexcept;

    // tries to reduce _Count waiting threads
    void _Reduce_waiting_threads(size_t _Count, size_t& _Reduced) noexcept;

    _Ebco_pair<_Thread_list_storage, _Alloc> _Mypair;
};

// CLASS thread_pool
class _TPLMGR_API thread_pool {
public:
    explicit thread_pool(const size_t _Threads) noexcept;
    ~thread_pool() noexcept;

    thread_pool() = delete;
    thread_pool(const thread_pool&) = delete;
    thread_pool& operator=(const thread_pool&) = delete;

    // returns the number of threads
    _NODISCARD size_t threads() const noexcept;

    // checks if the thread-pool is still open
    _NODISCARD bool is_open() const noexcept;

    // checks if the thread-pool is waiting
    _NODISCARD bool is_waiting() const noexcept;

    // checks if the thread-pool is working
    _NODISCARD bool is_working() const noexcept;

    // closes the thread-pool
    void close() noexcept;

    struct statistics {
        size_t waiting_threads;
        size_t working_threads;
        size_t pending_tasks;
    };

    // collects the thread-pool statistics
    _NODISCARD statistics collect_statistics() noexcept;

    // tries to hire _Count new threads to the thread-pool
    _NODISCARD bool increase_threads(const size_t _Count) noexcept;

    // tries to dismiss _Count threads from the thread-pool
    _NODISCARD bool decrease_threads(const size_t _Count) noexcept;

    // tries to schedule a new task
    _NODISCARD bool schedule_task(const thread::task _Task, void* const _Data) noexcept;

    // tries to schedule a new task (provides a hint about priority)
    _NODISCARD bool schedule_task(
        const thread::task _Task, void* const _Data, const task_priority _Priority) noexcept;

    // tries to suspend the thread-pool
    _NODISCARD bool suspend() noexcept;

    // tries to resume the thread-pool
    _NODISCARD bool resume() noexcept;

private:
    enum _Internal_state : unsigned char {
        _Closed,
        _Waiting,
        _Working
    };

    // returns a pointer to the best thread for task scheduling
    _NODISCARD thread* _Select_ideal_thread() noexcept;

    _Thread_list _Mylist;
    _Internal_state _Mystate;
};
_TPLMGR_END

#endif // _TPLMGR_PREPROCESSOR_GUARD
#endif // _TPLMGR_THREAD_POOL_HPP_