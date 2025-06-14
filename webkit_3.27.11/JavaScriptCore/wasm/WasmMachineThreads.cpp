/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
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
#include "WasmMachineThreads.h"

#if ENABLE(WEBASSEMBLY)

#include "MachineStackMarker.h"
#include <wtf/NeverDestroyed.h>
#include <wtf/ThreadMessage.h>

namespace JSC { namespace Wasm {


inline MachineThreads& wasmThreads()
{
    static LazyNeverDestroyed<MachineThreads> threads;
    static WKCStd::once_flag once;
    WKCStd::call_once(once, [] {
        threads.construct();
    });

    return threads;
}

void startTrackingCurrentThread()
{
    wasmThreads().addCurrentThread();
}

void resetInstructionCacheOnAllThreads()
{
    Locker locker { wasmThreads().getLock() };
    ThreadSuspendLocker threadSuspendLocker;
    for (auto& thread : wasmThreads().threads(locker)) {
        sendMessage(threadSuspendLocker, thread.get(), [] (const PlatformRegisters&) {
            // It's likely that the signal handler will already reset the instruction cache but we might as well be sure.
            WTF::crossModifyingCodeFence();
        });
    }
}

    
} } // namespace JSC::Wasm

#endif // ENABLE(WEBASSEMBLY)
