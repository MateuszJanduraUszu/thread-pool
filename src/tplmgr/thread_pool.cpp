// thread_pool.cpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#include <tplmgr/tplmgr_pch.hpp>
#include <tplmgr/thread_pool.hpp>
#if _TPLMGR_PREPROCESSOR_GUARD

_TPLMGR_BEGIN
// FUNCTION _Thread_list_node constructor/destructor
_Thread_list_node::_Thread_list_node() noexcept : _Next(nullptr), _Prev(nullptr), _Thread() {}

_Thread_list_node::~_Thread_list_node() noexcept {}

// FUNCTION _Thread_list_storage constructor/destructor
_Thread_list_storage::_Thread_list_storage() noexcept : _Head(nullptr), _Tail(nullptr), _Size(0) {}

_Thread_list_storage::~_Thread_list_storage() noexcept {}

// FUNCTION _Thread_list constructors/destructor
_Thread_list::_Thread_list() noexcept : _Mypair(_Ebco_default_init{}) {}

_Thread_list::_Thread_list(const size_t _Size) noexcept : _Mypair(_Ebco_default_init{}) {
    (void) _Grow(_Size);
}

_Thread_list::~_Thread_list() noexcept {
    _Release();
}

// FUNCTION _Thread_list::_Allocate_node
_NODISCARD_ATTR bool _Thread_list::_Allocate_node(_Thread_list_node** const _Node, _Alloc& _Al) noexcept {
    void* const _Raw = _Al.allocate(sizeof(_Thread_list_node));
    if (!_Raw) { // allocation failed
        return false;
    }

    *_Node = ::new (_Raw) _Thread_list_node;
    return true;
}

// FUNCTION _Thread_list::_Free_node
void _Thread_list::_Free_node(_Thread_list_node* _Node) noexcept {
    _Thread_list_storage& _Storage = _Mypair._Val1;
    _Alloc& _Al                    = _Mypair._Get_val2();
    if (_Node == _Storage._Head) { // free the first node
        if (_Node->_Next) {
            _Node->_Next->_Prev = nullptr;
            _Storage._Head      = _Node->_Next;
        }
    } else if (_Node == _Storage._Tail) { // free the last node
        _Node->_Prev->_Next = nullptr;
        _Storage._Tail      = _Node->_Prev;
    } else { // free the inner node
        _Node->_Prev->_Next = _Node->_Next;
        _Node->_Next->_Prev = _Node->_Prev;
    }

    _Node->~_Thread_list_node();
    _Al.deallocate(_Node, sizeof(_Thread_list_node));
    --_Storage._Size;
}

// FUNCTION _Thread_list::_Reduce_waiting_threads
void _Thread_list::_Reduce_waiting_threads(size_t& _Count) noexcept {
    _Thread_list_storage& _Storage = _Mypair._Val1;
    _Thread_list_node* _Node       = _Storage._Head;
    _Thread_list_node* _Next;
    while (_Node && _Count > 0) {
        _Next = _Node->_Next;
        if (_Node->_Thread.state() == thread_state::waiting) {
            _Free_node(_Node);
            --_Count;
        }

        _Node = _Next;
    }
}

// FUNCTION _Thread_list::_Size
const size_t _Thread_list::_Size() const noexcept {
    return _Mypair._Val1._Size;
}

// FUNCTION _Thread_list::_Grow
_NODISCARD_ATTR bool _Thread_list::_Grow(size_t _Count) noexcept {
    if (_Count == 0) { // no growth, do nothing
        return true;
    }

    _Thread_list_storage& _Storage = _Mypair._Val1;
    _Alloc& _Al                    = _Mypair._Get_val2();
    if (_Storage._Size == 0) { // allocate the first node
        if (!_Allocate_node(_TPLMGR addressof(_Storage._Head), _Al)) {
            return false;
        }

        _Storage._Tail = _Storage._Head;
        ++_Storage._Size;
        --_Count; // one node is already allocated
    }

    while (_Count-- > 0) {
        if (!_Allocate_node(_TPLMGR addressof(_Storage._Tail->_Next), _Al)) {
            return false;
        }

        _Storage._Tail->_Next->_Prev = _Storage._Tail;
        _Storage._Tail               = _Storage._Tail->_Next;
        ++_Storage._Size;
    }

    return true;
}

// FUNCTION _Thread_list::_Reduce
_NODISCARD_ATTR bool _Thread_list::_Reduce(size_t _Count) noexcept {
    if (_Count == 0) { // no reduction, do nothing
        return true;
    }

    _Thread_list_storage& _Storage = _Mypair._Val1;
    if (_Count > _Storage._Size) { // not enough threads
        return false;
    }

    if (_Count == _Storage._Size) { // reduce to 0
        _Release();
        return true;
    }

    _Reduce_waiting_threads(_Count);
    if (_Count == 0) { // no need to reduce more threads
        return true;
    }

    _Alloc& _Al     = _Mypair._Get_val2();
    _Storage._Size -= _Count; // subtract once
    _Thread_list_node* _Node;
    while (_Count-- > 0) {
        _Node               = _Storage._Tail;
        _Node->_Prev->_Next = nullptr;
        _Storage._Tail      = _Node->_Prev;
        _Node->~_Thread_list_node();
        _Al.deallocate(_Node, sizeof(_Thread_list_node));
    }

    return true;
}

