/*
 * Copyright (C) 2015-2021 Apple Inc. All rights reserved.
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
#if !PLATFORM(WKC)
#include "WebGLVertexArrayObjectBase.h"
#endif

#if ENABLE(WEBGL)
#if PLATFORM(WKC)
#include "WebGLVertexArrayObjectBase.h"
#endif

#include "WebGLRenderingContextBase.h"
#include <JavaScriptCore/AbstractSlotVisitorInlines.h>
#include <wtf/Locker.h>

namespace WebCore {

WebGLVertexArrayObjectBase::WebGLVertexArrayObjectBase(WebGLRenderingContextBase& context, Type type)
    : WebGLContextObject(context)
    , m_type(type)
{
    m_vertexAttribState.resize(context.getMaxVertexAttribs());
#if !USE(ANGLE)
    updateVertexAttrib0();
#endif
}

void WebGLVertexArrayObjectBase::setElementArrayBuffer(const AbstractLocker& locker, WebGLBuffer* buffer)
{
    if (buffer)
        buffer->onAttached();
    if (m_boundElementArrayBuffer)
        m_boundElementArrayBuffer->onDetached(locker, context()->graphicsContextGL());
    m_boundElementArrayBuffer = buffer;
    
}
void WebGLVertexArrayObjectBase::setVertexAttribEnabled(int index, bool flag)
{
    auto& state = m_vertexAttribState[index];
    if (state.enabled == flag)
        return;
    state.enabled = flag;
    if (!state.validateBinding())
        m_allEnabledAttribBuffersBoundCache = false;
    else
        m_allEnabledAttribBuffersBoundCache.reset();
}

void WebGLVertexArrayObjectBase::setVertexAttribState(const AbstractLocker& locker, GCGLuint index, GCGLsizei bytesPerElement, GCGLint size, GCGLenum type, GCGLboolean normalized, GCGLsizei stride, GCGLintptr offset, bool isInteger, WebGLBuffer* buffer)
{
    auto& state = m_vertexAttribState[index];
    bool bindingWasValid = state.validateBinding();
    if (buffer)
        buffer->onAttached();
    if (state.bufferBinding)
        state.bufferBinding->onDetached(locker, context()->graphicsContextGL());
    state.bufferBinding = buffer;
    if (!state.validateBinding())
        m_allEnabledAttribBuffersBoundCache = false;
    else if (!bindingWasValid)
        m_allEnabledAttribBuffersBoundCache.reset();
    state.bytesPerElement = bytesPerElement;
    state.size = size;
    state.type = type;
    state.normalized = normalized;
    state.stride = stride ? stride : bytesPerElement;
    state.originalStride = stride;
    state.offset = offset;
    state.isInteger = isInteger;
}

void WebGLVertexArrayObjectBase::unbindBuffer(const AbstractLocker& locker, WebGLBuffer& buffer)
{
    if (m_boundElementArrayBuffer == &buffer) {
        m_boundElementArrayBuffer->onDetached(locker, context()->graphicsContextGL());
        m_boundElementArrayBuffer = nullptr;
    }
    
    for (auto& state : m_vertexAttribState) {
        if (state.bufferBinding == &buffer) {
            buffer.onDetached(locker, context()->graphicsContextGL());
            state.bufferBinding = nullptr;
            if (!state.validateBinding())
                m_allEnabledAttribBuffersBoundCache = false;
        }
    }
#if !USE(ANGLE)
    updateVertexAttrib0();
#endif
}

#if !USE(ANGLE)
void WebGLVertexArrayObjectBase::updateVertexAttrib0()
{
    auto& state = m_vertexAttribState[0];
    if (!state.bufferBinding && !context()->isGLES2Compliant()) {
        state.bufferBinding = context()->m_vertexAttrib0Buffer;
        state.bufferBinding->onAttached();
        state.bytesPerElement = 0;
        state.size = 4;
        state.type = GraphicsContextGL::FLOAT;
        state.normalized = false;
        state.stride = 16;
        state.originalStride = 0;
        state.offset = 0;
        m_allEnabledAttribBuffersBoundCache.reset();
    }
}
#endif

void WebGLVertexArrayObjectBase::setVertexAttribDivisor(GCGLuint index, GCGLuint divisor)
{
    m_vertexAttribState[index].divisor = divisor;
}

void WebGLVertexArrayObjectBase::addMembersToOpaqueRoots(const AbstractLocker&, JSC::AbstractSlotVisitor& visitor)
{
    visitor.addOpaqueRoot(m_boundElementArrayBuffer.get());
    for (auto& state : m_vertexAttribState)
        visitor.addOpaqueRoot(state.bufferBinding.get());
}

bool WebGLVertexArrayObjectBase::areAllEnabledAttribBuffersBound()
{
    if (!m_allEnabledAttribBuffersBoundCache) {
        m_allEnabledAttribBuffersBoundCache = [&] {
            for (auto const& state : m_vertexAttribState) {
                if (!state.validateBinding())
                    return false;
            }
            return true;
        }();
    }
    return *m_allEnabledAttribBuffersBoundCache;
}

}

#endif // ENABLE(WEBGL)
