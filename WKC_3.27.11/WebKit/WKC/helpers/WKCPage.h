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

#ifndef _WKC_PAGE_H_
#define _WKC_PAGE_H_

#include <wkc/wkcbase.h>

#include "helpers/WKCHelpersEnums.h"

namespace WKC {
class FocusController;
class BackForwardList;
class Frame;
class String;
class HistoryItem;

class PagePrivate;

class WKC_API Page {
public:
    FocusController* focusController() const;
    BackForwardList* backForwardList() const;
    Frame* mainFrame() const;
    void setOpenedByDOM();
    void goToItem(HistoryItem*, FrameLoadType);
    void setGroupName(const String& name);
    const String& groupName() const;
    float deviceScaleFactor() const;
    void setDeviceScaleFactor(float);
    void updateRendering();
    void isolatedUpdateRendering();
    void finalizeRenderingUpdate();
    void accessibilitySettingsDidCange();

    void clear();

    PagePrivate& priv() const { return m_private; }

protected:
    // Applications must not create/destroy WKC helper instances by new/delete.
    // Or, it causes memory leaks or crashes.
    Page(PagePrivate&);
    ~Page();

private:
    Page(const Page&);
    Page& operator=(const Page&);

    PagePrivate& m_private;
};
} // namespace

#endif // _WKC_PAGE_H_
