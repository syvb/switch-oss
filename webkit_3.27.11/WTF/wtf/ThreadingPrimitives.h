/*
 * Copyright (C) 2007, 2008, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Justin Haygood (jhaygood@reaktix.com)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#pragma once

#include <limits.h>
#include <wtf/FastMalloc.h>
#include <wtf/Locker.h>
#include <wtf/Noncopyable.h>
#include <wtf/WallTime.h>

#if OS(WINDOWS)
#include <windows.h>
#endif

#if USE(PTHREADS)
#include <pthread.h>
#if !defined(PTHREAD_KEYS_MAX)
// PTHREAD_KEYS_MAX is not defined in bionic nor in Hurd, so explicitly define it here.
#define PTHREAD_KEYS_MAX 1024
#endif
#endif

#if PLATFORM(WKC)
#include <wkc/wkcpeer.h>
#include "wkcpthread.h"
#endif

namespace WTF {

using ThreadFunction = void (*)(void* argument);

#if USE(PTHREADS)
using PlatformThreadHandle = pthread_t;
using PlatformMutex = pthread_mutex_t;
using PlatformCondition = pthread_cond_t;
using ThreadSpecificKey = pthread_key_t;
#if OS(LINUX)
using ThreadIdentifier = pid_t;
#endif
#elif OS(WINDOWS)
using ThreadIdentifier = uint32_t;
using PlatformThreadHandle = HANDLE;
using PlatformMutex = SRWLOCK;
using PlatformCondition = CONDITION_VARIABLE;
using ThreadSpecificKey = DWORD;
#elif PLATFORM(WKC)
using ThreadIdentifier = uint32_t;
using PlatformThreadHandle = void*;
using PlatformMutex = WKCLightMutex;
using PlatformCondition = WKCLightCond;
using ThreadSpecificKey = uint32_t;
#else
#error "Not supported platform"
#endif

class Mutex final {
    WTF_MAKE_NONCOPYABLE(Mutex);
    WTF_MAKE_FAST_ALLOCATED;
public:
#if PLATFORM(WKC)
    Mutex();
#else
    constexpr Mutex() = default;
#endif
    WTF_EXPORT_PRIVATE ~Mutex();

    WTF_EXPORT_PRIVATE void lock();
    WTF_EXPORT_PRIVATE bool tryLock();
    WTF_EXPORT_PRIVATE void unlock();

    PlatformMutex& impl() { return m_mutex; }

private:
#if USE(PTHREADS)
    PlatformMutex m_mutex = PTHREAD_MUTEX_INITIALIZER;
#elif OS(WINDOWS)
    PlatformMutex m_mutex = SRWLOCK_INIT;
#elif PLATFORM(WKC)
    PlatformMutex m_mutex;
#endif
};

typedef Locker<Mutex> MutexLocker;

class ThreadCondition final {
    WTF_MAKE_NONCOPYABLE(ThreadCondition);
    WTF_MAKE_FAST_ALLOCATED;
public:
#if PLATFORM(WKC)
    ThreadCondition();
#else
    constexpr ThreadCondition() = default;
#endif
    WTF_EXPORT_PRIVATE ~ThreadCondition();
    
    WTF_EXPORT_PRIVATE void wait(Mutex& mutex);
    // Returns true if the condition was signaled before absoluteTime, false if the absoluteTime was reached or is in the past.
    WTF_EXPORT_PRIVATE bool timedWait(Mutex&, WallTime absoluteTime);
    WTF_EXPORT_PRIVATE void signal();
    WTF_EXPORT_PRIVATE void broadcast();
    
private:
#if USE(PTHREADS)
    PlatformCondition m_condition = PTHREAD_COND_INITIALIZER;
#elif OS(WINDOWS)
    PlatformCondition m_condition = CONDITION_VARIABLE_INIT;
#elif PLATFORM(WKC)
    PlatformCondition m_condition = WKC_PTHREAD_COND_INITIALIZER;
#endif
};

#if USE(PTHREADS)

static constexpr ThreadSpecificKey InvalidThreadSpecificKey = PTHREAD_KEYS_MAX;

inline void threadSpecificKeyCreate(ThreadSpecificKey* key, void (*destructor)(void *))
{
    int error = pthread_key_create(key, destructor);
    if (error)
        CRASH();
}

inline void threadSpecificKeyDelete(ThreadSpecificKey key)
{
    int error = pthread_key_delete(key);
    if (error)
        CRASH();
}

inline void threadSpecificSet(ThreadSpecificKey key, void* value)
{
    pthread_setspecific(key, value);
}

inline void* threadSpecificGet(ThreadSpecificKey key)
{
    return pthread_getspecific(key);
}
#elif PLATFORM(WKC)

const ThreadSpecificKey InvalidThreadSpecificKey = wkc_pthread_kys_max;

inline void threadSpecificKeyCreate(ThreadSpecificKey* key, void(*destructor)(void *))
{
    int error = wkc_pthread_key_create(key, destructor);
    if (error)
        CRASH();
}

inline void threadSpecificKeyDelete(ThreadSpecificKey key)
{
    int error = wkc_pthread_key_delete(key);
    if (error)
        CRASH();
}

inline void threadSpecificSet(ThreadSpecificKey key, void* value)
{
    wkc_pthread_setspecific(key, value);
}

inline void* threadSpecificGet(ThreadSpecificKey key)
{
    return wkc_pthread_getspecific(key);
}

#endif

} // namespace WTF

using WTF::Mutex;
using WTF::MutexLocker;
using WTF::ThreadCondition;
