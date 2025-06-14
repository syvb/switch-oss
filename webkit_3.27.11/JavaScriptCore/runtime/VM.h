/*
 * Copyright (C) 2008-2022 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "CalleeBits.h"
#include "CodeSpecializationKind.h"
#include "ConcurrentJSLock.h"
#include "DFGDoesGCCheck.h"
#include "DeleteAllCodeEffort.h"
#include "ExceptionEventLocation.h"
#include "FunctionHasExecutedCache.h"
#include "Heap.h"
#include "Integrity.h"
#include "Intrinsic.h"
#include "JSCJSValue.h"
#include "JSDateMath.h"
#include "JSLock.h"
#include "JSONAtomStringCache.h"
#include "MacroAssemblerCodeRef.h"
#include "Microtask.h"
#include "NativeFunction.h"
#include "NumericStrings.h"
#include "SmallStrings.h"
#include "Strong.h"
#include "StructureCache.h"
#include "SubspaceAccess.h"
#include "ThunkGenerator.h"
#include "VMTraps.h"
#include "WasmContext.h"
#include <wtf/BumpPointerAllocator.h>
#include <wtf/CheckedArithmetic.h>
#include <wtf/Deque.h>
#include <wtf/DoublyLinkedList.h>
#include <wtf/Forward.h>
#include <wtf/Gigacage.h>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/SetForScope.h>
#include <wtf/StackPointer.h>
#include <wtf/Stopwatch.h>
#include <wtf/ThreadSafeRefCounted.h>
#include <wtf/UniqueArray.h>
#include <wtf/text/SymbolRegistry.h>
#include <wtf/text/WTFString.h>
#if ENABLE(REGEXP_TRACING)
#include <wtf/ListHashSet.h>
#endif

// Enable the Objective-C API for platforms with a modern runtime. This has to match exactly what we
// have in JSBase.h.
#if !defined(JSC_OBJC_API_ENABLED)
#if (defined(__clang__) && defined(__APPLE__) && (defined(__MAC_OS_X_VERSION_MIN_REQUIRED) || (defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)))
#define JSC_OBJC_API_ENABLED 1
#else
#define JSC_OBJC_API_ENABLED 0
#endif
#endif

namespace WTF {
class RunLoop;
class SimpleStats;
class StackTrace;
} // namespace WTF
using WTF::SimpleStats;
using WTF::StackTrace;

namespace JSC {

class ArgList;
class BuiltinExecutables;
class BytecodeIntrinsicRegistry;
class CallFrame;
enum class CallMode;
struct CheckpointOSRExitSideState;
class CodeBlock;
class CodeCache;
class CommonIdentifiers;
class CompactTDZEnvironmentMap;
class ConservativeRoots;
class ControlFlowProfiler;
class Exception;
class ExceptionScope;
class FuzzerAgent;
class HasOwnPropertyCache;
class HeapAnalyzer;
class HeapProfiler;
class Interpreter;
class IntlCache;
class JSDestructibleObjectHeapCellType;
class JSGlobalObject;
class JSObject;
class JSPromise;
class JSPropertyNameEnumerator;
class JITSizeStatistics;
class JITThunks;
class NativeExecutable;
class DeferredWorkTimer;
class RegExp;
class RegExpCache;
class Register;
#if ENABLE(SAMPLING_PROFILER)
class SamplingProfiler;
#endif
class ShadowChicken;
class SharedJITStubSet;
class SourceProvider;
class SourceProviderCache;
class Structure;
class Symbol;
class TypedArrayController;
class VMEntryScope;
class TypeProfiler;
class TypeProfilerLog;
class Watchdog;
class WatchpointSet;

#if ENABLE(DFG_JIT) && ASSERT_ENABLED
#define ENABLE_DFG_DOES_GC_VALIDATION 1
#else
#define ENABLE_DFG_DOES_GC_VALIDATION 0
#endif

constexpr bool validateDFGDoesGC = ENABLE_DFG_DOES_GC_VALIDATION;

#if ENABLE(FTL_JIT)
namespace FTL {
class Thunks;
}
#endif // ENABLE(FTL_JIT)
namespace Profiler {
class Database;
}
namespace DOMJIT {
class Signature;
}

struct EntryFrame;
struct Instruction;

class QueuedTask {
    WTF_MAKE_NONCOPYABLE(QueuedTask);
    WTF_MAKE_FAST_ALLOCATED;
public:
    void run();

    QueuedTask(VM& vm, JSGlobalObject* globalObject, Ref<Microtask>&& microtask)
        : m_globalObject(vm, globalObject)
        , m_microtask(WTFMove(microtask))
    {
    }

private:
    Strong<JSGlobalObject> m_globalObject;
    Ref<Microtask> m_microtask;
};

DECLARE_ALLOCATOR_WITH_HEAP_IDENTIFIER(VM);

#if COMPILER(MSVC)
#pragma warning(push)
#pragma warning(disable: 4200) // Disable "zero-sized array in struct/union" warning
#endif
struct ScratchBuffer {
    ScratchBuffer()
    {
        u.m_activeLength = 0;
    }

    static ScratchBuffer* create(size_t size)
    {
        ScratchBuffer* result = new (VMMalloc::malloc(ScratchBuffer::allocationSize(size))) ScratchBuffer;
        return result;
    }

    static ScratchBuffer* fromData(void* buffer)
    {
        return bitwise_cast<ScratchBuffer*>(static_cast<char*>(buffer) - OBJECT_OFFSETOF(ScratchBuffer, m_buffer));
    }

    static size_t allocationSize(Checked<size_t> bufferSize) { return sizeof(ScratchBuffer) + bufferSize; }
    void setActiveLength(size_t activeLength) { u.m_activeLength = activeLength; }
    size_t activeLength() const { return u.m_activeLength; };
    size_t* addressOfActiveLength() { return &u.m_activeLength; };
    void* dataBuffer() { return m_buffer; }

    union {
        size_t m_activeLength;
        double pad; // Make sure m_buffer is double aligned.
    } u;
#if CPU(MIPS) && (defined WTF_MIPS_ARCH_REV && WTF_MIPS_ARCH_REV == 2)
    alignas(8) void* m_buffer[0];
#else
    void* m_buffer[0];
#endif
};
#if COMPILER(MSVC)
#pragma warning(pop)
#endif

class ActiveScratchBufferScope {
public:
    ActiveScratchBufferScope(ScratchBuffer*, size_t activeScratchBufferSizeInJSValues);
    ~ActiveScratchBufferScope();

private:
    ScratchBuffer* m_scratchBuffer;
};

class VM : public ThreadSafeRefCounted<VM>, public DoublyLinkedListNode<VM> {
public:
    // WebCore has a one-to-one mapping of threads to VMs;
    // create() should only be called once
    // on a thread, this is the 'default' VM (it uses the
    // thread's default string uniquing table from Thread::current()).
    // API contexts created using the new context group aware interface
    // create APIContextGroup objects which require less locking of JSC
    // than the old singleton APIShared VM created for use by
    // the original API.
    enum VMType { Default, APIContextGroup, APIShared };

    struct ClientData {
        JS_EXPORT_PRIVATE virtual ~ClientData() = 0;
    };

    bool isSharedInstance() { return vmType == APIShared; }
    bool usingAPI() { return vmType != Default; }
    JS_EXPORT_PRIVATE static bool sharedInstanceExists();
    JS_EXPORT_PRIVATE static VM& sharedInstance();

    JS_EXPORT_PRIVATE static Ref<VM> create(HeapType = SmallHeap, WTF::RunLoop* = nullptr);
    JS_EXPORT_PRIVATE static RefPtr<VM> tryCreate(HeapType = SmallHeap, WTF::RunLoop* = nullptr);
    static Ref<VM> createContextGroup(HeapType = SmallHeap);
    JS_EXPORT_PRIVATE ~VM();

    Watchdog& ensureWatchdog();
    Watchdog* watchdog() { return m_watchdog.get(); }

    HeapProfiler* heapProfiler() const { return m_heapProfiler.get(); }
    JS_EXPORT_PRIVATE HeapProfiler& ensureHeapProfiler();

    bool isAnalyzingHeap() const { return m_activeHeapAnalyzer; }
    HeapAnalyzer* activeHeapAnalyzer() const { return m_activeHeapAnalyzer; }
    void setActiveHeapAnalyzer(HeapAnalyzer* analyzer) { m_activeHeapAnalyzer = analyzer; }

#if ENABLE(SAMPLING_PROFILER)
    SamplingProfiler* samplingProfiler() { return m_samplingProfiler.get(); }
    JS_EXPORT_PRIVATE SamplingProfiler& ensureSamplingProfiler(Ref<Stopwatch>&&);
#endif

    FuzzerAgent* fuzzerAgent() const { return m_fuzzerAgent.get(); }
    void setFuzzerAgent(std::unique_ptr<FuzzerAgent>&&);

    static unsigned numberOfIDs() { return s_numberOfIDs.load(); }
    unsigned id() const { return m_id; }
    bool isEntered() const { return !!entryScope; }

    inline CallFrame* topJSCallFrame() const;

    // Global object in which execution began.
    JS_EXPORT_PRIVATE JSGlobalObject* deprecatedVMEntryGlobalObject(JSGlobalObject*) const;

    WeakRandom& random() { return m_random; }
    WeakRandom& heapRandom() { return m_heapRandom; }
    Integrity::Random& integrityRandom() { return m_integrityRandom; }

    bool terminationInProgress() const { return m_terminationInProgress; }
    void setTerminationInProgress(bool value) { m_terminationInProgress = value; }

    bool executionForbidden() const { return m_executionForbidden; }
    void setExecutionForbidden() { m_executionForbidden = true; }

    // Setting this means that the VM can never recover from a TerminationException.
    // Currently, we'll only set this for worker threads. Ideally, we want this
    // to always be true. However, we're only limiting it to workers for now until
    // we can be sure that clients using the JSC watchdog (which uses termination)
    // isn't broken by this change.
    void forbidExecutionOnTermination() { m_executionForbiddenOnTermination = true; }

    JS_EXPORT_PRIVATE Exception* ensureTerminationException();
    Exception* terminationException() const
    {
        ASSERT(m_terminationException);
        return m_terminationException;
    }
    bool isTerminationException(Exception* exception) const
    {
        ASSERT(exception);
        return exception == m_terminationException;
    }
    bool hasPendingTerminationException() const
    {
        return m_exception && isTerminationException(m_exception);
    }

    void throwTerminationException();

private:
    unsigned nextID();

#if !PLATFORM(WKC)
    static Atomic<unsigned> s_numberOfIDs;
#else
    WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY_ZERO(Atomic<unsigned>, s_numberOfIDs);
#endif

    unsigned m_id;
    RefPtr<JSLock> m_apiLock;
    Ref<WTF::RunLoop> m_runLoop;

    WeakRandom m_random;
    WeakRandom m_heapRandom;
    Integrity::Random m_integrityRandom;

public:
    Heap heap;

    const HeapCellType& cellHeapCellType() { return heap.cellHeapCellType; }
    const JSDestructibleObjectHeapCellType& destructibleObjectHeapCellType() { return heap.destructibleObjectHeapCellType; };

#if ENABLE(JIT)
    std::unique_ptr<JITSizeStatistics> jitSizeStatistics;
#endif
    
    ALWAYS_INLINE CompleteSubspace& primitiveGigacageAuxiliarySpace() { return heap.primitiveGigacageAuxiliarySpace; }
    ALWAYS_INLINE CompleteSubspace& jsValueGigacageAuxiliarySpace() { return heap.jsValueGigacageAuxiliarySpace; }
    ALWAYS_INLINE CompleteSubspace& immutableButterflyJSValueGigacageAuxiliarySpace() { return heap.immutableButterflyJSValueGigacageAuxiliarySpace; }
    ALWAYS_INLINE CompleteSubspace& gigacageAuxiliarySpace(Gigacage::Kind kind) { return heap.gigacageAuxiliarySpace(kind); }
    ALWAYS_INLINE CompleteSubspace& cellSpace() { return heap.cellSpace; }
    ALWAYS_INLINE CompleteSubspace& variableSizedCellSpace() { return heap.variableSizedCellSpace; }
    ALWAYS_INLINE CompleteSubspace& destructibleObjectSpace() { return heap.destructibleObjectSpace; }

    ALWAYS_INLINE IsoSubspace& arraySpace() { return heap.arraySpace; }
    ALWAYS_INLINE IsoSubspace& bigIntSpace() { return heap.bigIntSpace; }
    ALWAYS_INLINE IsoSubspace& calleeSpace() { return heap.calleeSpace; }
    ALWAYS_INLINE IsoSubspace& clonedArgumentsSpace() { return heap.clonedArgumentsSpace; }
    ALWAYS_INLINE IsoSubspace& customGetterSetterSpace() { return heap.customGetterSetterSpace; }
    ALWAYS_INLINE IsoSubspace& dateInstanceSpace() { return heap.dateInstanceSpace; }
    ALWAYS_INLINE IsoSubspace& domAttributeGetterSetterSpace() { return heap.domAttributeGetterSetterSpace; }
    ALWAYS_INLINE IsoSubspace& exceptionSpace() { return heap.exceptionSpace; }
    ALWAYS_INLINE IsoSubspace& executableToCodeBlockEdgeSpace() { return heap.executableToCodeBlockEdgeSpace; }
    ALWAYS_INLINE IsoSubspace& functionSpace() { return heap.functionSpace; }
    ALWAYS_INLINE IsoSubspace& getterSetterSpace() { return heap.getterSetterSpace; }
    ALWAYS_INLINE IsoSubspace& globalLexicalEnvironmentSpace() { return heap.globalLexicalEnvironmentSpace; }
    ALWAYS_INLINE IsoSubspace& internalFunctionSpace() { return heap.internalFunctionSpace; }
    ALWAYS_INLINE IsoSubspace& jsProxySpace() { return heap.jsProxySpace; }
    ALWAYS_INLINE IsoSubspace& nativeExecutableSpace() { return heap.nativeExecutableSpace; }
    ALWAYS_INLINE IsoSubspace& numberObjectSpace() { return heap.numberObjectSpace; }
    ALWAYS_INLINE IsoSubspace& plainObjectSpace() { return heap.plainObjectSpace; }
    ALWAYS_INLINE IsoSubspace& promiseSpace() { return heap.promiseSpace; }
    ALWAYS_INLINE IsoSubspace& propertyNameEnumeratorSpace() { return heap.propertyNameEnumeratorSpace; }
    ALWAYS_INLINE IsoSubspace& propertyTableSpace() { return heap.propertyTableSpace; }
    ALWAYS_INLINE IsoSubspace& regExpSpace() { return heap.regExpSpace; }
    ALWAYS_INLINE IsoSubspace& regExpObjectSpace() { return heap.regExpObjectSpace; }
    ALWAYS_INLINE IsoSubspace& ropeStringSpace() { return heap.ropeStringSpace; }
    ALWAYS_INLINE IsoSubspace& scopedArgumentsSpace() { return heap.scopedArgumentsSpace; }
    ALWAYS_INLINE IsoSubspace& sparseArrayValueMapSpace() { return heap.sparseArrayValueMapSpace; }
    ALWAYS_INLINE IsoSubspace& stringSpace() { return heap.stringSpace; }
    ALWAYS_INLINE IsoSubspace& stringObjectSpace() { return heap.stringObjectSpace; }
    ALWAYS_INLINE IsoSubspace& structureChainSpace() { return heap.structureChainSpace; }
    ALWAYS_INLINE IsoSubspace& structureRareDataSpace() { return heap.structureRareDataSpace; }
    ALWAYS_INLINE IsoSubspace& structureSpace() { return heap.structureSpace; }
    ALWAYS_INLINE IsoSubspace& brandedStructureSpace() { return heap.brandedStructureSpace; }
    ALWAYS_INLINE IsoSubspace& symbolTableSpace() { return heap.symbolTableSpace; }

#define DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(name) \
    template<SubspaceAccess mode> \
    ALWAYS_INLINE IsoSubspace* name() { return heap.name<mode>(); }

#if JSC_OBJC_API_ENABLED
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(apiWrapperObjectSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(objCCallbackFunctionSpace)
#endif
#ifdef JSC_GLIB_API_ENABLED
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(apiWrapperObjectSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(jscCallbackFunctionSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(callbackAPIWrapperGlobalObjectSpace)
#endif
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(apiGlobalObjectSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(apiValueWrapperSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(arrayBufferSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(arrayIteratorSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(asyncGeneratorSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(bigInt64ArraySpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(bigIntObjectSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(bigUint64ArraySpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(booleanObjectSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(boundFunctionSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(callbackConstructorSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(callbackGlobalObjectSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(callbackFunctionSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(callbackObjectSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(customGetterFunctionSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(customSetterFunctionSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(dataViewSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(debuggerScopeSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(errorInstanceSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(float32ArraySpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(float64ArraySpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(functionRareDataSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(generatorSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(globalObjectSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(injectedScriptHostSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(int8ArraySpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(int16ArraySpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(int32ArraySpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(javaScriptCallFrameSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(jsModuleRecordSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(mapBucketSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(mapIteratorSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(mapSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(moduleNamespaceObjectSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(nativeStdFunctionSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(proxyObjectSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(proxyRevokeSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(scopedArgumentsTableSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(scriptFetchParametersSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(scriptFetcherSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(setBucketSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(setIteratorSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(setSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(shadowRealmSpace);
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(strictEvalActivationSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(stringIteratorSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(sourceCodeSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(symbolSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(symbolObjectSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(templateObjectDescriptorSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(temporalCalendarSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(temporalDurationSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(temporalInstantSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(temporalPlainTimeSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(temporalTimeZoneSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(uint8ArraySpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(uint8ClampedArraySpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(uint16ArraySpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(uint32ArraySpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(unlinkedEvalCodeBlockSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(unlinkedFunctionCodeBlockSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(unlinkedModuleProgramCodeBlockSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(unlinkedProgramCodeBlockSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(finalizationRegistrySpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(weakObjectRefSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(weakSetSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(weakMapSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(withScopeSpace)
#if ENABLE(WKC_INTL)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(intlCollatorSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(intlDateTimeFormatSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(intlDisplayNamesSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(intlListFormatSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(intlLocaleSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(intlNumberFormatSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(intlPluralRulesSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(intlRelativeTimeFormatSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(intlSegmentIteratorSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(intlSegmenterSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(intlSegmentsSpace)
#endif
#if ENABLE(WEBASSEMBLY)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(jsToWasmICCalleeSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(webAssemblyExceptionSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(webAssemblyFunctionSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(webAssemblyGlobalSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(webAssemblyInstanceSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(webAssemblyMemorySpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(webAssemblyModuleSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(webAssemblyModuleRecordSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(webAssemblyTableSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(webAssemblyTagSpace)
    DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER(webAssemblyWrapperFunctionSpace)
#endif

#undef DYNAMIC_ISO_SUBSPACE_DEFINE_MEMBER

    ALWAYS_INLINE IsoCellSet& executableToCodeBlockEdgesWithConstraints() { return heap.executableToCodeBlockEdgesWithConstraints; }
    ALWAYS_INLINE IsoCellSet& executableToCodeBlockEdgesWithFinalizers() { return heap.executableToCodeBlockEdgesWithFinalizers; }

#define DYNAMIC_SPACE_AND_SET_DEFINE_MEMBER(name) \
    template<SubspaceAccess mode> \
    ALWAYS_INLINE IsoSubspace* name() { return heap.name<mode>(); }

    using SpaceAndSet = Heap::SpaceAndSet;
    ALWAYS_INLINE SpaceAndSet& codeBlockSpace() { return heap.codeBlockSpace; }

    DYNAMIC_SPACE_AND_SET_DEFINE_MEMBER(evalExecutableSpace)
    DYNAMIC_SPACE_AND_SET_DEFINE_MEMBER(moduleProgramExecutableSpace)

    ALWAYS_INLINE SpaceAndSet& functionExecutableSpace() { return heap.functionExecutableSpace; }
    ALWAYS_INLINE SpaceAndSet& programExecutableSpace() { return heap.programExecutableSpace; }
    ALWAYS_INLINE SpaceAndSet& unlinkedFunctionExecutableSpace() { return heap.unlinkedFunctionExecutableSpace; }

    VMType vmType;
    ClientData* clientData;
    EntryFrame* topEntryFrame { nullptr };
    // NOTE: When throwing an exception while rolling back the call frame, this may be equal to
    // topEntryFrame.
    // FIXME: This should be a void*, because it might not point to a CallFrame.
    // https://bugs.webkit.org/show_bug.cgi?id=160441
    CallFrame* topCallFrame { nullptr };
#if ENABLE(WEBASSEMBLY)
    Wasm::Context wasmContext;
#endif
    Strong<Structure> structureStructure;
    Strong<Structure> structureRareDataStructure;
    Strong<Structure> stringStructure;
    Strong<Structure> propertyNameEnumeratorStructure;
    Strong<Structure> getterSetterStructure;
    Strong<Structure> customGetterSetterStructure;
    Strong<Structure> domAttributeGetterSetterStructure;
    Strong<Structure> scopedArgumentsTableStructure;
    Strong<Structure> apiWrapperStructure;
    Strong<Structure> nativeExecutableStructure;
    Strong<Structure> evalExecutableStructure;
    Strong<Structure> programExecutableStructure;
    Strong<Structure> functionExecutableStructure;
#if ENABLE(WEBASSEMBLY)
    Strong<Structure> webAssemblyCalleeGroupStructure;
#endif
    Strong<Structure> moduleProgramExecutableStructure;
    Strong<Structure> regExpStructure;
    Strong<Structure> symbolStructure;
    Strong<Structure> symbolTableStructure;
    Strong<Structure> immutableButterflyStructures[NumberOfCopyOnWriteIndexingModes];
    Strong<Structure> sourceCodeStructure;
    Strong<Structure> scriptFetcherStructure;
    Strong<Structure> scriptFetchParametersStructure;
    Strong<Structure> structureChainStructure;
    Strong<Structure> sparseArrayValueMapStructure;
    Strong<Structure> templateObjectDescriptorStructure;
    Strong<Structure> unlinkedFunctionExecutableStructure;
    Strong<Structure> unlinkedProgramCodeBlockStructure;
    Strong<Structure> unlinkedEvalCodeBlockStructure;
    Strong<Structure> unlinkedFunctionCodeBlockStructure;
    Strong<Structure> unlinkedModuleProgramCodeBlockStructure;
    Strong<Structure> propertyTableStructure;
    Strong<Structure> functionRareDataStructure;
    Strong<Structure> exceptionStructure;
    Strong<Structure> programCodeBlockStructure;
    Strong<Structure> moduleProgramCodeBlockStructure;
    Strong<Structure> evalCodeBlockStructure;
    Strong<Structure> functionCodeBlockStructure;
    Strong<Structure> hashMapBucketSetStructure;
    Strong<Structure> hashMapBucketMapStructure;
    Strong<Structure> bigIntStructure;
    Strong<Structure> executableToCodeBlockEdgeStructure;

    Strong<JSPropertyNameEnumerator> m_emptyPropertyNameEnumerator;

    Strong<JSCell> m_sentinelSetBucket;
    Strong<JSCell> m_sentinelMapBucket;

    Weak<NativeExecutable> m_fastBoundExecutable;
    Weak<NativeExecutable> m_fastCanConstructBoundExecutable;
    Weak<NativeExecutable> m_slowBoundExecutable;
    Weak<NativeExecutable> m_slowCanConstructBoundExecutable;

    Ref<DeferredWorkTimer> deferredWorkTimer;

    JSCell* currentlyDestructingCallbackObject;
    const ClassInfo* currentlyDestructingCallbackObjectClassInfo { nullptr };

    AtomStringTable* m_atomStringTable;
    WTF::SymbolRegistry m_symbolRegistry;
    WTF::SymbolRegistry m_privateSymbolRegistry;
    CommonIdentifiers* propertyNames;
    const ArgList* emptyList;
    SmallStrings smallStrings;
    NumericStrings numericStrings;
    std::unique_ptr<SimpleStats> machineCodeBytesPerBytecodeWordForBaselineJIT;
    Strong<JSString> lastCachedString;
    Ref<StringImpl> lastAtomizedIdentifierStringImpl { *StringImpl::empty() };
    Ref<AtomStringImpl> lastAtomizedIdentifierAtomStringImpl { *static_cast<AtomStringImpl*>(StringImpl::empty()) };
    JSONAtomStringCache jsonAtomStringCache;

    AtomStringTable* atomStringTable() const { return m_atomStringTable; }
    WTF::SymbolRegistry& symbolRegistry() { return m_symbolRegistry; }
    WTF::SymbolRegistry& privateSymbolRegistry() { return m_privateSymbolRegistry; }

    Strong<JSBigInt> heapBigIntConstantOne;

    JSCell* sentinelSetBucket()
    {
        if (LIKELY(m_sentinelSetBucket))
            return m_sentinelSetBucket.get();
        return sentinelSetBucketSlow();
    }

    JSCell* sentinelMapBucket()
    {
        if (LIKELY(m_sentinelMapBucket))
            return m_sentinelMapBucket.get();
        return sentinelMapBucketSlow();
    }

    JSPropertyNameEnumerator* emptyPropertyNameEnumerator()
    {
        if (LIKELY(m_emptyPropertyNameEnumerator))
            return m_emptyPropertyNameEnumerator.get();
        return emptyPropertyNameEnumeratorSlow();
    }

    WeakGCMap<SymbolImpl*, Symbol, PtrHash<SymbolImpl*>> symbolImplToSymbolMap;

    enum class DeletePropertyMode {
        // Default behaviour of deleteProperty, matching the spec.
        Default,
        // This setting causes deleteProperty to force deletion of all
        // properties including those that are non-configurable (DontDelete).
        IgnoreConfigurable
    };

    DeletePropertyMode deletePropertyMode()
    {
        return m_deletePropertyMode;
    }

    class DeletePropertyModeScope {
    public:
        DeletePropertyModeScope(VM& vm, DeletePropertyMode mode)
            : m_vm(vm)
            , m_previousMode(vm.m_deletePropertyMode)
        {
            m_vm.m_deletePropertyMode = mode;
        }

        ~DeletePropertyModeScope()
        {
            m_vm.m_deletePropertyMode = m_previousMode;
        }

    private:
        VM& m_vm;
        DeletePropertyMode m_previousMode;
    };

    static JS_EXPORT_PRIVATE bool canUseAssembler();
    static bool isInMiniMode()
    {
#if !PLATFORM(WKC)
        return !Options::useJIT() || Options::forceMiniVMMode();
#else
        return false;
#endif
    }

    static bool useUnlinkedCodeBlockJettisoning()
    {
        return Options::useUnlinkedCodeBlockJettisoning() || isInMiniMode();
    }

    static void computeCanUseJIT();

    SourceProviderCache* addSourceProviderCache(SourceProvider*);
    void clearSourceProviderCaches();

    StructureCache structureCache;
    InlineWatchpointSet m_structureCacheClearedWatchpoint;

    typedef HashMap<RefPtr<SourceProvider>, RefPtr<SourceProviderCache>> SourceProviderCacheMap;
    SourceProviderCacheMap sourceProviderCacheMap;
    Interpreter* interpreter;
#if ENABLE(JIT)
    std::unique_ptr<JITThunks> jitStubs;
    MacroAssemblerCodeRef<JITThunkPtrTag> getCTIStub(ThunkGenerator);
    std::unique_ptr<SharedJITStubSet> m_sharedJITStubs;

#endif // ENABLE(JIT)
#if ENABLE(FTL_JIT)
    std::unique_ptr<FTL::Thunks> ftlThunks;
#endif

    NativeExecutable* getHostFunction(NativeFunction, NativeFunction constructor, const String& name);
    NativeExecutable* getHostFunction(NativeFunction, Intrinsic, NativeFunction constructor, const DOMJIT::Signature*, const String& name);

    NativeExecutable* getBoundFunction(bool isJSFunction, bool canConstruct);

    MacroAssemblerCodePtr<JSEntryPtrTag> getCTIInternalFunctionTrampolineFor(CodeSpecializationKind);
    MacroAssemblerCodeRef<JSEntryPtrTag> getCTILinkCall();
    MacroAssemblerCodeRef<JSEntryPtrTag> getCTIThrowExceptionFromCallSlowPath();
    MacroAssemblerCodeRef<JITStubRoutinePtrTag> getCTIVirtualCall(CallMode);

    static ptrdiff_t exceptionOffset()
    {
        return OBJECT_OFFSETOF(VM, m_exception);
    }

    static ptrdiff_t callFrameForCatchOffset()
    {
        return OBJECT_OFFSETOF(VM, callFrameForCatch);
    }

    static ptrdiff_t calleeForWasmCatchOffset()
    {
        return OBJECT_OFFSETOF(VM, calleeForWasmCatch);
    }

    static ptrdiff_t topEntryFrameOffset()
    {
        return OBJECT_OFFSETOF(VM, topEntryFrame);
    }

    static ptrdiff_t offsetOfEncodedHostCallReturnValue()
    {
        return OBJECT_OFFSETOF(VM, encodedHostCallReturnValue);
    }

    static ptrdiff_t offsetOfHeapBarrierThreshold()
    {
        return OBJECT_OFFSETOF(VM, heap) + OBJECT_OFFSETOF(Heap, m_barrierThreshold);
    }

    static ptrdiff_t offsetOfHeapMutatorShouldBeFenced()
    {
        return OBJECT_OFFSETOF(VM, heap) + OBJECT_OFFSETOF(Heap, m_mutatorShouldBeFenced);
    }

    void restorePreviousException(Exception* exception) { setException(exception); }

    void clearLastException() { m_lastException = nullptr; }

    CallFrame** addressOfCallFrameForCatch() { return &callFrameForCatch; }

    JSCell** addressOfException() { return reinterpret_cast<JSCell**>(&m_exception); }

    Exception* lastException() const { return m_lastException; }
    JSCell** addressOfLastException() { return reinterpret_cast<JSCell**>(&m_lastException); }

    // This should only be used for code that wants to check for any pending
    // exception without interfering with Throw/CatchScopes.
    Exception* exceptionForInspection() const { return m_exception; }

    void setFailNextNewCodeBlock() { m_failNextNewCodeBlock = true; }
    bool getAndClearFailNextNewCodeBlock()
    {
        bool result = m_failNextNewCodeBlock;
        m_failNextNewCodeBlock = false;
        return result;
    }
    
    ALWAYS_INLINE Structure* getStructure(StructureID id)
    {
        return heap.structureIDTable().get(decontaminate(id));
    }

    // FIXME: rdar://69036888: remove this function when no longer needed.
    ALWAYS_INLINE Structure* tryGetStructure(StructureID id)
    {
        return heap.structureIDTable().tryGet(decontaminate(id));
    }

    void* stackPointerAtVMEntry() const { return m_stackPointerAtVMEntry; }
    void setStackPointerAtVMEntry(void*);

    size_t softReservedZoneSize() const { return m_currentSoftReservedZoneSize; }
    size_t updateSoftReservedZoneSize(size_t softReservedZoneSize);
    
    static size_t committedStackByteCount();
    inline bool ensureStackCapacityFor(Register* newTopOfStack);

    void* stackLimit() { return m_stackLimit; }
    void* softStackLimit() { return m_softStackLimit; }
    void** addressOfSoftStackLimit() { return &m_softStackLimit; }
#if ENABLE(C_LOOP)
    void* cloopStackLimit() { return m_cloopStackLimit; }
    void setCLoopStackLimit(void* limit) { m_cloopStackLimit = limit; }
    JS_EXPORT_PRIVATE void* currentCLoopStackPointer() const;
#endif

    inline bool isSafeToRecurseSoft() const;
    bool isSafeToRecurse() const
    {
        return isSafeToRecurse(m_stackLimit);
    }

    void** addressOfLastStackTop() { return &m_lastStackTop; }
    void* lastStackTop() { return m_lastStackTop; }
    void setLastStackTop(const Thread&);
    
    void firePrimitiveGigacageEnabledIfNecessary()
    {
        if (m_needToFirePrimitiveGigacageEnabled) {
            m_needToFirePrimitiveGigacageEnabled = false;
            m_primitiveGigacageEnabled.fireAll(*this, "Primitive gigacage disabled asynchronously");
        }
    }

    EncodedJSValue encodedHostCallReturnValue { };
    unsigned varargsLength;
    CallFrame* newCallFrameReturnValue;
    CallFrame* callFrameForCatch;
    CalleeBits calleeForWasmCatch;
    void* targetMachinePCForThrow;
    const Instruction* targetInterpreterPCForThrow;
    uint32_t osrExitIndex;
    void* osrExitJumpDestination;
    RegExp* m_executingRegExp { nullptr };

    // The threading protocol here is as follows:
    // - You can call scratchBufferForSize from any thread.
    // - You can only set the ScratchBuffer's activeLength from the main thread.
    // - You can only write to entries in the ScratchBuffer from the main thread.
    ScratchBuffer* scratchBufferForSize(size_t size);
    void clearScratchBuffers();

    EncodedJSValue* exceptionFuzzingBuffer(size_t size)
    {
        ASSERT(Options::useExceptionFuzz());
        if (!m_exceptionFuzzBuffer)
#if PLATFORM(WKC)
#ifdef malloc
#undef malloc
#define MACRO_MALLOC_DEFINED
#endif
#endif
            m_exceptionFuzzBuffer = MallocPtr<EncodedJSValue, VMMalloc>::malloc(size);
#if PLATFORM(WKC)
#ifdef MACRO_MALLOC_DEFINED
#define malloc wkc_malloc
#endif
#endif
        return m_exceptionFuzzBuffer.get();
    }

    void gatherScratchBufferRoots(ConservativeRoots&);

    static constexpr unsigned expectedMaxActiveSideStateCount = 4;
    void pushCheckpointOSRSideState(std::unique_ptr<CheckpointOSRExitSideState>&&);
    std::unique_ptr<CheckpointOSRExitSideState> popCheckpointOSRSideState(CallFrame* expectedFrame);
    void popAllCheckpointOSRSideStateUntil(CallFrame* targetFrame);
    bool hasCheckpointOSRSideState() const { return m_checkpointSideState.size(); }
    void scanSideState(ConservativeRoots&) const;

    unsigned disallowVMEntryCount { 0 };
    VMEntryScope* entryScope;

    JSObject* stringRecursionCheckFirstObject { nullptr };
    HashSet<JSObject*> stringRecursionCheckVisitedObjects;

    DateCache dateCache;

    std::unique_ptr<Profiler::Database> m_perBytecodeProfiler;
    RefPtr<TypedArrayController> m_typedArrayController;
    RegExpCache* m_regExpCache;
    BumpPointerAllocator m_regExpAllocator;
    ConcurrentJSLock m_regExpAllocatorLock;

#if ENABLE(YARR_JIT_ALL_PARENS_EXPRESSIONS)
    static constexpr size_t patternContextBufferSize = 8192; // Space allocated to save nested parenthesis context
    UniqueArray<char> m_regExpPatternContexBuffer;
    Lock m_regExpPatternContextLock;
    char* acquireRegExpPatternContexBuffer() WTF_ACQUIRES_LOCK(m_regExpPatternContextLock);
    void releaseRegExpPatternContexBuffer() WTF_RELEASES_LOCK(m_regExpPatternContextLock);
#else
    static constexpr size_t patternContextBufferSize = 0; // Space allocated to save nested parenthesis context
#endif

    Ref<CompactTDZEnvironmentMap> m_compactVariableMap;

    std::unique_ptr<HasOwnPropertyCache> m_hasOwnPropertyCache;
    ALWAYS_INLINE HasOwnPropertyCache* hasOwnPropertyCache() { return m_hasOwnPropertyCache.get(); }
    HasOwnPropertyCache* ensureHasOwnPropertyCache();

#if ENABLE(REGEXP_TRACING)
    typedef ListHashSet<RegExp*> RTTraceList;
    RTTraceList* m_rtTraceList;
#endif

    void resetDateCache() { dateCache.reset(); }

    RegExpCache* regExpCache() { return m_regExpCache; }
#if ENABLE(REGEXP_TRACING)
    void addRegExpToTrace(RegExp*);
#endif
    JS_EXPORT_PRIVATE void dumpRegExpTrace();

    bool isCollectorBusyOnCurrentThread() { return heap.currentThreadIsDoingGCWork(); }

#if ENABLE(GC_VALIDATION)
    bool isInitializingObject() const; 
    void setInitializingObjectClass(const ClassInfo*);
#endif

    bool currentThreadIsHoldingAPILock() const { return m_apiLock->currentThreadIsHoldingLock(); }

    JSLock& apiLock() { return *m_apiLock; }
    CodeCache* codeCache() { return m_codeCache.get(); }
#if ENABLE(WKC_INTL)
    IntlCache& intlCache() { return *m_intlCache; }
#endif

    JS_EXPORT_PRIVATE void whenIdle(Function<void()>&&);

    JS_EXPORT_PRIVATE void deleteAllCode(DeleteAllCodeEffort);
    JS_EXPORT_PRIVATE void deleteAllLinkedCode(DeleteAllCodeEffort);

    void shrinkFootprintWhenIdle();

    WatchpointSet* ensureWatchpointSetForImpureProperty(UniquedStringImpl*);
    
    // FIXME: Use AtomString once it got merged with Identifier.
    JS_EXPORT_PRIVATE void addImpureProperty(UniquedStringImpl*);
    
    InlineWatchpointSet& primitiveGigacageEnabled() { return m_primitiveGigacageEnabled; }

    BuiltinExecutables* builtinExecutables() { return m_builtinExecutables.get(); }

    bool enableTypeProfiler();
    bool disableTypeProfiler();
    TypeProfilerLog* typeProfilerLog() { return m_typeProfilerLog.get(); }
    TypeProfiler* typeProfiler() { return m_typeProfiler.get(); }
    JS_EXPORT_PRIVATE void dumpTypeProfilerData();

    FunctionHasExecutedCache* functionHasExecutedCache() { return &m_functionHasExecutedCache; }

    ControlFlowProfiler* controlFlowProfiler() { return m_controlFlowProfiler.get(); }
    bool enableControlFlowProfiler();
    bool disableControlFlowProfiler();

    void queueMicrotask(JSGlobalObject&, Ref<Microtask>&&);
    JS_EXPORT_PRIVATE void drainMicrotasks();
    void setOnEachMicrotaskTick(WTF::Function<void(VM&)>&& func) { m_onEachMicrotaskTick = WTFMove(func); }
    void finalizeSynchronousJSExecution() { ASSERT(currentThreadIsHoldingAPILock()); m_currentWeakRefVersion++; }
    uintptr_t currentWeakRefVersion() const { return m_currentWeakRefVersion; }

    void setGlobalConstRedeclarationShouldThrow(bool globalConstRedeclarationThrow) { m_globalConstRedeclarationShouldThrow = globalConstRedeclarationThrow; }
    ALWAYS_INLINE bool globalConstRedeclarationShouldThrow() const { return m_globalConstRedeclarationShouldThrow; }

    void setShouldBuildPCToCodeOriginMapping() { m_shouldBuildPCToCodeOriginMapping = true; }
    bool shouldBuilderPCToCodeOriginMapping() const { return m_shouldBuildPCToCodeOriginMapping; }

    BytecodeIntrinsicRegistry& bytecodeIntrinsicRegistry() { return *m_bytecodeIntrinsicRegistry; }
    
    ShadowChicken* shadowChicken() { return m_shadowChicken.get(); }
    void ensureShadowChicken();
    
    template<typename Func>
    void logEvent(CodeBlock*, const char* summary, const Func& func);

    std::optional<RefPtr<Thread>> ownerThread() const { return m_apiLock->ownerThread(); }

    VMTraps& traps() { return m_traps; }

    JS_EXPORT_PRIVATE bool hasExceptionsAfterHandlingTraps();

    // These may be called concurrently from another thread.
    void notifyNeedDebuggerBreak() { m_traps.fireTrap(VMTraps::NeedDebuggerBreak); }
    void notifyNeedShellTimeoutCheck() { m_traps.fireTrap(VMTraps::NeedShellTimeoutCheck); }
    void notifyNeedTermination()
    {
        setTerminationInProgress(true);
        m_traps.fireTrap(VMTraps::NeedTermination);
    }
    void notifyNeedWatchdogCheck() { m_traps.fireTrap(VMTraps::NeedWatchdogCheck); }
#if PLATFORM(WKC)
    void notifyTriggerForceTerminate() { m_traps.setTrapBit(VMTraps::NeedWatchdogCheck); }
#endif

    void promiseRejected(JSPromise*);

#if ENABLE(EXCEPTION_SCOPE_VERIFICATION)
    StackTrace* nativeStackTraceOfLastThrow() const { return m_nativeStackTraceOfLastThrow.get(); }
    Thread* throwingThread() const { return m_throwingThread.get(); }
    bool needExceptionCheck() const { return m_needExceptionCheck; }
#endif

    WTF::RunLoop& runLoop() const { return m_runLoop; }

    static void setCrashOnVMCreation(bool);

    class DeferExceptionScope {
    public:
        DeferExceptionScope(VM& vm)
            : m_vm(vm)
            , m_exceptionWasSet(vm.m_exception)
            , m_savedException(vm.m_exception, nullptr)
            , m_savedLastException(vm.m_lastException, nullptr)
        {
            if (m_exceptionWasSet)
                m_vm.traps().clearTrapBit(VMTraps::NeedExceptionHandling);
        }

        ~DeferExceptionScope()
        {
            if (m_exceptionWasSet)
                m_vm.traps().setTrapBit(VMTraps::NeedExceptionHandling);
        }

    private:
        VM& m_vm;
        bool m_exceptionWasSet;
        SetForScope<Exception*> m_savedException;
        SetForScope<Exception*> m_savedLastException;
    };

    void addLoopHintExecutionCounter(const Instruction*);
    uintptr_t* getLoopHintExecutionCounter(const Instruction*);
    void removeLoopHintExecutionCounter(const Instruction*);

    ALWAYS_INLINE void writeBarrier(const JSCell* from) { heap.writeBarrier(from); }
    ALWAYS_INLINE void writeBarrier(const JSCell* from, JSValue to) { heap.writeBarrier(from, to); }
    ALWAYS_INLINE void writeBarrier(const JSCell* from, JSCell* to) { heap.writeBarrier(from, to); }
    ALWAYS_INLINE void writeBarrierSlowPath(const JSCell* from) { heap.writeBarrierSlowPath(from); }

    ALWAYS_INLINE void mutatorFence() { heap.mutatorFence(); }

#if ENABLE(DFG_DOES_GC_VALIDATION)
    DoesGCCheck* addressOfDoesGC() { return &m_doesGC; }
    void setDoesGCExpectation(bool expectDoesGC, unsigned nodeIndex, unsigned nodeOp) { m_doesGC.set(expectDoesGC, nodeIndex, nodeOp); }
    void setDoesGCExpectation(bool expectDoesGC, DoesGCCheck::Special special) { m_doesGC.set(expectDoesGC, special); }
    void verifyCanGC() { m_doesGC.verifyCanGC(*this); }
#else
    DoesGCCheck* addressOfDoesGC() { UNREACHABLE_FOR_PLATFORM(); return nullptr; }
    void setDoesGCExpectation(bool, unsigned, unsigned) { }
    void setDoesGCExpectation(bool, DoesGCCheck::Special) { }
    void verifyCanGC() { }
#endif

private:
    friend class LLIntOffsetsExtractor;

    VM(VMType, HeapType, WTF::RunLoop* = nullptr, bool* success = nullptr);
    static VM*& sharedInstanceInternal();
    void createNativeThunk();

    JS_EXPORT_PRIVATE JSCell* sentinelSetBucketSlow();
    JS_EXPORT_PRIVATE JSCell* sentinelMapBucketSlow();
    JSPropertyNameEnumerator* emptyPropertyNameEnumeratorSlow();

    void updateStackLimits();

    bool isSafeToRecurse(void* stackLimit) const
    {
        void* curr = currentStackPointer();
        return curr >= stackLimit;
    }

    Exception* exception() const
    {
#if ENABLE(EXCEPTION_SCOPE_VERIFICATION)
        m_needExceptionCheck = false;
#endif
        return m_exception;
    }

    JS_EXPORT_PRIVATE void clearException();
    JS_EXPORT_PRIVATE void setException(Exception*);

#if ENABLE(C_LOOP)
    bool ensureStackCapacityForCLoop(Register* newTopOfStack);
    bool isSafeToRecurseSoftCLoop() const;
#endif // ENABLE(C_LOOP)

    JS_EXPORT_PRIVATE Exception* throwException(JSGlobalObject*, Exception*);
    JS_EXPORT_PRIVATE Exception* throwException(JSGlobalObject*, JSValue);
    JS_EXPORT_PRIVATE Exception* throwException(JSGlobalObject*, JSObject*);

#if ENABLE(EXCEPTION_SCOPE_VERIFICATION)
    void verifyExceptionCheckNeedIsSatisfied(unsigned depth, ExceptionEventLocation&);
#endif
    
    static void primitiveGigacageDisabledCallback(void*);
    void primitiveGigacageDisabled();

    void callPromiseRejectionCallback(Strong<JSPromise>&);
    void didExhaustMicrotaskQueue();

#if ENABLE(GC_VALIDATION)
    const ClassInfo* m_initializingObjectClass;
#endif

    void* m_stackPointerAtVMEntry;
    size_t m_currentSoftReservedZoneSize;
    void* m_stackLimit { nullptr };
    void* m_softStackLimit { nullptr };
#if ENABLE(C_LOOP)
    void* m_cloopStackLimit { nullptr };
#endif
    void* m_lastStackTop { nullptr };

    Exception* m_exception { nullptr };
    Exception* m_terminationException { nullptr };
    Exception* m_lastException { nullptr };
#if ENABLE(EXCEPTION_SCOPE_VERIFICATION)
    ExceptionScope* m_topExceptionScope { nullptr };
    ExceptionEventLocation m_simulatedThrowPointLocation;
    unsigned m_simulatedThrowPointRecursionDepth { 0 };
    mutable bool m_needExceptionCheck { false };
    std::unique_ptr<StackTrace> m_nativeStackTraceOfLastThrow;
    std::unique_ptr<StackTrace> m_nativeStackTraceOfLastSimulatedThrow;
    RefPtr<Thread> m_throwingThread;
#endif

public:
    bool didEnterVM { false };
private:
    bool m_failNextNewCodeBlock { false };
    bool m_globalConstRedeclarationShouldThrow { true };
    bool m_shouldBuildPCToCodeOriginMapping { false };
    DeletePropertyMode m_deletePropertyMode { DeletePropertyMode::Default };
    HeapAnalyzer* m_activeHeapAnalyzer { nullptr };
    std::unique_ptr<CodeCache> m_codeCache;
#if ENABLE(WKC_INTL)
    std::unique_ptr<IntlCache> m_intlCache;
#endif
    std::unique_ptr<BuiltinExecutables> m_builtinExecutables;
    HashMap<RefPtr<UniquedStringImpl>, RefPtr<WatchpointSet>> m_impurePropertyWatchpointSets;
    std::unique_ptr<TypeProfiler> m_typeProfiler;
    std::unique_ptr<TypeProfilerLog> m_typeProfilerLog;
    unsigned m_typeProfilerEnabledCount;
    bool m_needToFirePrimitiveGigacageEnabled { false };
    Lock m_scratchBufferLock;
    Vector<ScratchBuffer*> m_scratchBuffers;
    size_t m_sizeOfLastScratchBuffer { 0 };
    Vector<std::unique_ptr<CheckpointOSRExitSideState>, expectedMaxActiveSideStateCount> m_checkpointSideState;
    InlineWatchpointSet m_primitiveGigacageEnabled;
    FunctionHasExecutedCache m_functionHasExecutedCache;
    std::unique_ptr<ControlFlowProfiler> m_controlFlowProfiler;
    unsigned m_controlFlowProfilerEnabledCount;
    Deque<std::unique_ptr<QueuedTask>> m_microtaskQueue;
    MallocPtr<EncodedJSValue, VMMalloc> m_exceptionFuzzBuffer;
    VMTraps m_traps;
    RefPtr<Watchdog> m_watchdog;
    std::unique_ptr<HeapProfiler> m_heapProfiler;
#if ENABLE(SAMPLING_PROFILER)
    RefPtr<SamplingProfiler> m_samplingProfiler;
#endif
    std::unique_ptr<FuzzerAgent> m_fuzzerAgent;
    std::unique_ptr<ShadowChicken> m_shadowChicken;
    std::unique_ptr<BytecodeIntrinsicRegistry> m_bytecodeIntrinsicRegistry;

    // FIXME: We should remove handled promises from this list at GC flip. <https://webkit.org/b/201005>
    Vector<Strong<JSPromise>> m_aboutToBeNotifiedRejectedPromises;

    WTF::Function<void(VM&)> m_onEachMicrotaskTick;
    uintptr_t m_currentWeakRefVersion { 0 };

    bool m_terminationInProgress { false };
    bool m_executionForbidden { false };
    bool m_executionForbiddenOnTermination { false };

    Lock m_loopHintExecutionCountLock;
    HashMap<const Instruction*, std::pair<unsigned, std::unique_ptr<uintptr_t>>> m_loopHintExecutionCounts;

#if ENABLE(DFG_DOES_GC_VALIDATION)
    DoesGCCheck m_doesGC;
#endif

    VM* m_prev; // Required by DoublyLinkedListNode.
    VM* m_next; // Required by DoublyLinkedListNode.

    // Friends for exception checking purpose only.
    friend class Heap;
    friend class CatchScope;
    friend class ExceptionScope;
    friend class JSDollarVMHelper;
    friend class ThrowScope;
    friend class VMTraps;
    friend class WTF::DoublyLinkedListNode<VM>;
};

#if ENABLE(GC_VALIDATION)
inline bool VM::isInitializingObject() const
{
    return !!m_initializingObjectClass;
}

inline void VM::setInitializingObjectClass(const ClassInfo* initializingObjectClass)
{
    m_initializingObjectClass = initializingObjectClass;
}
#endif

inline Heap* WeakSet::heap() const
{
    return &m_vm->heap;
}

#if !ENABLE(C_LOOP)
extern "C" void sanitizeStackForVMImpl(VM*);
#endif

JS_EXPORT_PRIVATE void sanitizeStackForVM(VM&);

} // namespace JSC
