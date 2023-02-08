// shared_lock.cpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#include <build/tplmgr_pch.hpp>
#include <tplmgr/shared_lock.hpp>
#if _TPLMGR_PREPROCESSOR_GUARD

_TPLMGR_BEGIN
// FUNCTION shared_lock constructor/destructor
shared_lock::shared_lock() noexcept : _Myimpl(nullptr) {
    ::InitializeSRWLock(_TPLMGR addressof(_Myimpl));
}

shared_lock::~shared_lock() noexcept {}

// FUNCTION shared_lock::lock
void shared_lock::lock() noexcept {
    ::AcquireSRWLockExclusive(_TPLMGR addressof(_Myimpl));
}

// FUNCTION shared_lock::unlock
void shared_lock::unlock() noexcept {
    ::ReleaseSRWLockExclusive(_TPLMGR addressof(_Myimpl));
}

// FUNCTION shared_lock::lock_shared
void shared_lock::lock_shared() noexcept {
    ::AcquireSRWLockShared(_TPLMGR addressof(_Myimpl));
}

// FUNCTION shared_lock::unlock_shared
void shared_lock::unlock_shared() noexcept {
    ::ReleaseSRWLockShared(_TPLMGR addressof(_Myimpl));
}

// FUNCTION lock_guard copy constructor/destructor
lock_guard::lock_guard(shared_lock& _Lock) noexcept : _Mylock(_Lock) {
    _Mylock.lock();
}

lock_guard::~lock_guard() noexcept {
    _Mylock.unlock();
}

// FUNCTION shared_lock_guard copy constructor/destructor
shared_lock_guard::shared_lock_guard(shared_lock& _Lock) noexcept : _Mylock(_Lock) {
    _Mylock.lock_shared();
}

shared_lock_guard::~shared_lock_guard() noexcept {
    _Mylock.unlock_shared();
}
_TPLMGR_END

#endif // _TPLMGR_PREPROCESSOR_GUARD