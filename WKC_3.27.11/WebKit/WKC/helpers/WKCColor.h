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

#ifndef _WKC_HELPERS_WKC_COLOR_H_
#define _WKC_HELPERS_WKC_COLOR_H_

#include <wkc/wkcbase.h>

namespace WKC {

typedef unsigned RGBA32;
typedef unsigned ARGB32;

class ColorPrivate;

class WKC_API Color {
public:
    Color();
    ~Color();

    Color(ColorPrivate*); // Not for applications, only for WKC.
    Color(const Color&);
    Color(Color&&);
    Color& operator=(const Color&);
    Color& operator=(Color&&);

    bool isValid() const;

    bool isOpaque() const;

    int red() const;
    int green() const;
    int blue() const;
    int alpha() const;

    RGBA32 rgba() const;
    ARGB32 argb() const;

    ColorPrivate* parent() const { return m_parent; }

private:
    // Heap allocation by operator new/delete is disallowed.
    // This restriction is to avoid memory leaks or crashes.
    void* operator new(size_t);
    void* operator new[](size_t);
    void operator delete(void*);
    void operator delete[](void*);

    ColorPrivate* m_parent;
};

} // namespace

#endif // _WKC_HELPERS_WKC_COLOR_H_
