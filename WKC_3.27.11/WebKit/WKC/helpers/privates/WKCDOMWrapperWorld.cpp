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

#include "helpers/WKCDOMWrapperWorld.h"
#include "helpers/privates/WKCDOMWrapperWorldPrivate.h"

namespace WKC {

DOMWrapperWorldPrivate::DOMWrapperWorldPrivate(WebCore::DOMWrapperWorld* parent)
    : m_webcore(parent)
    , m_wkc(*this)
{
}

DOMWrapperWorldPrivate::~DOMWrapperWorldPrivate()
{
}

////////////////////////////////////////////////////////////////////////////////

DOMWrapperWorld::DOMWrapperWorld(DOMWrapperWorldPrivate& parent)
    : m_private(parent)
{
}

DOMWrapperWorld::~DOMWrapperWorld()
{
}

} // namespace
