/*
 * Copyright (C) 2016-2019 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "ProfilerUID.h"

#include "JSCJSValueInlines.h"
#include "JSGlobalObject.h"
#include <wtf/Lock.h>

namespace JSC { namespace Profiler {

UID UID::create()
{
#if !PLATFORM(WKC)
    static Lock lock;
    static uint64_t counter;
    
    Locker locker { lock };
#else
    static LazyNeverDestroyed<Lock> lock;
    static uint64_t counter;
    static WKCStd::once_flag flag;

    WKCStd::call_once(flag, [&]() {
        lock.construct();
        counter = 0;
    });

    Locker locker { lock.get() };
#endif

    UID result;
    result.m_uid = ++counter;
    return result;
}

void UID::dump(PrintStream& out) const
{
    out.print(m_uid);
}

JSValue UID::toJS(JSGlobalObject* globalObject) const
{
    return jsString(globalObject->vm(), toString(*this));
}

} } // namespace JSC::Profiler

