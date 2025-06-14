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

#include "helpers/WKCFormState.h"
#include "helpers/privates/WKCFormStatePrivate.h"
#include "helpers/privates/WKCHTMLFormElementPrivate.h"

#include "FormState.h"

namespace WKC {

FormStatePrivate::FormStatePrivate(WebCore::FormState* parent)
    : m_webcore(parent)
    , m_wkc(*this)
    , m_formElement(0)
{
}

FormStatePrivate::~FormStatePrivate()
{
    delete m_formElement;
}

HTMLFormElement*
FormStatePrivate::form()
{
    WebCore::HTMLFormElement& elem = webcore()->form();

    delete m_formElement;
    m_formElement = new HTMLFormElementPrivate(&elem);
    return &m_formElement->wkc();
}

////////////////////////////////////////////////////////////////////////////////

FormState::FormState(FormStatePrivate& parent)
    : m_private(parent)
{
}

FormState::~FormState()
{
}

HTMLFormElement*
FormState::form()
{
    return priv().form();
}

} // namespace
