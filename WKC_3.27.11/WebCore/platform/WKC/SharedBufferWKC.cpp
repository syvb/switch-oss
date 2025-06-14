/*
 * Copyright (C) 2008 Collabora Ltd.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "SharedBuffer.h"

#include "CString.h"
#include "FileSystem.h"

#include <wkc/wkcfilepeer.h>

#ifdef __WKC_IMPLICIT_INCLUDE_SYSSTAT
#include <sys/stat.h>
#endif

namespace WebCore {

RefPtr<SharedBuffer>
SharedBuffer::createFromReadingFile(const String& filePath)
{
    if (filePath.isEmpty())
        return nullptr;

    CString filename = FileSystem::fileSystemRepresentation(filePath);
    void* fd = wkcFileFOpenPeer(WKC_FILEOPEN_USAGE_WEBCORE, filename.data(), "rb");
    if (!fd)
        return nullptr;

    struct stat fileStat;
    if (wkcFileFStatPeer(fd, &fileStat) == -1) {
        wkcFileFClosePeer(fd);
        return nullptr;
    }

    size_t bytesToRead;
    if (!WTF::convertSafely(fileStat.st_size, bytesToRead)) {
        wkcFileFClosePeer(fd);
        return nullptr;
    }

    Vector<uint8_t> buffer(bytesToRead);

    size_t totalBytesRead = 0;
    ssize_t bytesRead;
    while ((bytesRead = wkcFileFReadPeer(buffer.data() + totalBytesRead, 1, bytesToRead - totalBytesRead, fd)) > 0)
        totalBytesRead += bytesRead;

    wkcFileFClosePeer(fd);

    if (totalBytesRead != bytesToRead)
        return nullptr;

    return SharedBuffer::create(buffer.data(), buffer.size());
}

} // namespace WebCore
