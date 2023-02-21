// thread.hpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef _TPLMGR_THREAD_HPP_
#define _TPLMGR_THREAD_HPP_
#include <tplmgr/core.hpp>
#if _TPLMGR_PREPROCESSOR_GUARD
#include <tplmgr/shared_queue.hpp>
#include <tplmgr/stack.hpp>
#include <tplmgr/utils.hpp>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <handleapi.h>
#include <processthreadsapi.h>
#include <sysinfoapi.h>
#include <utility>

_TPLMGR_BEGIN
// STD types
using _STD atomic;

// FUNCTION _Hardware_concurrency
extern size_t _Hardware_concurrency() noexcept;

// FUNCTION _Wait_for_thread
extern void _Wait_for_thread(void* const _Handle) noexcept;

// FUNCTION _Terminate_current_thread
extern void _Terminate_current_thread() noexcept;

// FUNCTION _Suspend_current_thread
extern void _Suspend_current_thread() noexcept;

// FUNCTION _Suspend_thread
extern _NODISCARD_ATTR bool _Suspend_thread(void* const _Handle) noexcept;

// FUNCTION _Resume_thread
extern _NODISCARD_ATTR bool _Resume_thread(void* const _Handle) noexcept;

// ENUM CLASS thread_state
enum class thread_state : unsigned char {
    terminated,
    waiting,
    working
};

// ENUM CLASS task_priority
enum class task_priority : unsigned char {
    lowest,
    low,
    normal,
    high,
    highest
};

// STRUCT _Thread_task
struct _Thread_task {
    using _Fn = void(__STDCALL_OR_CDECL*)(void*);

    _Fn _Func;
    void* _Data;
    task_priority _Priority;
};

// STRUCT _Thread_cache
struct _Thread_cache { // internal thread cache
    explicit _Thread_cache(const thread_state _State) noexcept;

    atomic<thread_state> _State;
    shared_queue<_Thread_task> _Queue;
};

// CLASS thread
class _TPLMGR_API thread { // non-copyable and non-movable thread object
public:
    using native_handle_type = void*;
    using id                 = unsigned int;
    using task               = _Thread_task::_Fn;

    enum event : unsigned char {
        suspend_event,
        resume_event,
        terminate_event
    };

    using event_callback = void(__STDCALL_OR_CDECL*)(const event, void* const);

    thread() noexcept;
    ~thread() noexcept;

    thread(const thread&) = delete;
    thread& operator=(const thread&) = delete;

    explicit thread(const task _Task, void* const _Data) noexcept;

    // returns the max number of threads
    static size_t hardware_concurrency() noexcept;

    // registers a new callback
    _NODISCARD_ATTR bool register_event_callback(
        const event _Event, const event_callback _Callback, void* const _Data);

    // checks if the thread is joinable
    bool joinable() const noexcept;

    // returns thread ID
    const id get_id() const noexcept;

    // returns thread native handle
    const native_handle_type native_handle() const noexcept;

    // returns the current state
    thread_state state() const noexcept;

    // returns the number of pending tasks
    size_t pending_tasks() const noexcept;

    // tries to schedule a new task
    _NODISCARD_ATTR bool schedule_task(const task _Task, void* const _Data) noexcept;

    // tries to schedule a new task (provides a hint about priority)
    _NODISCARD_ATTR bool schedule_task(
        const task _Task, void* const _Data, const task_priority _Priority) noexcept;

    // tries to terminate the thread (optionally wait)
    _NODISCARD_ATTR bool terminate(const bool _Wait = true) noexcept;

    // tries to suspend the thread
    _NODISCARD_ATTR bool suspend() noexcept;

    // tries to resume the thread
    _NODISCARD_ATTR bool resume() noexcept;

private:
    // manages pending tasks
    static unsigned long __stdcall _Schedule_handler(void* const _Data) noexcept;

    // invokes registered event callbacks
    void _Invoke_callbacks(const event _Event) noexcept;

    // changes the thread state
    void _Set_state(const thread_state _New_state) noexcept;

    // clears whole thread data
    void _Erase_data() noexcept;

    // tries to attach a new thread
    bool _Attach() noexcept;

    // prepares thread termination
    void _Tidy() noexcept;

    struct _Has_higher_priority {
        // checks if the task has higher priority
        bool operator()(const _Thread_task& _Left, const _Thread_task& _Right) const noexcept;
    };

    struct _Event_callback {
        event _Event;
        event_callback _Func;
        void* _Data;
    };
    
    native_handle_type _Myimpl;
    id _Myid;
    _Thread_cache _Mycache;
    _Stack<_Event_callback> _Mystack;
};
_TPLMGR_END

#endif // _TPLMGR_PREPROCESSOR_GUARD
#endif // _TPLMGR_THREAD_HPP_