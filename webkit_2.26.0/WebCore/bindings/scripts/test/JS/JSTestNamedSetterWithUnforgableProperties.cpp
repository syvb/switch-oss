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
#include "JSTestNamedSetterWithUnforgableProperties.h"

#include "ActiveDOMObject.h"
#include "JSDOMAbstractOperations.h"
#include "JSDOMAttribute.h"
#include "JSDOMBinding.h"
#include "JSDOMConstructorNotConstructable.h"
#include "JSDOMConvertStrings.h"
#include "JSDOMExceptionHandling.h"
#include "JSDOMOperation.h"
#include "JSDOMWrapperCache.h"
#include "ScriptExecutionContext.h"
#include <JavaScriptCore/FunctionPrototype.h>
#include <JavaScriptCore/HeapAnalyzer.h>
#include <JavaScriptCore/JSCInlines.h>
#include <wtf/GetPtr.h>
#include <wtf/PointerPreparations.h>
#include <wtf/URL.h>


namespace WebCore {
using namespace JSC;

// Functions

JSC::EncodedJSValue JSC_HOST_CALL jsTestNamedSetterWithUnforgablePropertiesInstanceFunctionUnforgeableOperation(JSC::JSGlobalObject*, JSC::CallFrame*);

// Attributes

JSC::EncodedJSValue jsTestNamedSetterWithUnforgablePropertiesConstructor(JSC::JSGlobalObject*, JSC::EncodedJSValue, JSC::PropertyName);
bool setJSTestNamedSetterWithUnforgablePropertiesConstructor(JSC::JSGlobalObject*, JSC::EncodedJSValue, JSC::EncodedJSValue);
JSC::EncodedJSValue jsTestNamedSetterWithUnforgablePropertiesUnforgeableAttribute(JSC::JSGlobalObject*, JSC::EncodedJSValue, JSC::PropertyName);

class JSTestNamedSetterWithUnforgablePropertiesPrototype : public JSC::JSNonFinalObject {
public:
    using Base = JSC::JSNonFinalObject;
    static JSTestNamedSetterWithUnforgablePropertiesPrototype* create(JSC::VM& vm, JSDOMGlobalObject* globalObject, JSC::Structure* structure)
    {
        JSTestNamedSetterWithUnforgablePropertiesPrototype* ptr = new (NotNull, JSC::allocateCell<JSTestNamedSetterWithUnforgablePropertiesPrototype>(vm.heap)) JSTestNamedSetterWithUnforgablePropertiesPrototype(vm, globalObject, structure);
        ptr->finishCreation(vm);
        return ptr;
    }

    DECLARE_INFO;
    static JSC::Structure* createStructure(JSC::VM& vm, JSC::JSGlobalObject* globalObject, JSC::JSValue prototype)
    {
        return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), info());
    }

private:
    JSTestNamedSetterWithUnforgablePropertiesPrototype(JSC::VM& vm, JSC::JSGlobalObject*, JSC::Structure* structure)
        : JSC::JSNonFinalObject(vm, structure)
    {
    }

    void finishCreation(JSC::VM&);
};
STATIC_ASSERT_ISO_SUBSPACE_SHARABLE(JSTestNamedSetterWithUnforgablePropertiesPrototype, JSTestNamedSetterWithUnforgablePropertiesPrototype::Base);

using JSTestNamedSetterWithUnforgablePropertiesConstructor = JSDOMConstructorNotConstructable<JSTestNamedSetterWithUnforgableProperties>;

/* Hash table */

static const struct CompactHashIndex JSTestNamedSetterWithUnforgablePropertiesTableIndex[4] = {
    { -1, -1 },
    { 1, -1 },
    { -1, -1 },
    { 0, -1 },
};


static const HashTableValue JSTestNamedSetterWithUnforgablePropertiesTableValues[] =
{
    { "unforgeableAttribute", static_cast<unsigned>(JSC::PropertyAttribute::DontDelete | JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::CustomAccessor | JSC::PropertyAttribute::DOMAttribute), NoIntrinsic, { (intptr_t)static_cast<PropertySlot::GetValueFunc>(jsTestNamedSetterWithUnforgablePropertiesUnforgeableAttribute), (intptr_t) static_cast<PutPropertySlot::PutValueFunc>(0) } },
    { "unforgeableOperation", static_cast<unsigned>(JSC::PropertyAttribute::DontDelete | JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::Function), NoIntrinsic, { (intptr_t)static_cast<RawNativeFunction>(jsTestNamedSetterWithUnforgablePropertiesInstanceFunctionUnforgeableOperation), (intptr_t) (0) } },
};

