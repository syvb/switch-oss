/*
 * Copyright (C) 2010 Google Inc.  All rights reserved.
 * Copyright (C) 2012, 2014 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "AsyncFileStream.h"

#include "FileStream.h"
#include "FileStreamClient.h"
#include "URL.h"
#include <wtf/AutodrainedPool.h>
#include <wtf/MainThread.h>
#include <wtf/MessageQueue.h>
#include <wtf/NeverDestroyed.h>

namespace WebCore {

struct AsyncFileStream::Internals {
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
public:
#endif
    explicit Internals(FileStreamClient&);

    FileStream stream;
    FileStreamClient& client;
#if !COMPILER(MSVC)
    std::atomic_bool destroyed { false };
#else
    std::atomic_bool destroyed;
#endif
};

inline AsyncFileStream::Internals::Internals(FileStreamClient& client)
    : client(client)
{
#if COMPILER(MSVC)
    // Work around a bug that prevents the default value above from compiling.
    atomic_init(&destroyed, false);
#endif
}

#if PLATFORM(WKC)
static void callOnFileThread(WTF::Function<void()>&& function)
#else
static void callOnFileThread(std::function<void()>&& function)
#endif
{
    ASSERT(isMainThread());
    ASSERT(function);

#if !PLATFORM(WKC)
    static NeverDestroyed<MessageQueue<std::function<void()>>> queue;

    static std::once_flag createFileThreadOnce;
    std::call_once(createFileThreadOnce, [] {
        createThread("WebCore: AsyncFileStream", [] {
            for (;;) {
                AutodrainedPool pool;

                auto function = queue.get().waitForMessage();

                // This can never be null because we never kill the MessageQueue.
                ASSERT(function);

                // This can bever be null because we never queue a function that is null.
                ASSERT(*function);

                (*function)();
            }
        });
    });

    queue.get().append(std::make_unique<std::function<void()>>(WTF::move(function)));
#else
    WKC_DEFINE_STATIC_PTR(MessageQueue<WTF::Function<void()>>*, queue, 0);
    if (!queue)
        queue = new MessageQueue<WTF::Function<void()>>();
    WKC_DEFINE_STATIC_BOOL(createFileThreadOnce, false);
    if (!createFileThreadOnce) {
        createFileThreadOnce = true;
        createThread("WebCore: AsyncFileStream", [] {
            for (;;) {
                AutodrainedPool pool;

                auto function = queue->waitForMessage();

                // This can never be null because we never kill the MessageQueue.
                ASSERT(function);

                // This can bever be null because we never queue a function that is null.
                ASSERT(*function);

                (*function)();
                WTF::fastFree(function.release());
            }
        });
    }

    queue->append(std::make_unique<WTF::Function<void()>>(WTF::move(function)));
#endif
}

AsyncFileStream::AsyncFileStream(FileStreamClient& client)
    : m_internals(std::make_unique<Internals>(client))
{
    ASSERT(isMainThread());
}

AsyncFileStream::~AsyncFileStream()
{
    ASSERT(isMainThread());

    // Release so that we can control the timing of deletion below.
    auto& internals = *m_internals.release();

    // Set flag to prevent client callbacks and also prevent queued operations from starting.
    internals.destroyed = true;

    // Call through file thread and back to main thread to make sure deletion happens
    // after all file thread functions and all main thread functions called from them.
    callOnFileThread([&internals] {
        callOnMainThread([&internals] {
            delete &internals;
        });
    });
}

#if PLATFORM(WKC)
void AsyncFileStream::perform(WTF::Function<WTF::Function<void(FileStreamClient&)>(FileStream&)> operation)
#else
void AsyncFileStream::perform(std::function<std::function<void(FileStreamClient&)>(FileStream&)> operation)
#endif
{
    auto& internals = *m_internals;
#if PLATFORM(WKC)
    callOnFileThread([&internals, operation = WTF::move(operation)] {
#else
    callOnFileThread([&internals, operation] {
#endif
        // Don't do the operation if stop was already called on the main thread. Note that there is
        // a race here, but since skipping the operation is an optimization it's OK that we can't
        // guarantee exactly which operations are skipped. Note that this is also the only reason
        // we use an atomic_bool rather than just a bool for destroyed.
        if (internals.destroyed)
            return;
        auto mainThreadWork = operation(internals.stream);
#if PLATFORM(WKC)
        callOnMainThread([&internals, mainThreadWork = WTF::move(mainThreadWork)] {
#else
        callOnMainThread([&internals, mainThreadWork] {
#endif
            if (internals.destroyed)
                return;
            mainThreadWork(internals.client);
        });
    });
}

void AsyncFileStream::getSize(const String& path, double expectedModificationTime)
{
    StringCapture capturedPath(path);
    // FIXME: Explicit return type here and in all the other cases like this below is a workaround for a deficiency
    // in the Windows compiler at the time of this writing. Could remove it if that is resolved.
#if PLATFORM(WKC)
    perform([capturedPath, expectedModificationTime](FileStream& stream) -> WTF::Function<void(FileStreamClient&)> {
#else
    perform([capturedPath, expectedModificationTime](FileStream& stream) -> std::function<void(FileStreamClient&)> {
#endif
        long long size = stream.getSize(capturedPath.string(), expectedModificationTime);
        return [size](FileStreamClient& client) {
            client.didGetSize(size);
        };
    });
}

void AsyncFileStream::openForRead(const String& path, long long offset, long long length)
{
    StringCapture capturedPath(path);
    // FIXME: Explicit return type here is a workaround for a deficiency in the Windows compiler at the time of this writing.
#if PLATFORM(WKC)
    perform([capturedPath, offset, length](FileStream& stream) -> WTF::Function<void(FileStreamClient&)> {
#else
    perform([capturedPath, offset, length](FileStream& stream) -> std::function<void(FileStreamClient&)> {
#endif
        bool success = stream.openForRead(capturedPath.string(), offset, length);
        return [success](FileStreamClient& client) {
            client.didOpen(success);
        };
    });
}

void AsyncFileStream::openForWrite(const String& path)
{
    StringCapture capturedPath(path);
#if PLATFORM(WKC)
    perform([capturedPath](FileStream& stream) -> WTF::Function<void(FileStreamClient&)> {
#else
    perform([capturedPath](FileStream& stream) -> std::function<void(FileStreamClient&)> {
#endif
        bool success = stream.openForWrite(capturedPath.string());
        return [success](FileStreamClient& client) {
            client.didOpen(success);
        };
    });
}

void AsyncFileStream::close()
{
    auto& internals = *m_internals;
    callOnFileThread([&internals] {
        internals.stream.close();
    });
}

void AsyncFileStream::read(char* buffer, int length)
{
#if PLATFORM(WKC)
    perform([buffer, length](FileStream& stream) -> WTF::Function<void(FileStreamClient&)> {
#else
    perform([buffer, length](FileStream& stream) -> std::function<void(FileStreamClient&)> {
#endif
        int bytesRead = stream.read(buffer, length);
        return [bytesRead](FileStreamClient& client) {
            client.didRead(bytesRead);
        };
    });
}

void AsyncFileStream::write(const URL& blobURL, long long position, int length)
{
    URLCapture capturedURL(blobURL);
#if PLATFORM(WKC)
    perform([capturedURL, position, length](FileStream& stream) -> WTF::Function<void(FileStreamClient&)> {
#else
    perform([capturedURL, position, length](FileStream& stream) -> std::function<void(FileStreamClient&)> {
#endif
        int bytesWritten = stream.write(capturedURL.url(), position, length);
        return [bytesWritten](FileStreamClient& client) {
            client.didWrite(bytesWritten);
        };
    });
}

void AsyncFileStream::truncate(long long position)
{
#if PLATFORM(WKC)
    perform([position](FileStream& stream) -> WTF::Function<void(FileStreamClient&)> {
#else
    perform([position](FileStream& stream) -> std::function<void(FileStreamClient&)> {
#endif
        bool success = stream.truncate(position);
        return [success](FileStreamClient& client) {
            client.didTruncate(success);
        };
    });
}

} // namespace WebCore
