/*
 * Copyright (c) 2013-2023 ACCESS CO., LTD. All rights reserved.
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

#ifndef _WKC_HELPER_WKCTEXTTRACKCUE_H_
#define _WKC_HELPER_WKCTEXTTRACKCUE_H_

#include <wkc/wkcbase.h>
#include "helpers/WKCString.h"

namespace WKC {

class TextTrackCuePrivate;
class TextTrackCue;

class WKC_API TextTrackCue {
public:
    TextTrackCue(TextTrackCuePrivate&);
    ~TextTrackCue();

    const String& id() const;
    double startTime() const;
    double endTime() const;
    bool pauseOnExit() const;
    const String& vertical() const;
    bool snapToLines() const;
    int line() const;
    int position() const;
    int size() const;
    const String& align() const;
    const String& text() const;

private:
    TextTrackCuePrivate& m_private;
};
} // namespace

#endif // _WKC_HELPER_WKCTEXTTRACKCUE_H_
