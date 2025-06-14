/*
 * Copyright (C) 2010 Andras Becsi <abecsi@inf.u-szeged.hu>, University of Szeged
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

#ifndef HashTools_h
#define HashTools_h

#include "config.h"

namespace WebCore {

struct NamedColor {
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
public:
#endif
    const char* name;
    unsigned ARGBValue;
};

struct Property {
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
public:
#endif
    const char* name;
    int id;
};

struct Value {
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
public:
#endif
    const char* name;
    int id;
};

#if PLATFORM(WKC)
const NamedColor* findColor(const char* str, unsigned int len);
const Property* findProperty(const char* str, unsigned int len);
const Value* findValue(const char* str, unsigned int len);
#else
const NamedColor* findColor(register const char* str, register unsigned int len);
const Property* findProperty(register const char* str, register unsigned int len);
const Value* findValue(register const char* str, register unsigned int len);
#endif

} // namespace WebCore

#endif // HashTools_h
