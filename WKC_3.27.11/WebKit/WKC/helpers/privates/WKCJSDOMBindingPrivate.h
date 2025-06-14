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

#ifndef _WKC_HELPERS_PRIVATE_JSDOMBINDING_H_
#define _WKC_HELPERS_PRIVATE_JSDOMBINDING_H_

#include "helpers/WKCJSDOMBinding.h"

namespace WebCore {
class JSDOMBinding;
} // namespace

namespace WKC {
class JSDOMBindingPrivate {
public:
    JSDOMBindingPrivate(WebCore::JSDOMBinding*);
    ~JSDOMBindingPrivate();

    WebCore::JSDOMBinding* webcore() const { return m_webcore; }
    JSDOMBinding& wkc() { return m_wkc; }

private:
    WebCore::JSDOMBinding* m_webcore;
    JSDOMBinding m_wkc;

};
} // namespace

#endif // _WKC_HELPERS_PRIVATE_JSDOMBINDING_H_
