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

#include "helpers/WKCHTMLFrameElement.h"
#include "helpers/privates/WKCHTMLFrameElementPrivate.h"

namespace WebCore {
class HTMLFrameElement;
class HTMLFrameOwnerElement;
} // namespace

namespace WKC {

HTMLFrameElementPrivate::HTMLFrameElementPrivate(WebCore::HTMLFrameElement* parent)
    : HTMLFrameOwnerElementPrivate(reinterpret_cast<WebCore::HTMLFrameOwnerElement*>(parent))
{
}

HTMLFrameElementPrivate::~HTMLFrameElementPrivate()
{
}

} // namespace
