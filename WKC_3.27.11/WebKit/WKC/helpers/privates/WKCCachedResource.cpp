/*
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "config.h"

#include "helpers/WKCCachedResource.h"
#include "helpers/privates/WKCCachedResourcePrivate.h"

#include "CachedResource.h"

namespace WKC {

CachedResourcePrivate::CachedResourcePrivate(WebCore::CachedResource* parent)
    : m_webcore(parent)
    , m_wkc(*this)
{
}

CachedResourcePrivate::~CachedResourcePrivate()
{
}

CachedResource::Status
CachedResourcePrivate::status() const
{
    return (CachedResource::Status)m_webcore->status();
}

////////////////////////////////////////////////////////////////////////////////

CachedResource::CachedResource(CachedResourcePrivate& parent)
    : m_private(parent)
{
}

CachedResource::~CachedResource()
{
}

CachedResource::Status
CachedResource::status() const
{
    return m_private.status();
}

} // namespace
