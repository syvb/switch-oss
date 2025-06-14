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

#ifndef _WKC_HELPERS_PRIVATE_ContextMenuItemITEM_H_
#define _WKC_HELPERS_PRIVATE_ContextMenuItemITEM_H_

#include "helpers/WKCContextMenuItem.h"

namespace WebCore {
class ContextMenuItem;
} // namespace

namespace WKC {

class ContextMenuItemWrap : public ContextMenuItem {
WTF_MAKE_FAST_ALLOCATED;
public:
    ContextMenuItemWrap(ContextMenuItemPrivate& parent) : ContextMenuItem(parent) {}
    ~ContextMenuItemWrap() {}
};

class ContextMenuItemPrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    ContextMenuItemPrivate(WebCore::ContextMenuItem*);
    ~ContextMenuItemPrivate();

    WebCore::ContextMenuItem* webcore() const { return m_webcore; }
    ContextMenuItem& wkc() { return m_wkc; }

private:
    WebCore::ContextMenuItem* m_webcore;
    ContextMenuItemWrap m_wkc;

};
} // namespace

#endif // _WKC_HELPERS_PRIVATE_ContextMenuItemITEM_H_

