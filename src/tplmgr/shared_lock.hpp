// shared_lock.hpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef _TPLMGR_SHARED_LOCK_HPP_
#define _TPLMGR_SHARED_LOCK_HPP_
#include <tplmgr/core.hpp>
#if _TPLMGR_PREPROCESSOR_GUARD
#include <tplmgr/utils.hpp>
#include <synchapi.h>

_TPLMGR_BEGIN
// CLASS shared_lock
class _TPLMGR_API shared_lock { // non-copyable shared/exclusive lock object
public:
    shared_lock() noexcept;
    ~shared_lock() noexcept;

    shared_lock(const shared_lock&) = delete;
    shared_lock& operator=(const shared_lock&) = delete;

    // locks the code segment (exclusive mode)
    void lock() noexcept;

    // unlocks the code segment (exclusive mode)
    void unlock() noexcept;

    // locks the code segment (shared mode)
    void lock_shared() noexcept;

    // unlocks the code segment (shared mode)
    void unlock_shared() noexcept;

private:
    SRWLOCK _Myimpl;
};

// CLASS lock_guard
class _TPLMGR_API _NODISCARD_ATTR lock_guard { // locks the code segment for exactly one thread
public:
    explicit lock_guard(shared_lock& _Lock) noexcept;
    ~lock_guard() noexcept;

    lock_guard() = delete;
    lock_guard(const lock_guard&) = delete;
    lock_guard& operator=(const lock_guard&) = delete;

private:
    shared_lock& _Mylock;
};

// CLASS shared_lock_guard
class _TPLMGR_API _NODISCARD_ATTR shared_lock_guard { // locks the code segment for multiple threads
public:
    explicit shared_lock_guard(shared_lock& _Lock) noexcept;
    ~shared_lock_guard() noexcept;

    shared_lock_guard() = delete;
    shared_lock_guard(const shared_lock_guard&) = delete;
    shared_lock_guard& operator=(const shared_lock_guard&) = delete;

private:
    shared_lock& _Mylock;
};
_TPLMGR_END

#endif // _TPLMGR_PREPROCESSOR_GUARD
#endif // _TPLMGR_SHARED_LOCK_HPP_