static const HashTable JSTestNamedSetterWithUnforgablePropertiesTable = { 2, 3, true, JSTestNamedSetterWithUnforgableProperties::info(), JSTestNamedSetterWithUnforgablePropertiesTableValues, JSTestNamedSetterWithUnforgablePropertiesTableIndex };
template<> JSValue JSTestNamedSetterWithUnforgablePropertiesConstructor::prototypeForStructure(JSC::VM& vm, const JSDOMGlobalObject& globalObject)
{
    UNUSED_PARAM(vm);
    return globalObject.functionPrototype();
}

template<> void JSTestNamedSetterWithUnforgablePropertiesConstructor::initializeProperties(VM& vm, JSDOMGlobalObject& globalObject)
{
    putDirect(vm, vm.propertyNames->prototype, JSTestNamedSetterWithUnforgableProperties::prototype(vm, globalObject), JSC::PropertyAttribute::DontDelete | JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::DontEnum);
    putDirect(vm, vm.propertyNames->name, jsNontrivialString(vm, String("TestNamedSetterWithUnforgableProperties"_s)), JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::DontEnum);
    putDirect(vm, vm.propertyNames->length, jsNumber(0), JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::DontEnum);
}

template<> const ClassInfo JSTestNamedSetterWithUnforgablePropertiesConstructor::s_info = { "TestNamedSetterWithUnforgableProperties", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(JSTestNamedSetterWithUnforgablePropertiesConstructor) };

/* Hash table for prototype */

static const HashTableValue JSTestNamedSetterWithUnforgablePropertiesPrototypeTableValues[] =
{
    { "constructor", static_cast<unsigned>(JSC::PropertyAttribute::DontEnum), NoIntrinsic, { (intptr_t)static_cast<PropertySlot::GetValueFunc>(jsTestNamedSetterWithUnforgablePropertiesConstructor), (intptr_t) static_cast<PutPropertySlot::PutValueFunc>(setJSTestNamedSetterWithUnforgablePropertiesConstructor) } },
};

const ClassInfo JSTestNamedSetterWithUnforgablePropertiesPrototype::s_info = { "TestNamedSetterWithUnforgablePropertiesPrototype", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(JSTestNamedSetterWithUnforgablePropertiesPrototype) };

void JSTestNamedSetterWithUnforgablePropertiesPrototype::finishCreation(VM& vm)
{
    Base::finishCreation(vm);
    reifyStaticProperties(vm, JSTestNamedSetterWithUnforgableProperties::info(), JSTestNamedSetterWithUnforgablePropertiesPrototypeTableValues, *this);
}

const ClassInfo JSTestNamedSetterWithUnforgableProperties::s_info = { "TestNamedSetterWithUnforgableProperties", &Base::s_info, &JSTestNamedSetterWithUnforgablePropertiesTable, nullptr, CREATE_METHOD_TABLE(JSTestNamedSetterWithUnforgableProperties) };

JSTestNamedSetterWithUnforgableProperties::JSTestNamedSetterWithUnforgableProperties(Structure* structure, JSDOMGlobalObject& globalObject, Ref<TestNamedSetterWithUnforgableProperties>&& impl)
    : JSDOMWrapper<TestNamedSetterWithUnforgableProperties>(structure, globalObject, WTFMove(impl))
{
}

void JSTestNamedSetterWithUnforgableProperties::finishCreation(VM& vm)
{
    Base::finishCreation(vm);
    ASSERT(inherits(vm, info()));

    static_assert(!std::is_base_of<ActiveDOMObject, TestNamedSetterWithUnforgableProperties>::value, "Interface is not marked as [ActiveDOMObject] even though implementation class subclasses ActiveDOMObject.");

}

JSObject* JSTestNamedSetterWithUnforgableProperties::createPrototype(VM& vm, JSDOMGlobalObject& globalObject)
{
    return JSTestNamedSetterWithUnforgablePropertiesPrototype::create(vm, &globalObject, JSTestNamedSetterWithUnforgablePropertiesPrototype::createStructure(vm, &globalObject, globalObject.objectPrototype()));
}

