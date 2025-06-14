/*
 * Copyright (C) 2011 Google Inc.
 * Copyright (C) 2017 Yusuke Suzuki <utatane.tea@gmail.com>
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
 * THIS SOFTWARE IS PROVIDED BY GOOGLE, INC. ``AS IS'' AND ANY
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
#include <wtf/RandomDevice.h>

#include <stdlib.h>

#if !OS(DARWIN) && !OS(FUCHSIA) && OS(UNIX)
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#if OS(WINDOWS)
#include <windows.h>
#include <wincrypt.h> // windows.h must be included before wincrypt.h.
#endif

#if OS(DARWIN)
#include <CommonCrypto/CommonRandom.h>
#endif

#if OS(FUCHSIA)
#include <zircon/syscalls.h>
#endif

#if PLATFORM(WKC)
#include <wkc/wkcpeer.h>
#endif

namespace WTF {

#if !OS(DARWIN) && !OS(FUCHSIA) && OS(UNIX)
NEVER_INLINE NO_RETURN_DUE_TO_CRASH static void crashUnableToOpenURandom()
{
    CRASH();
}

NEVER_INLINE NO_RETURN_DUE_TO_CRASH static void crashUnableToReadFromURandom()
{
    CRASH();
}
#endif

#if !OS(DARWIN) && !OS(FUCHSIA) && !OS(WINDOWS) && !PLATFORM(WKC)
RandomDevice::RandomDevice()
{
    int ret = 0;
    do {
        ret = open("/dev/urandom", O_RDONLY, 0);
    } while (ret == -1 && errno == EINTR);
    m_fd = ret;
    if (m_fd < 0)
        crashUnableToOpenURandom(); // We need /dev/urandom for this API to work...
}
#endif

#if !OS(DARWIN) && !OS(FUCHSIA) && !OS(WINDOWS) && !PLATFORM(WKC)
RandomDevice::~RandomDevice()
{
    close(m_fd);
}
#endif

// FIXME: Make this call fast by creating the pool in RandomDevice.
// https://bugs.webkit.org/show_bug.cgi?id=170190
void RandomDevice::cryptographicallyRandomValues(unsigned char* buffer, size_t length)
{
#if OS(DARWIN)
    RELEASE_ASSERT(!CCRandomGenerateBytes(buffer, length));
#elif OS(FUCHSIA)
    zx_cprng_draw(buffer, length);
#elif OS(UNIX)
    ssize_t amountRead = 0;
    while (static_cast<size_t>(amountRead) < length) {
        ssize_t currentRead = read(m_fd, buffer + amountRead, length - amountRead);
        // We need to check for both EAGAIN and EINTR since on some systems /dev/urandom
        // is blocking and on others it is non-blocking.
        if (currentRead == -1) {
            if (!(errno == EAGAIN || errno == EINTR))
                crashUnableToReadFromURandom();
        } else
            amountRead += currentRead;
    }
#elif OS(WINDOWS)
    // FIXME: We cannot ensure that Cryptographic Service Provider context and CryptGenRandom are safe across threads.
    // If it is safe, we can acquire context per RandomDevice.
    HCRYPTPROV hCryptProv = 0;
    if (!CryptAcquireContext(&hCryptProv, nullptr, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
        CRASH();
    if (!CryptGenRandom(hCryptProv, length, buffer))
        CRASH();
    CryptReleaseContext(hCryptProv, 0);
#elif PLATFORM(WKC)
    if (!wkcCryptographicallyRandomValuesPeer(buffer, length))
        CRASH();
#else
#error "This configuration doesn't have a strong source of randomness."
// WARNING: When adding new sources of OS randomness, the randomness must
//          be of cryptographic quality!
#endif
}

}
