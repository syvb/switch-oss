/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) 2010, 2012, 2014 Apple Inc. All rights reserved.
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

#ifndef AsyncFileStream_h
#define AsyncFileStream_h

#include <functional>
#if PLATFORM(WKC)
#include <wtf/Function.h>
#endif
#include <wtf/Forward.h>

namespace WebCore {

class FileStreamClient;
class FileStream;
class URL;

class AsyncFileStream {
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
#endif
public:
    explicit AsyncFileStream(FileStreamClient&);
    ~AsyncFileStream();

    void getSize(const String& path, double expectedModificationTime);
    void openForRead(const String& path, long long offset, long long length);
    void openForWrite(const String& path);
    void close();
    void read(char* buffer, int length);
    void write(const URL& blobURL, long long position, int length);
    void truncate(long long position);

private:
    void start();
#if PLATFORM(WKC)
    void perform(WTF::Function<WTF::Function<void(FileStreamClient&)>(FileStream&)>);
#else
    void perform(std::function<std::function<void(FileStreamClient&)>(FileStream&)>);
#endif

#if PLATFORM(WKC)
public:
#endif
    struct Internals;
#if PLATFORM(WKC)
private:
#endif
    std::unique_ptr<Internals> m_internals;
};

} // namespace WebCore

#endif // AsyncFileStream_h
