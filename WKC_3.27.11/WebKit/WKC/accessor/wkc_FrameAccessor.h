﻿/*
 *  wkc_FrameAccessor.h
 *
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
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifdef WKC_CUSTOMER_PATCH_0304674

#pragma once

namespace WebCore {

class Frame;
class FrameTree;

} // namespace WebCore

namespace WKC {

class FrameAccessor
{
public:
    FrameAccessor( WebCore::Frame* obj )
    : m_webcore( obj )
    {}

    WebCore::Frame*       ptr()       { return static_cast<WebCore::Frame*>( m_webcore ); }
    const WebCore::Frame* ptr() const { return static_cast<const WebCore::Frame*>( m_webcore ); }

    WebCore::FrameTree* tree() const;

private:
    void* m_webcore;

};

} // namespace WKC

#endif // WKC_CUSTOMER_PATCH_0304674
