/*
 * Copyright (C) 2021 Apple Inc. All rights reserved.
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

#pragma once

#if HAVE(WEBGPU_IMPLEMENTATION)

#include "WebGPU.h"
#include <WebGPU/WebGPU.h>
#include <wtf/Deque.h>

namespace PAL::WebGPU {

class ConvertToBackingContext;

class GPUImpl final : public GPU, public RefCounted<GPUImpl> {
    WTF_MAKE_FAST_ALLOCATED;
public:
    PAL_EXPORT static RefPtr<GPUImpl> create();

    static Ref<GPUImpl> create(WGPUInstance instance, ConvertToBackingContext& convertToBackingContext)
    {
        return adoptRef(*new GPUImpl(instance, convertToBackingContext));
    }

    virtual ~GPUImpl();

    void ref() const final { RefCounted<GPUImpl>::ref(); }
    void deref() const final { RefCounted<GPUImpl>::deref(); }

private:
    friend class DowncastConvertToBackingContext;
    friend void requestAdapterCallback(WGPURequestAdapterStatus, WGPUAdapter, const char* message, void* userdata);

    GPUImpl(WGPUInstance, ConvertToBackingContext&);

    GPUImpl(const GPUImpl&) = delete;
    GPUImpl(GPUImpl&&) = delete;
    GPUImpl& operator=(const GPUImpl&) = delete;
    GPUImpl& operator=(GPUImpl&&) = delete;

    WGPUInstance backing() const { return m_backing; }

    void requestAdapterCallback(WGPURequestAdapterStatus, WGPUAdapter, const char* message);
    void requestAdapter(const RequestAdapterOptions&, WTF::Function<void(RefPtr<Adapter>&&)>&&) final;

    Deque<WTF::Function<void(RefPtr<Adapter>&&)>> m_callbacks;

    WGPUInstance m_backing { nullptr };
    Ref<ConvertToBackingContext> m_convertToBackingContext;
};

} // namespace PAL::WebGPU

#endif // HAVE(WEBGPU_IMPLEMENTATION)
