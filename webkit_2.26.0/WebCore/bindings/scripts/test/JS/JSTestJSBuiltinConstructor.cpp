/*
    This file is part of the WebKit open source project.
    This file has been generated by generate-bindings.pl. DO NOT MODIFY!

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "JSTestJSBuiltinConstructor.h"

#include "JSDOMAttribute.h"
#include "JSDOMBinding.h"
#include "JSDOMBuiltinConstructor.h"
#include "JSDOMExceptionHandling.h"
#include "JSDOMOperation.h"
#include "JSDOMWrapperCache.h"
#include "TestJSBuiltinConstructorBuiltins.h"
#include <JavaScriptCore/FunctionPrototype.h>
#include <JavaScriptCore/JSCInlines.h>
#include <wtf/GetPtr.h>
#include <wtf/PointerPreparations.h>


namespace WebCore {
using namespace JSC;

// Functions

JSC::EncodedJSValue JSC_HOST_CALL jsTestJSBuiltinConstructorPrototypeFunctionTestCustomFunction(JSC::JSGlobalObject*, JSC::CallFrame*);

// Attributes

JSC::EncodedJSValue jsTestJSBuiltinConstructorConstructor(JSC::JSGlobalObject*, JSC::EncodedJSValue, JSC::PropertyName);
bool setJSTestJSBuiltinConstructorConstructor(JSC::JSGlobalObject*, JSC::EncodedJSValue, JSC::EncodedJSValue);
JSC::EncodedJSValue jsTestJSBuiltinConstructorTestAttributeCustom(JSC::JSGlobalObject*, JSC::EncodedJSValue, JSC::PropertyName);
JSC::EncodedJSValue jsTestJSBuiltinConstructorTestAttributeRWCustom(JSC::JSGlobalObject*, JSC::EncodedJSValue, JSC::PropertyName);
bool setJSTestJSBuiltinConstructorTestAttributeRWCustom(JSC::JSGlobalObject*, JSC::EncodedJSValue, JSC::EncodedJSValue);

class JSTestJSBuiltinConstructorPrototype : public JSC::JSNonFinalObject {
public:
    using Base = JSC::JSNonFinalObject;
    static JSTestJSBuiltinConstructorPrototype* create(JSC::VM& vm, JSDOMGlobalObject* globalObject, JSC::Structure* structure)
    {
        JSTestJSBuiltinConstructorPrototype* ptr = new (NotNull, JSC::allocateCell<JSTestJSBuiltinConstructorPrototype>(vm.heap)) JSTestJSBuiltinConstructorPrototype(vm, globalObject, structure);
        ptr->finishCreation(vm);
        return ptr;
    }

    DECLARE_INFO;
    static JSC::Structure* createStructure(JSC::VM& vm, JSC::JSGlobalObject* globalObject, JSC::JSValue prototype)
    {
        return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), info());
    }

private:
    JSTestJSBuiltinConstructorPrototype(JSC::VM& vm, JSC::JSGlobalObject*, JSC::Structure* structure)
        : JSC::JSNonFinalObject(vm, structure)
    {
    }

    void finishCreation(JSC::VM&);
};
STATIC_ASSERT_ISO_SUBSPACE_SHARABLE(JSTestJSBuiltinConstructorPrototype, JSTestJSBuiltinConstructorPrototype::Base);

using JSTestJSBuiltinConstructorConstructor = JSDOMBuiltinConstructor<JSTestJSBuiltinConstructor>;

template<> JSValue JSTestJSBuiltinConstructorConstructor::prototypeForStructure(JSC::VM& vm, const JSDOMGlobalObject& globalObject)
{
    UNUSED_PARAM(vm);
    return globalObject.functionPrototype();
}

template<> void JSTestJSBuiltinConstructorConstructor::initializeProperties(VM& vm, JSDOMGlobalObject& globalObject)
{
    putDirect(vm, vm.propertyNames->prototype, JSTestJSBuiltinConstructor::prototype(vm, globalObject), JSC::PropertyAttribute::DontDelete | JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::DontEnum);
    putDirect(vm, vm.propertyNames->name, jsNontrivialString(vm, String("TestJSBuiltinConstructor"_s)), JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::DontEnum);
    putDirect(vm, vm.propertyNames->length, jsNumber(0), JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::DontEnum);
}

template<> FunctionExecutable* JSTestJSBuiltinConstructorConstructor::initializeExecutable(VM& vm)
{
    return testJSBuiltinConstructorInitializeTestJSBuiltinConstructorCodeGenerator(vm);
}

template<> const ClassInfo JSTestJSBuiltinConstructorConstructor::s_info = { "TestJSBuiltinConstructor", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(JSTestJSBuiltinConstructorConstructor) };

/* Hash table for prototype */

