/*
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 */

#include "config.h"
#include "PublicSuffix.h"
#include "NotImplemented.h"

#if ENABLE(PUBLIC_SUFFIX_LIST)

namespace WebCore {

bool isPublicSuffix(const WTF::String& domain)
{
    notImplemented();
    return false;
}

WTF::String topPrivatelyControlledDomain(const WTF::String& domain)
{
    notImplemented();
    return WTF::String();
}

} // namespace WebCore

#endif // ENABLE(PUBLIC_SUFFIX_LIST)