JSObject* JSTestNamedSetterWithUnforgableProperties::prototype(VM& vm, JSDOMGlobalObject& globalObject)
{
    return getDOMPrototype<JSTestNamedSetterWithUnforgableProperties>(vm, globalObject);
}

JSValue JSTestNamedSetterWithUnforgableProperties::getConstructor(VM& vm, const JSGlobalObject* globalObject)
{
    return getDOMConstructor<JSTestNamedSetterWithUnforgablePropertiesConstructor>(vm, *jsCast<const JSDOMGlobalObject*>(globalObject));
}

void JSTestNamedSetterWithUnforgableProperties::destroy(JSC::JSCell* cell)
{
    JSTestNamedSetterWithUnforgableProperties* thisObject = static_cast<JSTestNamedSetterWithUnforgableProperties*>(cell);
    thisObject->JSTestNamedSetterWithUnforgableProperties::~JSTestNamedSetterWithUnforgableProperties();
}

bool JSTestNamedSetterWithUnforgableProperties::getOwnPropertySlot(JSObject* object, JSGlobalObject* lexicalGlobalObject, PropertyName propertyName, PropertySlot& slot)
{
    auto* thisObject = jsCast<JSTestNamedSetterWithUnforgableProperties*>(object);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());
    using GetterIDLType = IDLDOMString;
    auto getterFunctor = [] (auto& thisObject, auto propertyName) -> Optional<typename GetterIDLType::ImplementationType> {
        auto result = thisObject.wrapped().namedItem(propertyNameToAtomString(propertyName));
        if (!GetterIDLType::isNullValue(result))
            return typename GetterIDLType::ImplementationType { GetterIDLType::extractValueFromNullable(result) };
        return WTF::nullopt;
    };
    if (auto namedProperty = accessVisibleNamedProperty<OverrideBuiltins::No>(*lexicalGlobalObject, *thisObject, propertyName, getterFunctor)) {
        auto value = toJS<IDLDOMString>(*lexicalGlobalObject, WTFMove(namedProperty.value()));
        slot.setValue(thisObject, static_cast<unsigned>(0), value);
        return true;
    }
    return JSObject::getOwnPropertySlot(object, lexicalGlobalObject, propertyName, slot);
}

bool JSTestNamedSetterWithUnforgableProperties::getOwnPropertySlotByIndex(JSObject* object, JSGlobalObject* lexicalGlobalObject, unsigned index, PropertySlot& slot)
{
    VM& vm = JSC::getVM(lexicalGlobalObject);
    auto* thisObject = jsCast<JSTestNamedSetterWithUnforgableProperties*>(object);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());
    auto propertyName = Identifier::from(vm, index);
    using GetterIDLType = IDLDOMString;
    auto getterFunctor = [] (auto& thisObject, auto propertyName) -> Optional<typename GetterIDLType::ImplementationType> {
        auto result = thisObject.wrapped().namedItem(propertyNameToAtomString(propertyName));
        if (!GetterIDLType::isNullValue(result))
            return typename GetterIDLType::ImplementationType { GetterIDLType::extractValueFromNullable(result) };
        return WTF::nullopt;
    };
    if (auto namedProperty = accessVisibleNamedProperty<OverrideBuiltins::No>(*lexicalGlobalObject, *thisObject, propertyName, getterFunctor)) {
        auto value = toJS<IDLDOMString>(*lexicalGlobalObject, WTFMove(namedProperty.value()));
        slot.setValue(thisObject, static_cast<unsigned>(0), value);
        return true;
    }
    return JSObject::getOwnPropertySlotByIndex(object, lexicalGlobalObject, index, slot);
}

void JSTestNamedSetterWithUnforgableProperties::getOwnPropertyNames(JSObject* object, JSGlobalObject* lexicalGlobalObject, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    VM& vm = JSC::getVM(lexicalGlobalObject);
    auto* thisObject = jsCast<JSTestNamedSetterWithUnforgableProperties*>(object);
    ASSERT_GC_OBJECT_INHERITS(object, info());
    for (auto& propertyName : thisObject->wrapped().supportedPropertyNames())
        propertyNames.add(Identifier::fromString(vm, propertyName));
    JSObject::getOwnPropertyNames(object, lexicalGlobalObject, propertyNames, mode);
}