// FUNCTION _Thread_list::_Release
void _Thread_list::_Release() noexcept {
    _Thread_list_storage& _Storage = _Mypair._Val1;
    _Alloc& _Al                    = _Mypair._Get_val2();
    _Thread_list_node* _Next;
    for (_Thread_list_node* _Node = _Storage._Head; _Node != nullptr; _Node = _Next) {
        _Next = _Node->_Next;
        _Node->~_Thread_list_node();
        _Al.deallocate(_Node, sizeof(_Thread_list_node));
    }

    _Storage._Head = nullptr;
    _Storage._Tail = nullptr;
    _Storage._Size = 0;
}

// FUNCTION _Thread_list::_Select_thread
thread* _Thread_list::_Select_thread(size_t _Which) noexcept {
    _Thread_list_storage& _Storage = _Mypair._Val1;
    if (_Which >= _Storage._Size) { // not enough threads
        return nullptr;
    }

    if (_Which == 0) { // select the first thread
        return _TPLMGR addressof(_Storage._Head->_Thread);
    } else if (_Which == _Storage._Size - 1) { // select the last thread
        return _TPLMGR addressof(_Storage._Tail->_Thread);
    } else { // select the inner thread
        _Thread_list_node* _Node = _Storage._Head;
        while (_Which-- > 0) {
            _Node = _Node->_Next;
        }

        return _TPLMGR addressof(_Node->_Thread);
    }
}

// FUNCTION _Thread_list::_Select_thread_by_id
thread* _Thread_list::_Select_thread_by_id(const thread::id _Id) noexcept {
    _Thread_list_storage& _Storage = _Mypair._Val1;
    if (_Storage._Size == 0) {
        return nullptr;
    }

    _Thread_list_node* _Node = _Storage._Head;
    while (_Node) {
        if (_Node->_Thread.get_id() == _Id) {
            return _TPLMGR addressof(_Node->_Thread);
        }

        _Node = _Node->_Next;
    }

    return nullptr;
}

// FUNCTION _Thread_list::_Select_any_waiting_thread
thread* _Thread_list::_Select_any_waiting_thread() noexcept {
    _Thread_list_storage& _Storage = _Mypair._Val1;
    if (_Storage._Size == 0) {
        return nullptr;
    }

    _Thread_list_node* _Node = _Storage._Head;
    while (_Node) {
        if (_Node->_Thread.state() == thread_state::waiting) {
            return _TPLMGR addressof(_Node->_Thread);
        }

        _Node = _Node->_Next;
    }

    return nullptr;
}

// FUNCTION _Thread_list::_Select_thread_with_fewest_pending_tasks
thread* _Thread_list::_Select_thread_with_fewest_pending_tasks() noexcept {
    _Thread_list_storage& _Storage = _Mypair._Val1;
    if (_Storage._Size == 0) {
        return nullptr;
    }

    thread* _Result = _TPLMGR addressof(_Storage._Head->_Thread); // first thread by default
    size_t _Count   = _Result->pending_tasks();
    for (_Thread_list_node* _Node = _Storage._Head->_Next; _Node != nullptr; _Node = _Node->_Next) {
        const size_t _Tasks = _Node->_Thread.pending_tasks();
        if (_Tasks < _Count) {
            _Result = _TPLMGR addressof(_Node->_Thread);
            _Count  = _Tasks;
        }
    }

    return _Result;
}

// FUNCTION thread_pool copy constructor/destructor
thread_pool::thread_pool(const size_t _Size) noexcept : _Mylist(
    (_STD max)(_Size, size_t{1})), _Mystate(_Working) {} // at least 1 thread must be active

thread_pool::~thread_pool() noexcept {
    close();
}

// FUNCTION thread_pool::_Select_ideal_thread
thread* thread_pool::_Select_ideal_thread() noexcept {
    switch (_Mystate) {
    case _Waiting: // all threads are waiting, choose the one with the fewest pending tasks
        return _Mylist._Select_thread_with_fewest_pending_tasks();
    case _Working: // some threads may be waiting, choose one if available
    {
        thread* _Thread = _Mylist._Select_any_waiting_thread();
        if (_Thread) { // some thread is waiting, select it
            return _Thread;
        } else { // no thread is waiting, choose the thread with the fewest pending tasks
            return _Mylist._Select_thread_with_fewest_pending_tasks();
        }
    }
    default:
#if _HAS_CXX23_FEATURES
        _STD unreachable();
#endif // _HAS_CXX23_FEATURES
        return nullptr;
    }
}

