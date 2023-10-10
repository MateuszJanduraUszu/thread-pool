thread-pool
---

>**WARNING: This API is deprecated and will be removed soon.**

A simple thread-pool implementation in C++11 with features from the latest version (if possible).

Features
---

* Thread-safe memory allocation/deallocation
* No standard primitives like [thread](https://en.cppreference.com/w/cpp/thread/thread), [mutex](https://en.cppreference.com/w/cpp/thread/mutex), [condition_variable](https://en.cppreference.com/w/cpp/thread/condition_variable) etc. (except [atomic](https://en.cppreference.com/w/cpp/atomic/atomic))
* No exceptions
* Calls WinAPI functions directly (no wrappers)
* Thread-safe containers
* Each thread has its own priority-based task queue
* Suspendable

Task scheduling
---

* standard scheduling:

```cpp
#include <tplmgr/thread_pool.hpp>

::tplmgr::thread_pool _Pool(/* initial number of threads*/);
value_type _Value; // value passed to the task
if (!_Pool.schedule( // schedule a new task with normal priority
    [](void* const _Data) {
        value_type& _Value = *static_cast<value_type*>(_Data);
        // do the task...
    },
    ::tplmgr::addressof(_Value))) {
    // handle failure...
}

if (!_Pool.schedule( // schedule a new task with high priority
    [](void* const _Data) {
        value_type& _Value = *static_cast<value_type*>(_Data);
        // do the task...
    },
    ::tplmgr::addressof(_Value)), ::tplmgr::task_priority::high) {
    // handle failure...
}
```

* custom scheduling:

```cpp
#include <tplmgr/thread_pool.hpp>

::tplmgr::thread_pool _Pool(/* initial number of threads */);
value_type _Value; // value passed to the task
if (!::tplmgr::async(_Pool, // schedule a new task with normal priority
    [&_Value]{
        // do the task...
    }
    )) {
    // handle failure...
}

if (!::tplmgr::async(_Pool, ::tplmgr::task_priority::high, // schedule a new task with high priority
    [&_Value]{
        // do the task...
    }
    )) {
    // handle failure...
}
```

Examples
---

* increasing the thread-pool

```cpp
::tplmgr::thread_pool _Pool(/* initial number of threads */);
if (!_Pool.increase_threads(/* number of threads to hire */)) {
    // handle failure...
}
```

* decreasing the thread-pool

```cpp
::tplmgr::thread_pool _Pool(/* initial number of threads */);
if (!_Pool.decrease_threads(/* number of threads to dismiss */)) {
    // handle failure...
}
```

* collecting thread-pool statistics

```cpp
::tplmgr::thread_pool _Pool(/* initial number of threads */);
const ::tplmgr::thread_pool::statistics& _Stats = _Pool.collect_statistics();
::std::cout << "Waiting threads: " << _Stats.waiting_threads << '\n'
    << "Working threads: " << _Stats.working_threads << '\n'
    << "Pending tasks: " << _Stats.pending_tasks << '\n';
```

Important
---
* Once the thread-pool is closed, it cannot be reopened
* At least 1 thread must be available
* The thread-pool assumes that you handle all exceptions that occur in your task
* If the thread-pool is about to close, all threads will finish their current task and discard others
* When a thread finishes its current task and there are no other tasks in its task queue, it suspends itself and waits for any task
* The default task priority is normal

Other usable types
---

* `allocator<T>` - provides thread-safe memory allocation/deallocation (compatible with the standard)
* `lock_guard` - automatically locks and unlocks an exclusive lock (RAII)
* `shared_lock` - provides a shared/exclusive lock
* `shared_lock_guard` - automatically locks and unlocks a shared lock (RAII)
* `shared_queue<T>` - provides a thread-safe queue that can be shared between multiple threads
* `thread` - manages a single thread (state, task scheduling etc.)

Dependencies
---

The library requires the Windows OS and a compiler with at least C++11 support.