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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "RuntimeApplicationChecks.h"

#include <wtf/NeverDestroyed.h>
#include <wtf/ProcessID.h>
#include <wtf/RunLoop.h>

namespace WebCore {

#if !ASSERT_MSG_DISABLED
#if !PLATFORM(WKC)
static bool presentingApplicationPIDOverrideWasQueried;
#else
WKC_DEFINE_GLOBAL_BOOL(presentingApplicationPIDOverrideWasQueried, false);
#endif
#endif

static std::optional<int>& presentingApplicationPIDOverride()
{
    static NeverDestroyed<std::optional<int>> pid;
#if PLATFORM(WKC)
    if (pid.isNull())
        pid.construct();
#endif
#if !ASSERT_MSG_DISABLED
    presentingApplicationPIDOverrideWasQueried = true;
#endif
    return pid;
}

int presentingApplicationPID()
{
    const auto& pid = presentingApplicationPIDOverride();
    ASSERT(!pid || RunLoop::isMain());
    return pid ? pid.value() : getCurrentProcessID();
}

void setPresentingApplicationPID(int pid)
{
    ASSERT(RunLoop::isMain());
    ASSERT_WITH_MESSAGE(!presentingApplicationPIDOverrideWasQueried, "presentingApplicationPID() should not be called before setPresentingApplicationPID()");
    presentingApplicationPIDOverride() = pid;
}

static std::optional<AuxiliaryProcessType>& auxiliaryProcessType()
{
#if !PLATFORM(WKC)
    static std::optional<AuxiliaryProcessType> auxiliaryProcessType;
#else
    static LazyNeverDestroyed<std::optional<AuxiliaryProcessType>> auxiliaryProcessType;
    if (auxiliaryProcessType.isNull())
        auxiliaryProcessType.construct();
#endif
    return auxiliaryProcessType;
}

bool isInAuxiliaryProcess()
{
    return !!auxiliaryProcessType();
}

void setAuxiliaryProcessType(AuxiliaryProcessType type)
{
    auxiliaryProcessType() = type;
}

void setAuxiliaryProcessTypeForTesting(std::optional<AuxiliaryProcessType> type)
{
    auxiliaryProcessType() = type;
}

bool checkAuxiliaryProcessType(AuxiliaryProcessType type)
{
    auto currentType = auxiliaryProcessType();
    if (!currentType)
        return false;
    return *currentType == type; 
}

std::optional<AuxiliaryProcessType> processType()
{
    return auxiliaryProcessType();
}

const char* processTypeDescription(std::optional<AuxiliaryProcessType> type)
{
    if (!type)
        return "UI";

    switch (*type) {
    case AuxiliaryProcessType::WebContent:
        return "Web";
    case AuxiliaryProcessType::Network:
        return "Network";
    case AuxiliaryProcessType::Plugin:
        return "Plugin";
#if ENABLE(GPU_PROCESS)
    case AuxiliaryProcessType::GPU:
        return "GPU";
#endif
#if ENABLE(WEB_AUTHN)
    case AuxiliaryProcessType::WebAuthn:
        return "WebAuthn";
#endif
    }
    return "Unknown";
}

} // namespace WebCore