// FUNCTION thread_pool::threads
size_t thread_pool::threads() const noexcept {
    return _Mylist._Size();
}

// FUNCTION thread_pool::is_open
bool thread_pool::is_open() const noexcept {
    return _Mystate != _Closed;
}

// FUNCTION thread_pool::is_waiting
bool thread_pool::is_waiting() const noexcept {
    return _Mystate == _Waiting;
}

// FUNCTION thread_pool::is_working
bool thread_pool::is_working() const noexcept {
    return _Mystate == _Working;
}

// FUNCTION thread_pool::close
void thread_pool::close() noexcept {
    _Mystate = _Closed;
    _Mylist._Release();
}

// FUNCTION thread_pool::collect_statistics
_NODISCARD_ATTR thread_pool::statistics thread_pool::collect_statistics() noexcept {
    if (_Mystate == _Closed) { // must not be closed
        return statistics{0, 0, 0};
    }

    statistics _Result = {0, 0, 0};
    _Mylist._For_each_thread(
        [&_Result](thread& _Thread) mutable noexcept {
            _Result.pending_tasks += _Thread.pending_tasks();
            if (_Thread.state() == thread_state::waiting) {
                ++_Result.waiting_threads;
            } else {
                ++_Result.working_threads;
            }
        }
    );

    return _Result;
}

// FUNCTION thread_pool::is_thread_in_pool
bool thread_pool::is_thread_in_pool(const thread::id _Id) const noexcept {
    return _Mylist._Select_thread_by_id(_Id) != nullptr;
}

// FUNCTION thread_pool::increase_threads
_NODISCARD_ATTR bool thread_pool::increase_threads(const size_t _Count) noexcept {
    if (_Mystate == _Closed) { // must not be closed
        return false;
    }

    return _Mylist._Grow(_Count);
}

// FUNCTION thread_pool::decrease_threads
_NODISCARD_ATTR bool thread_pool::decrease_threads(const size_t _Count) noexcept {
    if (_Mystate == _Closed) { // must not be closed
        return false;
    }
    
    if (_Count >= _Mylist._Size()) { // at least 1 thread must be available
        return false;
    }

    return _Mylist._Reduce(_Count);
}

// FUNCTION thread_pool::resize
_NODISCARD_ATTR bool thread_pool::resize(const size_t _New_size) noexcept {
    if (_Mystate == _Closed) { // must not be closed
        return false;
    }
    
    if (_New_size == 0) { // at least 1 thread must be available
        return false;
    }
    
    const size_t _Old_size = _Mylist._Size();
    if (_New_size == _Old_size) { // nothing has changed
        return true;
    }

    if (_New_size > _Old_size) { // hire additional threads
        return increase_threads(_New_size - _Old_size);
    } else { // dismiss some of the existing threads
        return decrease_threads(_Old_size - _New_size);
    }
}

// FUNCTION thread_pool::cancel_all_pending_tasks
void thread_pool::cancel_all_pending_tasks() noexcept {
    if (_Mystate != _Closed) { // must not be closed
        _Mylist._For_each_thread(
            [](thread& _Thread) noexcept {
                _Thread.cancel_all_pending_tasks();
            }
        );
    }
}

// FUNCTION thread_pool::schedule_task
_NODISCARD_ATTR bool thread_pool::schedule_task(const thread::task _Task, void* const _Data) noexcept {
    if (_Mystate == _Closed) { // scheduling inactive
        return false;
    }

    thread* const _Thread = _Select_ideal_thread();
    return _Thread ? _Thread->schedule_task(_Task, _Data) : false;
}

_NODISCARD_ATTR bool thread_pool::schedule_task(
    const thread::task _Task, void* const _Data, const task_priority _Priority) noexcept {
    if (_Mystate == _Closed) { // scheduling inactive
        return false;
    }

    thread* const _Thread = _Select_ideal_thread();
    return _Thread ? _Thread->schedule_task(_Task, _Data, _Priority) : false;
}

// FUNCTION thread_pool::suspend
_NODISCARD_ATTR bool thread_pool::suspend() noexcept {
    if (_Mystate != _Working) { // must be working
        return false;
    }

    _Mystate = _Waiting;
    _Mylist._For_each_thread( // try to suspend all threads
        [](thread& _Thread) noexcept {
            (void) _Thread.suspend();
        }
    );
    return true;
}

// FUNCTION thread_pool::resume
_NODISCARD_ATTR bool thread_pool::resume() noexcept {
    if (_Mystate != _Waiting) { // must be waiting
        return false;
    }

    _Mystate = _Working;
    _Mylist._For_each_thread( // try to resume all threads
        [](thread& _Thread) noexcept {
            (void) _Thread.resume();
        }
    );
    return true;
}
_TPLMGR_END

#endif // _TPLMGR_PREPROCESSOR_GUARD