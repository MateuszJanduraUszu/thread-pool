thread-pool
---

A simple thread-pool implementation in C++11 with features from the latest version (if possible).

Features
---

* Thread-safe memory allocation/deallocation
* No standard primitives like [thread](https://en.cppreference.com/w/cpp/thread/thread), [mutex](https://en.cppreference.com/w/cpp/thread/mutex), [condition_variable](https://en.cppreference.com/w/cpp/thread/condition_variable) etc. (except [atomic](https://en.cppreference.com/w/cpp/atomic/atomic))
* No exceptions
* Calls WinAPI functions directly (no wrappers)
* Almost no STL containers
* Each thread has its own task queue
* Suspendable

Future features
---

* No STL containers
* No STL traits
* Other compilers support

How to use it?
---

There are two ways, the first is:

```cpp
#include <tplmgr/thread_pool.hpp>

::tplmgr::thread_pool _Pool(/* number of threads */);
value_type _Value;
_Pool.schedule_task(
    [](void* const _Data) {
        value_type* const _Value = static_cast<value_type*>(_Data);
        // work to do...
    },
    ::tplmgr::addressof(_Value)
);
```

The other is:

```cpp
#include <tplmgr/async.hpp>
#include <tplmgr/thread_pool.hpp>

::tplmgr::thread_pool _Pool(/* number of threads */);
::tplmgr::async(_Pool,
    []() noexcept {
        // work to do...
    }
);
```

The first one only accepts a `void` function type with one `void*` argument.
The second one accepts any type of function with any arguments.

How to
---

* Hire additional threads

```cpp
::tplmgr::thread_pool _Pool(/* number of threads */);
if (_Pool.increase_threads(/* number of threads */)) {
    // on success...
} else {
    // on failure...
}
```

* Dismiss a few existing threads

```cpp
::tplmgr::thread_pool _Pool(/* number of threads */);
if (_Pool.decrease_threads(/* number of threads */)) {
    // on success...
} else {
    // on failure...
}
```

* Collect thread-pool statistics

```cpp
::tplmgr::thread_pool _Pool(/* number of threads */);
const ::tplmgr::thread_pool::statistics& _Stats = _Pool.collect_statistics();
::std::cout << "Waiting threads: " << _Stats.waiting_threads << '\n'
    << "Working threads: " << _Stats.working_threads << '\n'
    << "Pending tasks: " << _Stats.pending_tasks << '\n';
```

Important
---
* Once the thread-pool is closed, it cannot be reopened
* The number of threads must not be greater than the total number of threads - 1
* At least 1 thread must be available
* The thread-pool assumes that you handle all exceptions that occur in your task
* If the thread-pool is about to close, all threads will finish their current task and discard others
* When a thread finishes its current task and there are no other tasks in its task queue, it suspends itself and waits for any task

Other usable types
---

* `allocator` - provides thread-safe memory allocation/deallocation
* `lock_guard` - automatically locks and unlocks an exclusive lock (RAII)
* `shared_lock` - provides shared/exclusive lock
* `shared_lock_guard` - automatically locks and unlocks a shared lock (RAII)
* `shared_queue` - controls access to the default queue
* `thread` - manages single thread (state, task scheduling)

Dependencies
---

The library must be used on Windows with Microsoft Visual C++ (at least C++11 support) compiler.