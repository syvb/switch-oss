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
 */

#ifndef MainThread_h
#define MainThread_h

#include <functional>
#if PLATFORM(WKC)
#include <wtf/Function.h>
#endif
#include <stdint.h>

namespace WTF {

typedef uint32_t ThreadIdentifier;
typedef void MainThreadFunction(void*);

// Must be called from the main thread.
WTF_EXPORT_PRIVATE void initializeMainThread();

WTF_EXPORT_PRIVATE void callOnMainThread(MainThreadFunction*, void* context);
WTF_EXPORT_PRIVATE void cancelCallOnMainThread(MainThreadFunction*, void* context);

#if PLATFORM(WKC)
WTF_EXPORT_PRIVATE void callOnMainThread(WTF::Function<void ()>);
#else
WTF_EXPORT_PRIVATE void callOnMainThread(std::function<void ()>);
#endif

#if PLATFORM(COCOA)
WTF_EXPORT_PRIVATE void callOnWebThreadOrDispatchAsyncOnMainThread(void (^block)());
#endif

WTF_EXPORT_PRIVATE void setMainThreadCallbacksPaused(bool paused);

WTF_EXPORT_PRIVATE bool isMainThread();

WTF_EXPORT_PRIVATE bool canAccessThreadLocalDataForThread(ThreadIdentifier);

#if USE(WEB_THREAD)
WTF_EXPORT_PRIVATE bool isWebThread();
WTF_EXPORT_PRIVATE bool isUIThread();
WTF_EXPORT_PRIVATE void initializeWebThread();
WTF_EXPORT_PRIVATE void initializeApplicationUIThreadIdentifier();
WTF_EXPORT_PRIVATE void initializeWebThreadIdentifier();
void initializeWebThreadPlatform();
#else
inline bool isWebThread() { return isMainThread(); }
inline bool isUIThread() { return isMainThread(); }
#endif // USE(WEB_THREAD)

void initializeGCThreads();

#if ENABLE(PARALLEL_GC)
void registerGCThread();
WTF_EXPORT_PRIVATE bool isMainThreadOrGCThread();
#elif OS(DARWIN) && !PLATFORM(EFL) && !PLATFORM(GTK)
WTF_EXPORT_PRIVATE bool isMainThreadOrGCThread();
#else
inline bool isMainThreadOrGCThread() { return isMainThread(); }
#endif

// NOTE: these functions are internal to the callOnMainThread implementation.
void initializeMainThreadPlatform();
void scheduleDispatchFunctionsOnMainThread();
void dispatchFunctionsFromMainThread();

#if OS(DARWIN) && !PLATFORM(EFL) && !PLATFORM(GTK)
#if !USE(WEB_THREAD)
// This version of initializeMainThread sets up the main thread as corresponding
// to the process's main thread, and not necessarily the thread that calls this
// function. It should only be used as a legacy aid for Mac WebKit.
WTF_EXPORT_PRIVATE void initializeMainThreadToProcessMainThread();
#endif // !USE(WEB_THREAD)
void initializeMainThreadToProcessMainThreadPlatform();
#endif

} // namespace WTF

using WTF::callOnMainThread;
using WTF::cancelCallOnMainThread;
#if PLATFORM(COCOA)
using WTF::callOnWebThreadOrDispatchAsyncOnMainThread;
#endif
using WTF::setMainThreadCallbacksPaused;
using WTF::isMainThread;
using WTF::isMainThreadOrGCThread;
using WTF::canAccessThreadLocalDataForThread;
using WTF::isUIThread;
using WTF::isWebThread;
#if USE(WEB_THREAD)
using WTF::initializeWebThread;
using WTF::initializeApplicationUIThreadIdentifier;
using WTF::initializeWebThreadIdentifier;
#endif

#endif // MainThread_h