bool JSTestNamedSetterWithUnforgableProperties::put(JSCell* cell, JSGlobalObject* lexicalGlobalObject, PropertyName propertyName, JSValue value, PutPropertySlot& putPropertySlot)
{
    auto* thisObject = jsCast<JSTestNamedSetterWithUnforgableProperties*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());

    if (!propertyName.isSymbol()) {
        PropertySlot slot { thisObject, PropertySlot::InternalMethodType::VMInquiry };
        JSValue prototype = thisObject->getPrototypeDirect(JSC::getVM(lexicalGlobalObject));
        if (!(prototype.isObject() && asObject(prototype)->getPropertySlot(lexicalGlobalObject, propertyName, slot))) {
            auto throwScope = DECLARE_THROW_SCOPE(JSC::getVM(lexicalGlobalObject));
            auto nativeValue = convert<IDLDOMString>(*lexicalGlobalObject, value);
            RETURN_IF_EXCEPTION(throwScope, true);
            thisObject->wrapped().setNamedItem(propertyNameToString(propertyName), WTFMove(nativeValue));
            return true;
        }
    }

    return JSObject::put(thisObject, lexicalGlobalObject, propertyName, value, putPropertySlot);
}

bool JSTestNamedSetterWithUnforgableProperties::putByIndex(JSCell* cell, JSGlobalObject* lexicalGlobalObject, unsigned index, JSValue value, bool shouldThrow)
{
    VM& vm = JSC::getVM(lexicalGlobalObject);
    auto* thisObject = jsCast<JSTestNamedSetterWithUnforgableProperties*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());

    auto propertyName = Identifier::from(vm, index);
    PropertySlot slot { thisObject, PropertySlot::InternalMethodType::VMInquiry };
    JSValue prototype = thisObject->getPrototypeDirect(vm);
    if (!(prototype.isObject() && asObject(prototype)->getPropertySlot(lexicalGlobalObject, propertyName, slot))) {
        auto throwScope = DECLARE_THROW_SCOPE(JSC::getVM(lexicalGlobalObject));
        auto nativeValue = convert<IDLDOMString>(*lexicalGlobalObject, value);
        RETURN_IF_EXCEPTION(throwScope, true);
        thisObject->wrapped().setNamedItem(propertyNameToString(propertyName), WTFMove(nativeValue));
        return true;
    }

    return JSObject::putByIndex(cell, lexicalGlobalObject, index, value, shouldThrow);
}

bool JSTestNamedSetterWithUnforgableProperties::defineOwnProperty(JSObject* object, JSGlobalObject* lexicalGlobalObject, PropertyName propertyName, const PropertyDescriptor& propertyDescriptor, bool shouldThrow)
{
    auto* thisObject = jsCast<JSTestNamedSetterWithUnforgableProperties*>(object);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());

    if (!propertyName.isSymbol()) {
static bool isUnforgeablePropertyName(PropertyName propertyName)
{
    return propertyName == "unforgeableAttribute" || propertyName == "unforgeableOperation";
}

        if (!isUnforgeablePropertyName(propertyName)) {
            PropertySlot slot { thisObject, PropertySlot::InternalMethodType::VMInquiry };
            if (!JSObject::getOwnPropertySlot(thisObject, lexicalGlobalObject, propertyName, slot)) {
                if (!propertyDescriptor.isDataDescriptor())
                    return false;
                auto throwScope = DECLARE_THROW_SCOPE(JSC::getVM(lexicalGlobalObject));
                auto nativeValue = convert<IDLDOMString>(*lexicalGlobalObject, propertyDescriptor.value());
                RETURN_IF_EXCEPTION(throwScope, true);
                thisObject->wrapped().setNamedItem(propertyNameToString(propertyName), WTFMove(nativeValue));
                return true;
            }
        }
    }

    PropertyDescriptor newPropertyDescriptor = propertyDescriptor;
    newPropertyDescriptor.setConfigurable(true);
    return JSObject::defineOwnProperty(object, lexicalGlobalObject, propertyName, newPropertyDescriptor, shouldThrow);
}

