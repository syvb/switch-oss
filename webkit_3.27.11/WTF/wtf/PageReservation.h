/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#include <wtf/PageAllocation.h>

namespace WTF {

/*
    PageReservation

    Like PageAllocation, the PageReservation class provides a cross-platform memory
    allocation interface, but with a set of capabilities more similar to that of
    VirtualAlloc than posix mmap.  PageReservation can be used to allocate virtual
    memory without committing physical memory pages using PageReservation::reserve.
    Following a call to reserve all memory in the region is in a decommited state,
    in which the memory should not be used (accessing the memory may cause a fault).

    Before using memory it must be committed by calling commit, which is passed start
    and size values (both of which require system page size granularity).  One the
    committed memory is no longer needed 'decommit' may be called to return the
    memory to its devommitted state.  Commit should only be called on memory that is
    currently decommitted, and decommit should only be called on memory regions that
    are currently committed.  All memory should be decommited before the reservation
    is deallocated.  Values in memory may not be retained accross a pair of calls if
    the region of memory is decommitted and then committed again.

    Memory protection should not be changed on decommitted memory, and if protection
    is changed on memory while it is committed it should be returned to the orignal
    protection before decommit is called.
*/

class PageReservation : private PageBlock {
public:
    PageReservation() = default;

    using PageBlock::base;
    using PageBlock::size;
    using PageBlock::operator bool;

    void commit(void* start, size_t size)
    {
        ASSERT(*this);
        ASSERT(isPageAligned(start));
        ASSERT(isPageAligned(size));
        ASSERT(contains(start, size));

        m_committed += size;
        OSAllocator::commit(start, size, m_writable, m_executable);
    }

    void decommit(void* start, size_t size)
    {
        ASSERT(*this);
        ASSERT(isPageAligned(start));
        ASSERT(isPageAligned(size));
        ASSERT(contains(start, size));

        m_committed -= size;
        OSAllocator::decommit(start, size);
    }

    size_t committed()
    {
        return m_committed;
    }

    static PageReservation reserve(size_t size, OSAllocator::Usage usage = OSAllocator::UnknownUsage, bool writable = true, bool executable = false, bool jitCageEnabled = false)
    {
        ASSERT(isPageAligned(size));
#if PLATFORM(WKC)
        void* p = OSAllocator::reserveUncommitted(size, usage, writable, executable, jitCageEnabled, false);
        if (p)
            PageBlock::changeReservedSize(static_cast<long>(size), usage);
        return PageReservation(p, size, writable, executable, jitCageEnabled, false, usage);
#else
        return PageReservation(OSAllocator::reserveUncommitted(size, usage, writable, executable, jitCageEnabled, false), size, writable, executable, jitCageEnabled, false);
#endif
    }
    
    static PageReservation reserveWithGuardPages(size_t size, OSAllocator::Usage usage = OSAllocator::UnknownUsage, bool writable = true, bool executable = false, bool jitCageEnabled = false)
    {
        ASSERT(isPageAligned(size));
#if PLATFORM(WKC)
        void* p = OSAllocator::reserveUncommitted(size + pageSize() * 2, usage, writable, executable, jitCageEnabled, true);
        if (p)
            PageBlock::changeReservedSize(static_cast<long>(size + pageSize() * 2), usage);
        return PageReservation(p, size, writable, executable, jitCageEnabled, true, usage);
#else
        return PageReservation(OSAllocator::reserveUncommitted(size + pageSize() * 2, usage, writable, executable, jitCageEnabled, true), size, writable, executable, jitCageEnabled, true);
#endif
    }

    static PageReservation reserveAndCommitWithGuardPages(size_t size, OSAllocator::Usage usage = OSAllocator::UnknownUsage, bool writable = true, bool executable = false, bool jitCageEnabled = false)
    {
        ASSERT(isPageAligned(size));
#if PLATFORM(WKC)
        void* p = OSAllocator::reserveUncommitted(size + pageSize() * 2, usage, writable, executable, jitCageEnabled, true);
        if (p)
            PageBlock::changeReservedSize(static_cast<long>(size + pageSize() * 2), usage);
        return PageReservation(p, size, writable, executable, jitCageEnabled, true, usage);
#else
        return PageReservation(OSAllocator::reserveAndCommit(size + pageSize() * 2, usage, writable, executable, jitCageEnabled, true), size, writable, executable, jitCageEnabled, true);
#endif
    }

    void deallocate()
    {
        ASSERT(!m_committed);

#if PLATFORM(WKC)
        size_t guardPageSize = hasGuardPages() ? pageSize() * 2 : 0;
        PageBlock::changeReservedSize(-(static_cast<long>(size() + guardPageSize)), usage());
#endif

        // Clear base & size before calling release; if this is *inside* allocation
        // then we won't be able to clear then after deallocating the memory.
        PageReservation tmp;
        std::swap(tmp, *this);

        ASSERT(tmp);
        ASSERT(!*this);

        OSAllocator::releaseDecommitted(tmp.base(), tmp.size());
    }

private:
#if PLATFORM(WKC)
    PageReservation(void* base, size_t size, bool writable, bool executable, bool, bool hasGuardPages, OSAllocator::Usage usage)
        : PageBlock(base, size, hasGuardPages, usage)
        , m_committed(0)
        , m_writable(writable)
        , m_executable(executable)
    {
    }
#else
    PageReservation(void* base, size_t size, bool writable, bool executable, bool, bool hasGuardPages)
        : PageBlock(base, size, hasGuardPages)
        , m_committed(0)
        , m_writable(writable)
        , m_executable(executable)
    {
    }
#endif

    size_t m_committed { 0 };
    bool m_writable { false };
    bool m_executable { false };
};

}

using WTF::PageReservation;
