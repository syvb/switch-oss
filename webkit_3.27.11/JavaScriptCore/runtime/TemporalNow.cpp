/*
 *  Copyright (C) 2021 Igalia S.L. All rights reserved.
 *  Copyright (C) 2021 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "config.h"
#include "TemporalNow.h"

#include "JSCJSValueInlines.h"
#include "JSGlobalObject.h"
#include "JSObjectInlines.h"
#include "ObjectPrototype.h"
#include "TemporalTimeZone.h"

namespace JSC {

STATIC_ASSERT_IS_TRIVIALLY_DESTRUCTIBLE(TemporalNow);

static JSC_DECLARE_HOST_FUNCTION(temporalNowFuncTimeZone);

} // namespace JSC

#include "TemporalNow.lut.h"

namespace JSC {

/* Source for TemporalNow.lut.h
@begin temporalNowTable
    timeZone        temporalNowFuncTimeZone     DontEnum|Function 0
@end
*/

const ClassInfo TemporalNow::s_info = { "Temporal.Now", &Base::s_info, &temporalNowTable, nullptr, CREATE_METHOD_TABLE(TemporalNow) };

TemporalNow::TemporalNow(VM& vm, Structure* structure)
    : Base(vm, structure)
{
}

TemporalNow* TemporalNow::create(VM& vm, Structure* structure)
{
    TemporalNow* object = new (NotNull, allocateCell<TemporalNow>(vm)) TemporalNow(vm, structure);
    object->finishCreation(vm);
    return object;
}

Structure* TemporalNow::createStructure(VM& vm, JSGlobalObject* globalObject)
{
    return Structure::create(vm, globalObject, globalObject->objectPrototype(), TypeInfo(ObjectType, StructureFlags), info());
}

void TemporalNow::finishCreation(VM& vm)
{
    Base::finishCreation(vm);
    ASSERT(inherits(vm, info()));
    JSC_TO_STRING_TAG_WITHOUT_TRANSITION();
}

// https://tc39.es/proposal-temporal/#sec-temporal.now.timezone
// https://tc39.es/proposal-temporal/#sec-temporal-systemtimezone
JSC_DEFINE_HOST_FUNCTION(temporalNowFuncTimeZone, (JSGlobalObject* globalObject, CallFrame*))
{
    VM& vm = globalObject->vm();

#if ENABLE(WKC_INTL)
    String timeZoneString = vm.dateCache.defaultTimeZone();
#else
    String timeZoneString = "UTC"_s;
#endif

    std::optional<TimeZoneID> identifier = ISO8601::parseTimeZoneName(timeZoneString);
    if (!identifier)
        return JSValue::encode(TemporalTimeZone::createFromUTCOffset(vm, globalObject->timeZoneStructure(), 0));
    return JSValue::encode(TemporalTimeZone::createFromID(vm, globalObject->timeZoneStructure(), identifier.value()));
}

} // namespace JSC