template<> inline JSTestNamedSetterWithUnforgableProperties* IDLAttribute<JSTestNamedSetterWithUnforgableProperties>::cast(JSGlobalObject& lexicalGlobalObject, EncodedJSValue thisValue)
{
    return jsDynamicCast<JSTestNamedSetterWithUnforgableProperties*>(JSC::getVM(&lexicalGlobalObject), JSValue::decode(thisValue));
}

template<> inline JSTestNamedSetterWithUnforgableProperties* IDLOperation<JSTestNamedSetterWithUnforgableProperties>::cast(JSGlobalObject& lexicalGlobalObject, CallFrame& callFrame)
{
    return jsDynamicCast<JSTestNamedSetterWithUnforgableProperties*>(JSC::getVM(&lexicalGlobalObject), callFrame.thisValue());
}

EncodedJSValue jsTestNamedSetterWithUnforgablePropertiesConstructor(JSGlobalObject* lexicalGlobalObject, EncodedJSValue thisValue, PropertyName)
{
    VM& vm = JSC::getVM(lexicalGlobalObject);
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    auto* prototype = jsDynamicCast<JSTestNamedSetterWithUnforgablePropertiesPrototype*>(vm, JSValue::decode(thisValue));
    if (UNLIKELY(!prototype))
        return throwVMTypeError(lexicalGlobalObject, throwScope);
    return JSValue::encode(JSTestNamedSetterWithUnforgableProperties::getConstructor(JSC::getVM(lexicalGlobalObject), prototype->globalObject()));
}

bool setJSTestNamedSetterWithUnforgablePropertiesConstructor(JSGlobalObject* lexicalGlobalObject, EncodedJSValue thisValue, EncodedJSValue encodedValue)
{
    VM& vm = JSC::getVM(lexicalGlobalObject);
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    auto* prototype = jsDynamicCast<JSTestNamedSetterWithUnforgablePropertiesPrototype*>(vm, JSValue::decode(thisValue));
    if (UNLIKELY(!prototype)) {
        throwVMTypeError(lexicalGlobalObject, throwScope);
        return false;
    }
    // Shadowing a built-in constructor
    return prototype->putDirect(vm, vm.propertyNames->constructor, JSValue::decode(encodedValue));
}

static inline JSValue jsTestNamedSetterWithUnforgablePropertiesUnforgeableAttributeGetter(JSGlobalObject& lexicalGlobalObject, JSTestNamedSetterWithUnforgableProperties& thisObject, ThrowScope& throwScope)
{
    UNUSED_PARAM(throwScope);
    UNUSED_PARAM(lexicalGlobalObject);
    auto& impl = thisObject.wrapped();
    JSValue result = toJS<IDLDOMString>(lexicalGlobalObject, throwScope, impl.unforgeableAttribute());
    return result;
}

EncodedJSValue jsTestNamedSetterWithUnforgablePropertiesUnforgeableAttribute(JSGlobalObject* lexicalGlobalObject, EncodedJSValue thisValue, PropertyName)
{
    return IDLAttribute<JSTestNamedSetterWithUnforgableProperties>::get<jsTestNamedSetterWithUnforgablePropertiesUnforgeableAttributeGetter, CastedThisErrorBehavior::Assert>(*lexicalGlobalObject, thisValue, "unforgeableAttribute");
}

static inline JSC::EncodedJSValue jsTestNamedSetterWithUnforgablePropertiesInstanceFunctionUnforgeableOperationBody(JSC::JSGlobalObject* lexicalGlobalObject, JSC::CallFrame* callFrame, typename IDLOperation<JSTestNamedSetterWithUnforgableProperties>::ClassParameter castedThis, JSC::ThrowScope& throwScope)
{
    UNUSED_PARAM(lexicalGlobalObject);
    UNUSED_PARAM(callFrame);
    UNUSED_PARAM(throwScope);
    auto& impl = castedThis->wrapped();
    impl.unforgeableOperation();
    return JSValue::encode(jsUndefined());
}

EncodedJSValue JSC_HOST_CALL jsTestNamedSetterWithUnforgablePropertiesInstanceFunctionUnforgeableOperation(JSGlobalObject* lexicalGlobalObject, CallFrame* callFrame)
{
    return IDLOperation<JSTestNamedSetterWithUnforgableProperties>::call<jsTestNamedSetterWithUnforgablePropertiesInstanceFunctionUnforgeableOperationBody>(*lexicalGlobalObject, *callFrame, "unforgeableOperation");
}