static const HashTableValue JSTestJSBuiltinConstructorPrototypeTableValues[] =
{
    { "constructor", static_cast<unsigned>(JSC::PropertyAttribute::DontEnum), NoIntrinsic, { (intptr_t)static_cast<PropertySlot::GetValueFunc>(jsTestJSBuiltinConstructorConstructor), (intptr_t) static_cast<PutPropertySlot::PutValueFunc>(setJSTestJSBuiltinConstructorConstructor) } },
    { "testAttribute", static_cast<unsigned>(JSC::PropertyAttribute::Accessor | JSC::PropertyAttribute::Builtin), NoIntrinsic, { (intptr_t)static_cast<BuiltinGenerator>(testJSBuiltinConstructorTestAttributeCodeGenerator), (intptr_t) (setTestJSBuiltinConstructorTestAttributeCodeGenerator) } },
    { "testAttributeCustom", static_cast<unsigned>(JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::CustomAccessor | JSC::PropertyAttribute::DOMAttribute), NoIntrinsic, { (intptr_t)static_cast<PropertySlot::GetValueFunc>(jsTestJSBuiltinConstructorTestAttributeCustom), (intptr_t) static_cast<PutPropertySlot::PutValueFunc>(0) } },
    { "testAttributeRWCustom", static_cast<unsigned>(JSC::PropertyAttribute::CustomAccessor | JSC::PropertyAttribute::DOMAttribute), NoIntrinsic, { (intptr_t)static_cast<PropertySlot::GetValueFunc>(jsTestJSBuiltinConstructorTestAttributeRWCustom), (intptr_t) static_cast<PutPropertySlot::PutValueFunc>(setJSTestJSBuiltinConstructorTestAttributeRWCustom) } },
    { "testFunction", static_cast<unsigned>(JSC::PropertyAttribute::Builtin), NoIntrinsic, { (intptr_t)static_cast<BuiltinGenerator>(testJSBuiltinConstructorTestFunctionCodeGenerator), (intptr_t) (0) } },
    { "testCustomFunction", static_cast<unsigned>(JSC::PropertyAttribute::Function), NoIntrinsic, { (intptr_t)static_cast<RawNativeFunction>(jsTestJSBuiltinConstructorPrototypeFunctionTestCustomFunction), (intptr_t) (0) } },
};

const ClassInfo JSTestJSBuiltinConstructorPrototype::s_info = { "TestJSBuiltinConstructorPrototype", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(JSTestJSBuiltinConstructorPrototype) };

void JSTestJSBuiltinConstructorPrototype::finishCreation(VM& vm)
{
    Base::finishCreation(vm);
    reifyStaticProperties(vm, JSTestJSBuiltinConstructor::info(), JSTestJSBuiltinConstructorPrototypeTableValues, *this);
}

const ClassInfo JSTestJSBuiltinConstructor::s_info = { "TestJSBuiltinConstructor", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(JSTestJSBuiltinConstructor) };

JSTestJSBuiltinConstructor::JSTestJSBuiltinConstructor(Structure* structure, JSDOMGlobalObject& globalObject)
    : JSDOMObject(structure, globalObject) { }

void JSTestJSBuiltinConstructor::finishCreation(VM& vm)
{
    Base::finishCreation(vm);
    ASSERT(inherits(vm, info()));

}

JSObject* JSTestJSBuiltinConstructor::createPrototype(VM& vm, JSDOMGlobalObject& globalObject)
{
    return JSTestJSBuiltinConstructorPrototype::create(vm, &globalObject, JSTestJSBuiltinConstructorPrototype::createStructure(vm, &globalObject, globalObject.objectPrototype()));
}

JSObject* JSTestJSBuiltinConstructor::prototype(VM& vm, JSDOMGlobalObject& globalObject)
{
    return getDOMPrototype<JSTestJSBuiltinConstructor>(vm, globalObject);
}

JSValue JSTestJSBuiltinConstructor::getConstructor(VM& vm, const JSGlobalObject* globalObject)
{
    return getDOMConstructor<JSTestJSBuiltinConstructorConstructor>(vm, *jsCast<const JSDOMGlobalObject*>(globalObject));
}

void JSTestJSBuiltinConstructor::destroy(JSC::JSCell* cell)
{
    JSTestJSBuiltinConstructor* thisObject = static_cast<JSTestJSBuiltinConstructor*>(cell);
    thisObject->JSTestJSBuiltinConstructor::~JSTestJSBuiltinConstructor();
}

template<> inline JSTestJSBuiltinConstructor* IDLAttribute<JSTestJSBuiltinConstructor>::cast(JSGlobalObject& lexicalGlobalObject, EncodedJSValue thisValue)
{
    return jsDynamicCast<JSTestJSBuiltinConstructor*>(JSC::getVM(&lexicalGlobalObject), JSValue::decode(thisValue));
}

template<> inline JSTestJSBuiltinConstructor* IDLOperation<JSTestJSBuiltinConstructor>::cast(JSGlobalObject& lexicalGlobalObject, CallFrame& callFrame)
{
    return jsDynamicCast<JSTestJSBuiltinConstructor*>(JSC::getVM(&lexicalGlobalObject), callFrame.thisValue());
}

