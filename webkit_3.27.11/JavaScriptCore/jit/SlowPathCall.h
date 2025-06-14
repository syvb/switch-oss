/*
 * Copyright (C) 2013-2021 Apple Inc. All rights reserved.
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

#pragma once

#include "JIT.h"
#include "MacroAssemblerCodeRef.h"
#include "SlowPathFunction.h"

#if ENABLE(JIT)

namespace JSC {

class JITSlowPathCall {
public:
    JITSlowPathCall(JIT* jit, const Instruction* pc, SlowPathFunction slowPathFunction)
        : m_jit(jit)
        , m_slowPathFunction(slowPathFunction)
        , m_pc(pc)
    {
        assertIsCFunctionPtr(slowPathFunction);
    }

#if ENABLE(EXTRA_CTI_THUNKS)
    void call();
    static MacroAssemblerCodeRef<JITThunkPtrTag> generateThunk(VM&, SlowPathFunction);

#else // not ENABLE(EXTRA_CTI_THUNKS)

    JIT::Call call()
    {
        m_jit->updateTopCallFrame();
#if CPU(X86_64) && (OS(WINDOWS) || OS(WINDOWS_WKC))
        m_jit->addPtr(MacroAssembler::TrustedImm32(-16), MacroAssembler::stackPointerRegister);
        m_jit->move(MacroAssembler::stackPointerRegister, JIT::argumentGPR0);
        m_jit->move(JIT::callFrameRegister, JIT::argumentGPR1);
        m_jit->move(JIT::TrustedImmPtr(m_pc), JIT::argumentGPR2);
#else
        m_jit->move(JIT::callFrameRegister, JIT::argumentGPR0);
        m_jit->move(JIT::TrustedImmPtr(m_pc), JIT::argumentGPR1);
#endif
        JIT::Call call = m_jit->appendCall(m_slowPathFunction);

#if CPU(X86_64) && (OS(WINDOWS) || OS(WINDOWS_WKC))
        m_jit->pop(JIT::regT0); // vPC
        m_jit->pop(JIT::regT1); // callFrame register
        static_assert(JIT::regT0 == GPRInfo::returnValueGPR);
        static_assert(JIT::regT1 == GPRInfo::returnValueGPR2);
#endif

        m_jit->exceptionCheck();
        return call;
    }
#endif // ENABLE(EXTRA_CTI_THUNKS)

private:
    JIT* m_jit;
    SlowPathFunction m_slowPathFunction;
    const Instruction* m_pc;
};

} // namespace JS

#endif // ENABLE(JIT)
