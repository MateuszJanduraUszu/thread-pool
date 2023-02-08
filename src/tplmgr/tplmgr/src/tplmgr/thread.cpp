// thread.cpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#include <build/tplmgr_pch.hpp>
#include <tplmgr/thread.hpp>
#if _TPLMGR_PREPROCESSOR_GUARD

_TPLMGR_BEGIN
// FUNCTION _Hardware_concurrency
_NODISCARD size_t _Hardware_concurrency() noexcept {
    SYSTEM_INFO _Info;
    ::GetSystemInfo(_TPLMGR addressof(_Info));
    return static_cast<size_t>(_Info.dwNumberOfProcessors);
}

// FUNCTION _Wait_for_thread
void _Wait_for_thread(void* const _Handle) noexcept {
    ::WaitForSingleObject(_Handle, 0xFFFF'FFFF); // infinite timeout
}

// FUNCTION _Terminate_current_thread
void _Terminate_current_thread() noexcept {
    ::ExitThread(0);
}

// FUNCTION _Suspend_current_thread
void _Suspend_current_thread() noexcept {
    ::SuspendThread(::GetCurrentThread());
}

// FUNCTION _Suspend_thread
_NODISCARD bool _Suspend_thread(void* const _Handle) noexcept {
    return ::SuspendThread(_Handle) != static_cast<unsigned long>(-1);
}

// FUNCTION _Resume_thread
_NODISCARD bool _Resume_thread(void* const _Handle) noexcept {
    return ::ResumeThread(_Handle) != static_cast<unsigned long>(-1);
}

// FUNCTION _Thread_cache copy constructor
_Thread_cache::_Thread_cache(const thread_state _State) noexcept
    : _State(_State), _Queue() {}

// FUNCTION thread constructors/destructor
thread::thread() noexcept : _Myid(0), _Mycache(thread_state::waiting), _Mycbs() {
    _Attach();
}

thread::thread(const task _Task, void* const _Data) noexcept
    : _Myid(0), _Mycache(thread_state::working), _Mycbs() {
    _Mycache._Queue.push(_Thread_task{_Task, _Data}); // schedule an immediate task
    if (!_Attach()) {
        _Mycache._Queue.clear();
    }
}

thread::~thread() noexcept {
    (void) terminate(); // wait for the current task to finish, reject the rest
}

// FUNCTION thread::_Schedule_handler
unsigned long __stdcall thread::_Schedule_handler(void* const _Data) noexcept {
    _Thread_cache* const _Cache = static_cast<_Thread_cache*>(_Data);
    for (;;) {
        switch (_Cache->_State.load(_STD memory_order_relaxed)) {
        case thread_state::terminated: // try terminate itself
            _Terminate_current_thread();
            break;
        case thread_state::waiting: // try suspend itself
            _Suspend_current_thread();
            break;
        case thread_state::working: // try perform next task
            if (!_Cache->_Queue.empty()) {
                const _Thread_task& _Task = _Cache->_Queue.pop();
                (*_Task._Func)(_Task._Data);
            } else { // nothing to do, wait for any task
                _Cache->_State.store(thread_state::waiting, _STD memory_order_relaxed);
            }

            break;
        default:
#if _HAS_CXX23
            _STD unreachable();
#endif // _HAS_CXX23
            break;
        }
    }
}

// FUNCTION thread::_Invoke_callbacks
void thread::_Invoke_callbacks(const event _Event) noexcept {
    for (const _Event_callback& _Cb : _Mycbs) {
        if (_Cb._Event == _Event) {
            (*_Cb._Func)(_Cb._Event, _Cb._Data);
        }
    }
}

// FUNCTION thread::_Set_state
void thread::_Set_state(const thread_state _New_state) noexcept {
    _Mycache._State.store(_New_state, _STD memory_order_relaxed);
}

// FUNCTION thread::_Erase_data
void thread::_Erase_data() noexcept {
    _Set_state(thread_state::terminated);
    _Mycache._Queue.clear(); // clear task queue
    _Mycbs.clear(); // clear event callbacks
    ::CloseHandle(_Myimpl); // close thread handle
    _Myimpl = nullptr;
    _Myid   = 0;
}

// FUNCTION thread::_Attach
bool thread::_Attach() noexcept {
    _Myimpl = ::CreateThread(nullptr, 0, _Schedule_handler,
        _TPLMGR addressof(_Mycache), 0, reinterpret_cast<unsigned long*>(&_Myid));
    if (_Myimpl) { // attached a new thread
        return true;
    } else { // failed to attach a new thread
        _Myid = 0;
        _Set_state(thread_state::terminated);
        return false;
    }
}

// FUNCTION thread::_Tidy
void thread::_Tidy() noexcept {
    if (state() != thread_state::waiting) {
        (void) suspend(); // must be suspended
    }

    _Set_state(thread_state::terminated);
    _Invoke_callbacks(terminate_event);
    
    // Note: We cannot use resume() here, because it will set the state to thread_state::working.
    //       The thread would continue doing its task.
    (void) _Resume_thread(_Myimpl);
}

// FUNCTION thread::hardware_concurrency
_NODISCARD size_t thread::hardware_concurrency() noexcept {
    static size_t _Count = _Hardware_concurrency();
    return _Count;
}

// FUNCTION thread::register_event_callback
void thread::register_event_callback(
    const event _Event, const event_callback _Callback, void* const _Data) {
    _Mycbs.push_back(_Event_callback{_Event, _Callback, _Data});
}

// FUNCTION thread::joinable
_NODISCARD bool thread::joinable() const noexcept {
    return state() != thread_state::terminated;
}

// FUNCTION thread::get_id
_NODISCARD const thread::id thread::get_id() const noexcept {
    return _Myid;
}

// FUNCTION thread::native_handle
_NODISCARD const thread::native_handle_type thread::native_handle() const noexcept {
    return _Myimpl;
}

// FUNCTION thread::state
_NODISCARD thread_state thread::state() const noexcept {
    return _Mycache._State.load(_STD memory_order_relaxed);
}

// FUNCTION thread::pending_tasks
_NODISCARD size_t thread::pending_tasks() const noexcept {
    return _Mycache._Queue.size();
}

// FUNCTION thread::schedule_task
_NODISCARD bool thread::schedule_task(const task _Task, void* const _Data) noexcept {
    const thread_state _State = state();
    if (_State == thread_state::terminated || _Mycache._Queue.full()) {
        return false;
    }

    _Mycache._Queue.push(_Thread_task{_Task, _Data});
    if (_State != thread_state::working) { // notify waiting thread
        (void) resume();
    }

    return true;
}

// FUNCTION thread::terminate
_NODISCARD bool thread::terminate(const bool _Wait) noexcept {
    if (!joinable()) {
        return false;
    }

    _Tidy(); // force the thread to terminate itself
    if (_Wait) { // wait for the thread
        _Wait_for_thread(_Myimpl);
    }

    _Erase_data();
    return true;
}

// FUNCTION thread::suspend
_NODISCARD bool thread::suspend() noexcept {
    if (state() != thread_state::working) { // must be working
        return false;
    }

    _Invoke_callbacks(suspend_event);
    _Set_state(thread_state::waiting);
    if (_Suspend_thread(_Myimpl)) {
        return true;
    } else { // failed to suspend the thread, restore an old state
        _Set_state(thread_state::working);
        return false;
    }
}

// FUNCTION thread::resume
_NODISCARD bool thread::resume() noexcept {
    if (state() != thread_state::waiting) { // must be waiting
        return false;
    }

    _Invoke_callbacks(resume_event);
    _Set_state(thread_state::working);
    if (_Resume_thread(_Myimpl)) {
        return true;
    } else { // failed to resume the thread, restore an old state
        _Set_state(thread_state::waiting);
        return false;
    }
}
_TPLMGR_END

#endif // _TPLMGR_PREPROCESSOR_GUARD