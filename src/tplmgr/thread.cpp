// thread.cpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#include <tplmgr/tplmgr_pch.hpp>
#include <tplmgr/thread.hpp>
#if _TPLMGR_PREPROCESSOR_GUARD

_TPLMGR_BEGIN
// FUNCTION _Hardware_concurrency
size_t _Hardware_concurrency() noexcept {
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
_NODISCARD_ATTR bool _Suspend_thread(void* const _Handle) noexcept {
    return ::SuspendThread(_Handle) != static_cast<unsigned long>(-1);
}

// FUNCTION _Resume_thread
_NODISCARD_ATTR bool _Resume_thread(void* const _Handle) noexcept {
    return ::ResumeThread(_Handle) != static_cast<unsigned long>(-1);
}

// FUNCTION _Thread_cache constructors
_Thread_cache::_Thread_cache(_Thread_cache&& _Other) noexcept
    : _State(_Other._State.exchange(thread_state::terminated)), _Queue(_STD move(_Other._Queue)) {}

_Thread_cache::_Thread_cache(const thread_state _State) noexcept
    : _State(_State), _Queue() {}

// FUNCTION _Thread_cache::operator=
_Thread_cache& _Thread_cache::operator=(_Thread_cache&& _Other) noexcept {
    if (this != _TPLMGR addressof(_Other)) {
        _State.store(_Other._State.exchange(thread_state::terminated), _STD memory_order_relaxed);
        _Queue = _STD move(_Other._Queue);
    }

    return *this;
}

// FUNCTION thread constructors/destructor
thread::thread() noexcept : _Myid(0), _Mycache(thread_state::waiting), _Mystack() {
    _Attach();
}

thread::thread(thread&& _Other) noexcept
    : _Myimpl(_TPLMGR exchange(_Other._Myimpl, nullptr)), _Myid(_TPLMGR exchange(_Other._Myid, 0)),
    _Mycache(_STD move(_Other._Mycache)), _Mystack(_STD move(_Other._Mystack)) {}

thread::thread(const task _Task, void* const _Data) noexcept
    : _Myimpl(nullptr), _Myid(0), _Mycache(thread_state::working), _Mystack() {
    if (_Mycache._Queue.push(_Thread_task{_Task, _Data})) { // try schedule an immediate task
        if (!_Attach()) {
            _Mycache._Queue.clear();
        }
    }
}

thread::~thread() noexcept {
    (void) terminate(); // wait for the current task to finish, discard the rest
}

// FUNCTION thread::operator=
thread& thread::operator=(thread&& _Other) noexcept {
    if (this != _TPLMGR addressof(_Other)) {
        _Myimpl        = _Other._Myimpl;
        _Myid          = _Other._Myid;
        _Mycache       = _STD move(_Other._Mycache);
        _Mystack       = _STD move(_Other._Mystack);
        _Other._Myimpl = nullptr;
        _Other._Myid   = 0;
    }

    return *this;
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
#if _HAS_CXX23_FEATURES
            _STD unreachable();
#endif // _HAS_CXX23_FEATURES
            break;
        }
    }

    return 0;
}

// FUNCTION thread::_Invoke_callbacks
void thread::_Invoke_callbacks(const event _Event) noexcept {
    if (!_Mystack._Empty()) {
        size_t _Size = _Mystack._Size();
        auto* _Node  = _Mystack._Bottom();
        while (_Size-- > 0 && _Node) {
            _Event_callback& _Callback = _Node->_Value;
            if (_Callback._Event == _Event) {
                (*_Callback._Func)(_Callback._Event, _Callback._Data);
            }

            _Node = _Node->_Next;
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
    _Mystack._Clear(); // clear event callbacks
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

// FUNCTION thread::_Has_higher_priority::operator()
bool thread::_Has_higher_priority::operator()(
    const _Thread_task& _Left, const _Thread_task& _Right) const noexcept {
    return static_cast<uint8_t>(_Left._Priority) > static_cast<uint8_t>(_Right._Priority);
}

// FUNCTION thread::hardware_concurrency
size_t thread::hardware_concurrency() noexcept {
    static const size_t _Count = _Hardware_concurrency();
    return _Count;
}

// FUNCTION thread::register_event_callback
_NODISCARD_ATTR bool thread::register_event_callback(
    const event _Event, const event_callback _Callback, void* const _Data) noexcept {
    return _Mystack._Push(_Event_callback{_Event, _Callback, _Data});
}

// FUNCTION thread::joinable
bool thread::joinable() const noexcept {
    return state() != thread_state::terminated;
}

// FUNCTION thread::get_id
const thread::id thread::get_id() const noexcept {
    return _Myid;
}

// FUNCTION thread::native_handle
const thread::native_handle_type thread::native_handle() const noexcept {
    return _Myimpl;
}

// FUNCTION thread::state
thread_state thread::state() const noexcept {
    return _Mycache._State.load(_STD memory_order_relaxed);
}

// FUNCTION thread::pending_tasks
size_t thread::pending_tasks() const noexcept {
    return _Mycache._Queue.size();
}

// FUNCTION thread::cancel_all_pending_tasks
void thread::cancel_all_pending_tasks() noexcept {
    _Mycache._Queue.clear();
}

// FUNCTION thread::schedule_task
_NODISCARD_ATTR bool thread::schedule_task(const task _Task, void* const _Data) noexcept {
    const thread_state _State = state();
    if (_State == thread_state::terminated || _Mycache._Queue.full()) {
        return false;
    }

    if (!_Mycache._Queue.push_with_priority(
        _Thread_task{_Task, _Data, task_priority::normal}, _Has_higher_priority{})) {
        return false;
    }
    
    if (_State != thread_state::working) { // notify waiting thread
        (void) resume();
    }

    return true;
}

_NODISCARD_ATTR bool thread::schedule_task(
    const task _Task, void* const _Data, const task_priority _Priority) noexcept {
    const thread_state _State = state();
    if (_State == thread_state::terminated || _Mycache._Queue.full()) {
        return false;
    }

    if (_Priority == task_priority::idle) { // always at the end
        if (!_Mycache._Queue.push(_Thread_task{_Task, _Data, task_priority::idle})) {
            return false;
        }
    } else { // position not specified
        if (!_Mycache._Queue.push_with_priority(
            _Thread_task{_Task, _Data, _Priority}, _Has_higher_priority{})) {
            return false;
        }
    }

    if (_State != thread_state::working) { // notify waiting thread
        (void) resume();
    }

    return true;
}

// FUNCTION thread::terminate
_NODISCARD_ATTR bool thread::terminate(const bool _Wait) noexcept {
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
_NODISCARD_ATTR bool thread::suspend() noexcept {
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
_NODISCARD_ATTR bool thread::resume() noexcept {
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