EncodedJSValue jsTestJSBuiltinConstructorConstructor(JSGlobalObject* lexicalGlobalObject, EncodedJSValue thisValue, PropertyName)
{
    VM& vm = JSC::getVM(lexicalGlobalObject);
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    auto* prototype = jsDynamicCast<JSTestJSBuiltinConstructorPrototype*>(vm, JSValue::decode(thisValue));
    if (UNLIKELY(!prototype))
        return throwVMTypeError(lexicalGlobalObject, throwScope);
    return JSValue::encode(JSTestJSBuiltinConstructor::getConstructor(JSC::getVM(lexicalGlobalObject), prototype->globalObject()));
}

bool setJSTestJSBuiltinConstructorConstructor(JSGlobalObject* lexicalGlobalObject, EncodedJSValue thisValue, EncodedJSValue encodedValue)
{
    VM& vm = JSC::getVM(lexicalGlobalObject);
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    auto* prototype = jsDynamicCast<JSTestJSBuiltinConstructorPrototype*>(vm, JSValue::decode(thisValue));
    if (UNLIKELY(!prototype)) {
        throwVMTypeError(lexicalGlobalObject, throwScope);
        return false;
    }
    // Shadowing a built-in constructor
    return prototype->putDirect(vm, vm.propertyNames->constructor, JSValue::decode(encodedValue));
}

static inline JSValue jsTestJSBuiltinConstructorTestAttributeCustomGetter(JSGlobalObject& lexicalGlobalObject, JSTestJSBuiltinConstructor& thisObject, ThrowScope& throwScope)
{
    UNUSED_PARAM(throwScope);
    UNUSED_PARAM(lexicalGlobalObject);
    return thisObject.testAttributeCustom(lexicalGlobalObject);
}

EncodedJSValue jsTestJSBuiltinConstructorTestAttributeCustom(JSGlobalObject* lexicalGlobalObject, EncodedJSValue thisValue, PropertyName)
{
    return IDLAttribute<JSTestJSBuiltinConstructor>::get<jsTestJSBuiltinConstructorTestAttributeCustomGetter, CastedThisErrorBehavior::Assert>(*lexicalGlobalObject, thisValue, "testAttributeCustom");
}

static inline JSValue jsTestJSBuiltinConstructorTestAttributeRWCustomGetter(JSGlobalObject& lexicalGlobalObject, JSTestJSBuiltinConstructor& thisObject, ThrowScope& throwScope)
{
    UNUSED_PARAM(throwScope);
    UNUSED_PARAM(lexicalGlobalObject);
    return thisObject.testAttributeRWCustom(lexicalGlobalObject);
}

EncodedJSValue jsTestJSBuiltinConstructorTestAttributeRWCustom(JSGlobalObject* lexicalGlobalObject, EncodedJSValue thisValue, PropertyName)
{
    return IDLAttribute<JSTestJSBuiltinConstructor>::get<jsTestJSBuiltinConstructorTestAttributeRWCustomGetter, CastedThisErrorBehavior::Assert>(*lexicalGlobalObject, thisValue, "testAttributeRWCustom");
}

static inline bool setJSTestJSBuiltinConstructorTestAttributeRWCustomSetter(JSGlobalObject& lexicalGlobalObject, JSTestJSBuiltinConstructor& thisObject, JSValue value, ThrowScope& throwScope)
{
    UNUSED_PARAM(lexicalGlobalObject);
    UNUSED_PARAM(throwScope);
    thisObject.setTestAttributeRWCustom(lexicalGlobalObject, value);
    return true;
}

bool setJSTestJSBuiltinConstructorTestAttributeRWCustom(JSGlobalObject* lexicalGlobalObject, EncodedJSValue thisValue, EncodedJSValue encodedValue)
{
    return IDLAttribute<JSTestJSBuiltinConstructor>::set<setJSTestJSBuiltinConstructorTestAttributeRWCustomSetter>(*lexicalGlobalObject, thisValue, encodedValue, "testAttributeRWCustom");
}

static inline JSC::EncodedJSValue jsTestJSBuiltinConstructorPrototypeFunctionTestCustomFunctionBody(JSC::JSGlobalObject* lexicalGlobalObject, JSC::CallFrame* callFrame, typename IDLOperation<JSTestJSBuiltinConstructor>::ClassParameter castedThis, JSC::ThrowScope& throwScope)
{
    UNUSED_PARAM(lexicalGlobalObject);
    UNUSED_PARAM(callFrame);
    UNUSED_PARAM(throwScope);
    return JSValue::encode(castedThis->testCustomFunction(*lexicalGlobalObject, *callFrame));
}

EncodedJSValue JSC_HOST_CALL jsTestJSBuiltinConstructorPrototypeFunctionTestCustomFunction(JSGlobalObject* lexicalGlobalObject, CallFrame* callFrame)
{
    return IDLOperation<JSTestJSBuiltinConstructor>::call<jsTestJSBuiltinConstructorPrototypeFunctionTestCustomFunctionBody>(*lexicalGlobalObject, *callFrame, "testCustomFunction");
}


}