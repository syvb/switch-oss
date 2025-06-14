/*
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
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
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
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

#include "config.h"

#if COMPILER(MSVC) && _MSC_VER >= 1700
extern bool __uncaught_exception();
#endif

#include <JavaScriptCore/JSObjectRef.h>

#include "JSLock.h"
#include "VM.h"
#include "CommonVM.h"
#include "GCController.h"

static void
do_collect()
{
    WebCore::GCController::singleton().garbageCollectNow();
}

static void
do_collectOnAlternateThread(bool waitUntilDone)
{
    WebCore::GCController::singleton().garbageCollectOnAlternateThreadForDebugging(waitUntilDone);
}

static int
do_getJSObjectCount()
{
    JSC::JSLockHolder lock(WebCore::commonVM());
    return WebCore::commonVM().heap.objectCount();
}

class GCController {
public:
    GCController();
    ~GCController();

    void makeWindowObject(JSContextRef context, JSObjectRef windowObject, JSValueRef* exception);

    // Controller Methods - platfrom independant implementations
    void collect() const;
    void collectOnAlternateThread(bool waitUntilDone) const;
    size_t getJSObjectCount() const;

private:
    static JSClassRef getJSClass();
};

void GCController::collect() const
{
    do_collect();
}

void GCController::collectOnAlternateThread(bool waitUntilDone) const
{
    do_collectOnAlternateThread(waitUntilDone);
}

size_t GCController::getJSObjectCount() const
{
    return do_getJSObjectCount();
}