void JSTestNamedSetterWithUnforgableProperties::analyzeHeap(JSCell* cell, HeapAnalyzer& analyzer)
{
    auto* thisObject = jsCast<JSTestNamedSetterWithUnforgableProperties*>(cell);
    analyzer.setWrappedObjectForCell(cell, &thisObject->wrapped());
    if (thisObject->scriptExecutionContext())
        analyzer.setLabelForCell(cell, "url " + thisObject->scriptExecutionContext()->url().string());
    Base::analyzeHeap(cell, analyzer);
}

bool JSTestNamedSetterWithUnforgablePropertiesOwner::isReachableFromOpaqueRoots(JSC::Handle<JSC::Unknown> handle, void*, SlotVisitor& visitor, const char** reason)
{
    UNUSED_PARAM(handle);
    UNUSED_PARAM(visitor);
    UNUSED_PARAM(reason);
    return false;
}

void JSTestNamedSetterWithUnforgablePropertiesOwner::finalize(JSC::Handle<JSC::Unknown> handle, void* context)
{
    auto* jsTestNamedSetterWithUnforgableProperties = static_cast<JSTestNamedSetterWithUnforgableProperties*>(handle.slot()->asCell());
    auto& world = *static_cast<DOMWrapperWorld*>(context);
    uncacheWrapper(world, &jsTestNamedSetterWithUnforgableProperties->wrapped(), jsTestNamedSetterWithUnforgableProperties);
}

#if ENABLE(BINDING_INTEGRITY)
#if PLATFORM(WIN)
#pragma warning(disable: 4483)
extern "C" { extern void (*const __identifier("??_7TestNamedSetterWithUnforgableProperties@WebCore@@6B@")[])(); }
#else
extern "C" { extern void* _ZTVN7WebCore39TestNamedSetterWithUnforgablePropertiesE[]; }
#endif
#endif

JSC::JSValue toJSNewlyCreated(JSC::JSGlobalObject*, JSDOMGlobalObject* globalObject, Ref<TestNamedSetterWithUnforgableProperties>&& impl)
{

#if ENABLE(BINDING_INTEGRITY)
    void* actualVTablePointer = *(reinterpret_cast<void**>(impl.ptr()));
#if PLATFORM(WIN)
    void* expectedVTablePointer = WTF_PREPARE_VTBL_POINTER_FOR_INSPECTION(__identifier("??_7TestNamedSetterWithUnforgableProperties@WebCore@@6B@"));
#else
    void* expectedVTablePointer = WTF_PREPARE_VTBL_POINTER_FOR_INSPECTION(&_ZTVN7WebCore39TestNamedSetterWithUnforgablePropertiesE[2]);
#endif

    // If this fails TestNamedSetterWithUnforgableProperties does not have a vtable, so you need to add the
    // ImplementationLacksVTable attribute to the interface definition
    static_assert(std::is_polymorphic<TestNamedSetterWithUnforgableProperties>::value, "TestNamedSetterWithUnforgableProperties is not polymorphic");

    // If you hit this assertion you either have a use after free bug, or
    // TestNamedSetterWithUnforgableProperties has subclasses. If TestNamedSetterWithUnforgableProperties has subclasses that get passed
    // to toJS() we currently require TestNamedSetterWithUnforgableProperties you to opt out of binding hardening
    // by adding the SkipVTableValidation attribute to the interface IDL definition
    RELEASE_ASSERT(actualVTablePointer == expectedVTablePointer);
#endif
    return createWrapper<TestNamedSetterWithUnforgableProperties>(globalObject, WTFMove(impl));
}

JSC::JSValue toJS(JSC::JSGlobalObject* lexicalGlobalObject, JSDOMGlobalObject* globalObject, TestNamedSetterWithUnforgableProperties& impl)
{
    return wrap(lexicalGlobalObject, globalObject, impl);
}

TestNamedSetterWithUnforgableProperties* JSTestNamedSetterWithUnforgableProperties::toWrapped(JSC::VM& vm, JSC::JSValue value)
{
    if (auto* wrapper = jsDynamicCast<JSTestNamedSetterWithUnforgableProperties*>(vm, value))
        return &wrapper->wrapped();
    return nullptr;
}

}