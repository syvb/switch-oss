/*
 * Copyright (C) 2008-2022 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Cameron Zwarich <cwzwarich@uwaterloo.ca>
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

#include "config.h"
#include "Interpreter.h"

#include "AbstractModuleRecord.h"
#include "BatchedTransitionOptimizer.h"
#include "Bytecodes.h"
#include "CallFrameClosure.h"
#include "CatchScope.h"
#include "CheckpointOSRExitSideState.h"
#include "CodeBlock.h"
#include "DirectArguments.h"
#include "Debugger.h"
#include "DirectEvalCodeCache.h"
#include "EvalCodeBlock.h"
#include "ExecutableBaseInlines.h"
#include "FrameTracers.h"
#include "InterpreterInlines.h"
#include "JITCodeInlines.h"
#include "JSArrayInlines.h"
#include "JSCInlines.h"
#include "JSImmutableButterfly.h"
#include "JSLexicalEnvironment.h"
#include "JSModuleEnvironment.h"
#include "JSModuleRecord.h"
#include "JSString.h"
#include "JSWebAssemblyException.h"
#include "LLIntThunks.h"
#include "LiteralParser.h"
#include "ModuleProgramCodeBlock.h"
#include "ProgramCodeBlock.h"
#include "ProtoCallFrameInlines.h"
#include "Register.h"
#include "RegisterAtOffsetList.h"
#include "ScopedArguments.h"
#include "StackFrame.h"
#include "StackVisitor.h"
#include "StrictEvalActivation.h"
#include "VMEntryScope.h"
#include "VMInlines.h"
#include "VMTrapsInlines.h"
#include "VirtualRegister.h"
#include <stdio.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/Scope.h>
#include <wtf/StdLibExtras.h>
#include <wtf/text/StringBuilder.h>

#if ENABLE(WEBASSEMBLY)
#include "JSWebAssemblyInstance.h"
#include "WasmContextInlines.h"
#include "WebAssemblyFunction.h"
#endif

namespace JSC {

#if PLATFORM(WKC)
    static bool isStackOverflow(size_t margin)
    {
        size_t consumption;
        void* stack_top;
        bool stack_overflow;

        stack_overflow = wkcMemoryIsStackOverflowPeer(margin, &consumption, &stack_top);
        return stack_overflow;
    }
#define IS_STACK_OVERFLOW(margin)   JSC::isStackOverflow((margin))
#endif

JSValue eval(JSGlobalObject* globalObject, CallFrame* callFrame, ECMAMode ecmaMode)
{
    VM& vm = globalObject->vm();
    auto scope = DECLARE_THROW_SCOPE(vm);

    auto clobberizeValidator = makeScopeExit([&] {
        vm.didEnterVM = true;
    });

    if (!callFrame->argumentCount())
        return jsUndefined();

    JSValue program = callFrame->argument(0);
    if (!program.isString())
        return program;

    auto* programString = asString(program);

    TopCallFrameSetter topCallFrame(vm, callFrame);
    if (!globalObject->evalEnabled()) {
        globalObject->globalObjectMethodTable()->reportViolationForUnsafeEval(globalObject, programString);
        throwException(globalObject, scope, createEvalError(globalObject, globalObject->evalDisabledErrorMessage()));
        return jsUndefined();
    }
    String programSource = programString->value(globalObject);
    RETURN_IF_EXCEPTION(scope, JSValue());
    
    CallFrame* callerFrame = callFrame->callerFrame();
    CallSiteIndex callerCallSiteIndex = callerFrame->callSiteIndex();
    CodeBlock* callerCodeBlock = callerFrame->codeBlock();
    JSScope* callerScopeChain = callerFrame->uncheckedR(callerCodeBlock->scopeRegister()).Register::scope();
    UnlinkedCodeBlock* callerUnlinkedCodeBlock = callerCodeBlock->unlinkedCodeBlock();

    bool isArrowFunctionContext = callerUnlinkedCodeBlock->isArrowFunction() || callerUnlinkedCodeBlock->isArrowFunctionContext();

    DerivedContextType derivedContextType = callerUnlinkedCodeBlock->derivedContextType();
    if (!isArrowFunctionContext && callerUnlinkedCodeBlock->isClassContext()) {
        derivedContextType = callerUnlinkedCodeBlock->isConstructor()
            ? DerivedContextType::DerivedConstructorContext
            : DerivedContextType::DerivedMethodContext;
    }

    EvalContextType evalContextType;
    if (callerUnlinkedCodeBlock->parseMode() == SourceParseMode::ClassFieldInitializerMode)
        evalContextType = EvalContextType::InstanceFieldEvalContext;
    else if (isFunctionParseMode(callerUnlinkedCodeBlock->parseMode()))
        evalContextType = EvalContextType::FunctionEvalContext;
    else if (callerUnlinkedCodeBlock->codeType() == EvalCode)
        evalContextType = callerUnlinkedCodeBlock->evalContextType();
    else
        evalContextType = EvalContextType::None;

    DirectEvalExecutable* eval = callerCodeBlock->directEvalCodeCache().tryGet(programSource, callerCallSiteIndex);
    if (!eval) {
        if (!ecmaMode.isStrict()) {
            if (programSource.is8Bit()) {
                LiteralParser<LChar> preparser(globalObject, programSource.characters8(), programSource.length(), NonStrictJSON, callerCodeBlock);
                if (JSValue parsedObject = preparser.tryLiteralParse())
                    RELEASE_AND_RETURN(scope, parsedObject);

            } else {
                LiteralParser<UChar> preparser(globalObject, programSource.characters16(), programSource.length(), NonStrictJSON, callerCodeBlock);
                if (JSValue parsedObject = preparser.tryLiteralParse())
                    RELEASE_AND_RETURN(scope, parsedObject);

            }
            RETURN_IF_EXCEPTION(scope, JSValue());
        }
        
        TDZEnvironment variablesUnderTDZ;
        PrivateNameEnvironment privateNameEnvironment;
        JSScope::collectClosureVariablesUnderTDZ(callerScopeChain, variablesUnderTDZ, privateNameEnvironment);
        eval = DirectEvalExecutable::create(globalObject, makeSource(programSource, callerCodeBlock->source().provider()->sourceOrigin()), derivedContextType, callerUnlinkedCodeBlock->needsClassFieldInitializer(), callerUnlinkedCodeBlock->privateBrandRequirement(), isArrowFunctionContext, callerCodeBlock->ownerExecutable()->isInsideOrdinaryFunction(), evalContextType, &variablesUnderTDZ, &privateNameEnvironment, ecmaMode);
        EXCEPTION_ASSERT(!!scope.exception() == !eval);
        if (!eval)
            return jsUndefined();

        callerCodeBlock->directEvalCodeCache().set(globalObject, callerCodeBlock, programSource, callerCallSiteIndex, eval);
    }

    JSValue thisValue = callerFrame->thisValue();
    Interpreter* interpreter = vm.interpreter;
    RELEASE_AND_RETURN(scope, interpreter->execute(eval, globalObject, thisValue, callerScopeChain));
}

unsigned sizeOfVarargs(JSGlobalObject* globalObject, JSValue arguments, uint32_t firstVarArgOffset)
{
    VM& vm = globalObject->vm();
    auto scope = DECLARE_THROW_SCOPE(vm);

    if (UNLIKELY(!arguments.isCell())) {
        if (arguments.isUndefinedOrNull())
            return 0;
        
        throwException(globalObject, scope, createInvalidFunctionApplyParameterError(globalObject, arguments));
        return 0;
    }
    
    JSCell* cell = arguments.asCell();
    unsigned length;
    switch (cell->type()) {
    case DirectArgumentsType:
        length = jsCast<DirectArguments*>(cell)->length(globalObject);
        break;
    case ScopedArgumentsType:
        length = jsCast<ScopedArguments*>(cell)->length(globalObject);
        break;
    case JSImmutableButterflyType:
        length = jsCast<JSImmutableButterfly*>(cell)->length();
        break;
    case StringType:
    case SymbolType:
    case HeapBigIntType:
        throwException(globalObject, scope, createInvalidFunctionApplyParameterError(globalObject,  arguments));
        return 0;
        
    default:
        RELEASE_ASSERT(arguments.isObject());
        length = clampToUnsigned(toLength(globalObject, jsCast<JSObject*>(cell)));
        break;
    }
    RETURN_IF_EXCEPTION(scope, 0);
    
    if (length > maxArguments)
        throwStackOverflowError(globalObject, scope);

    if (length >= firstVarArgOffset)
        length -= firstVarArgOffset;
    else
        length = 0;
    
    return length;
}

unsigned sizeFrameForForwardArguments(JSGlobalObject* globalObject, CallFrame* callFrame, VM& vm, unsigned numUsedStackSlots)
{
    auto scope = DECLARE_THROW_SCOPE(vm);

    unsigned length = callFrame->argumentCount();
    CallFrame* calleeFrame = calleeFrameForVarargs(callFrame, numUsedStackSlots, length + 1);
    if (UNLIKELY(!vm.ensureStackCapacityFor(calleeFrame->registers())))
        throwStackOverflowError(globalObject, scope);

    return length;
}

unsigned sizeFrameForVarargs(JSGlobalObject* globalObject, CallFrame* callFrame, VM& vm, JSValue arguments, unsigned numUsedStackSlots, uint32_t firstVarArgOffset)
{
    auto scope = DECLARE_THROW_SCOPE(vm);

    unsigned length = sizeOfVarargs(globalObject, arguments, firstVarArgOffset);
    RETURN_IF_EXCEPTION(scope, 0);

    CallFrame* calleeFrame = calleeFrameForVarargs(callFrame, numUsedStackSlots, length + 1);
    if (UNLIKELY(length > maxArguments || !vm.ensureStackCapacityFor(calleeFrame->registers()))) {
        throwStackOverflowError(globalObject, scope);
        return 0;
    }
    
    return length;
}

void loadVarargs(JSGlobalObject* globalObject, JSValue* firstElementDest, JSValue arguments, uint32_t offset, uint32_t length)
{
    if (UNLIKELY(!arguments.isCell()) || !length)
        return;

    VM& vm = globalObject->vm();
    auto scope = DECLARE_THROW_SCOPE(vm);
    JSCell* cell = arguments.asCell();

    switch (cell->type()) {
    case DirectArgumentsType:
        scope.release();
        jsCast<DirectArguments*>(cell)->copyToArguments(globalObject, firstElementDest, offset, length);
        return;
    case ScopedArgumentsType:
        scope.release();
        jsCast<ScopedArguments*>(cell)->copyToArguments(globalObject, firstElementDest, offset, length);
        return;
    case JSImmutableButterflyType:
        scope.release();
        jsCast<JSImmutableButterfly*>(cell)->copyToArguments(globalObject, firstElementDest, offset, length);
        return; 
    default: {
        ASSERT(arguments.isObject());
        JSObject* object = jsCast<JSObject*>(cell);
        if (isJSArray(object)) {
            scope.release();
            jsCast<JSArray*>(object)->copyToArguments(globalObject, firstElementDest, offset, length);
            return;
        }
        unsigned i;
        for (i = 0; i < length && object->canGetIndexQuickly(i + offset); ++i)
            firstElementDest[i] = object->getIndexQuickly(i + offset);
        for (; i < length; ++i) {
            JSValue value = object->get(globalObject, i + offset);
            RETURN_IF_EXCEPTION(scope, void());
            firstElementDest[i] = value;
        }
        return;
    } }
}

void setupVarargsFrame(JSGlobalObject* globalObject, CallFrame* callFrame, CallFrame* newCallFrame, JSValue arguments, uint32_t offset, uint32_t length)
{
    VirtualRegister calleeFrameOffset(newCallFrame - callFrame);
    
    loadVarargs(
        globalObject,
        bitwise_cast<JSValue*>(&callFrame->r(calleeFrameOffset + CallFrame::argumentOffset(0))),
        arguments, offset, length);
    
    newCallFrame->setArgumentCountIncludingThis(length + 1);
}

void setupVarargsFrameAndSetThis(JSGlobalObject* globalObject, CallFrame* callFrame, CallFrame* newCallFrame, JSValue thisValue, JSValue arguments, uint32_t firstVarArgOffset, uint32_t length)
{
    setupVarargsFrame(globalObject, callFrame, newCallFrame, arguments, firstVarArgOffset, length);
    newCallFrame->setThisValue(thisValue);
}

void setupForwardArgumentsFrame(JSGlobalObject*, CallFrame* execCaller, CallFrame* execCallee, uint32_t length)
{
    ASSERT(length == execCaller->argumentCount());
    unsigned offset = execCaller->argumentOffset(0) * sizeof(Register);
    memcpy(reinterpret_cast<char*>(execCallee) + offset, reinterpret_cast<char*>(execCaller) + offset, length * sizeof(Register));
    execCallee->setArgumentCountIncludingThis(length + 1);
}

void setupForwardArgumentsFrameAndSetThis(JSGlobalObject* globalObject, CallFrame* execCaller, CallFrame* execCallee, JSValue thisValue, uint32_t length)
{
    setupForwardArgumentsFrame(globalObject, execCaller, execCallee, length);
    execCallee->setThisValue(thisValue);
}

    

Interpreter::Interpreter(VM& vm)
    : m_vm(vm)
#if ENABLE(C_LOOP)
    , m_cloopStack(vm)
#endif
{
#if PLATFORM(WKC)
#if ENABLE(COMPUTED_GOTO_OPCODES)
    opcodeIDTable();
#endif
#endif
#if ASSERT_ENABLED
    static WKCStd::once_flag assertOnceKey;
    WKCStd::call_once(assertOnceKey, [] {
        for (unsigned i = 0; i < NUMBER_OF_BYTECODE_IDS; ++i) {
            OpcodeID opcodeID = static_cast<OpcodeID>(i);
            RELEASE_ASSERT(getOpcodeID(getOpcode(opcodeID)) == opcodeID);
        }
    });
#endif // ASSERT_ENABLED
}

Interpreter::~Interpreter()
{
}

#if ENABLE(COMPUTED_GOTO_OPCODES)
#if !ENABLE(LLINT_EMBEDDED_OPCODE_ID) || ASSERT_ENABLED
HashMap<Opcode, OpcodeID>& Interpreter::opcodeIDTable()
{
    static LazyNeverDestroyed<HashMap<Opcode, OpcodeID>> opcodeIDTable;

    static WKCStd::once_flag initializeKey;
    WKCStd::call_once(initializeKey, [&] {
        opcodeIDTable.construct();
        const Opcode* opcodeTable = LLInt::opcodeMap();
        for (unsigned i = 0; i < NUMBER_OF_BYTECODE_IDS; ++i)
            opcodeIDTable->add(opcodeTable[i], static_cast<OpcodeID>(i));
    });

    return opcodeIDTable;
}
#endif // !ENABLE(LLINT_EMBEDDED_OPCODE_ID) || ASSERT_ENABLED
#endif // ENABLE(COMPUTED_GOTO_OPCODES)

#if ASSERT_ENABLED
bool Interpreter::isOpcode(Opcode opcode)
{
#if ENABLE(COMPUTED_GOTO_OPCODES)
    return opcode != HashTraits<Opcode>::emptyValue()
        && !HashTraits<Opcode>::isDeletedValue(opcode)
        && opcodeIDTable().contains(opcode);
#else
    return opcode >= 0 && opcode <= op_end;
#endif
}
#endif // ASSERT_ENABLED

class GetStackTraceFunctor {
public:
    GetStackTraceFunctor(VM& vm, JSCell* owner, Vector<StackFrame>& results, size_t framesToSkip, size_t capacity)
        : m_vm(vm)
        , m_owner(owner)
        , m_results(results)
        , m_framesToSkip(framesToSkip)
        , m_remainingCapacityForFrameCapture(capacity)
    {
        m_results.reserveInitialCapacity(capacity);
    }

    StackVisitor::Status operator()(StackVisitor& visitor) const
    {
        if (m_framesToSkip > 0) {
            m_framesToSkip--;
            return StackVisitor::Continue;
        }

        if (m_remainingCapacityForFrameCapture) {
            if (visitor->isWasmFrame()) {
                m_results.append(StackFrame(visitor->wasmFunctionIndexOrName()));
            } else if (!!visitor->codeBlock() && !visitor->codeBlock()->unlinkedCodeBlock()->isBuiltinFunction()) {
                m_results.append(
                    StackFrame(m_vm, m_owner, visitor->callee().asCell(), visitor->codeBlock(), visitor->bytecodeIndex()));
            } else {
                m_results.append(
                    StackFrame(m_vm, m_owner, visitor->callee().asCell()));
            }
    
            m_remainingCapacityForFrameCapture--;
            return StackVisitor::Continue;
        }
        return StackVisitor::Done;
    }

private:
    VM& m_vm;
    JSCell* m_owner;
    Vector<StackFrame>& m_results;
    mutable size_t m_framesToSkip;
    mutable size_t m_remainingCapacityForFrameCapture;
};

void Interpreter::getStackTrace(JSCell* owner, Vector<StackFrame>& results, size_t framesToSkip, size_t maxStackSize)
{
    DisallowGC disallowGC;
    VM& vm = m_vm;
    CallFrame* callFrame = vm.topCallFrame;
    if (!callFrame || !maxStackSize)
        return;

    size_t framesCount = 0;
    size_t maxFramesCountNeeded = maxStackSize + framesToSkip;
    StackVisitor::visit(callFrame, vm, [&] (StackVisitor&) -> StackVisitor::Status {
        if (++framesCount < maxFramesCountNeeded)
            return StackVisitor::Continue;
        return StackVisitor::Done;
    });
    if (framesCount <= framesToSkip)
        return;

    framesCount -= framesToSkip;
    framesCount = std::min(maxStackSize, framesCount);

    GetStackTraceFunctor functor(vm, owner, results, framesToSkip, framesCount);
    StackVisitor::visit(callFrame, vm, functor);
    ASSERT(results.size() == results.capacity());
}

String Interpreter::stackTraceAsString(VM& vm, const Vector<StackFrame>& stackTrace)
{
    // FIXME: JSStringJoiner could be more efficient than StringBuilder here.
    StringBuilder builder;
    for (unsigned i = 0; i < stackTrace.size(); i++) {
        builder.append(String(stackTrace[i].toString(vm)));
        if (i != stackTrace.size() - 1)
            builder.append('\n');
    }
    return builder.toString();
}

ALWAYS_INLINE static HandlerInfo* findExceptionHandler(StackVisitor& visitor, CodeBlock* codeBlock, RequiredHandler requiredHandler)
{
    ASSERT(codeBlock);
#if ENABLE(DFG_JIT)
    ASSERT(!visitor->isInlinedFrame());
#endif

    CallFrame* callFrame = visitor->callFrame();
    unsigned exceptionHandlerIndex;
    if (JITCode::isOptimizingJIT(codeBlock->jitType()))
        exceptionHandlerIndex = callFrame->callSiteIndex().bits();
    else
        exceptionHandlerIndex = callFrame->bytecodeIndex().offset();

    return codeBlock->handlerForIndex(exceptionHandlerIndex, requiredHandler);
}

class GetCatchHandlerFunctor {
public:
    GetCatchHandlerFunctor()
        : m_handler(nullptr)
    {
    }

    HandlerInfo* handler() { return m_handler; }

    StackVisitor::Status operator()(StackVisitor& visitor) const
    {
        visitor.unwindToMachineCodeBlockFrame();

        CodeBlock* codeBlock = visitor->codeBlock();
        if (!codeBlock)
            return StackVisitor::Continue;

        m_handler = findExceptionHandler(visitor, codeBlock, RequiredHandler::CatchHandler);
        if (m_handler)
            return StackVisitor::Done;

        return StackVisitor::Continue;
    }

private:
    mutable HandlerInfo* m_handler;
};

ALWAYS_INLINE static void notifyDebuggerOfUnwinding(VM& vm, CallFrame* callFrame)
{
    JSGlobalObject* globalObject = callFrame->lexicalGlobalObject(vm);
    Debugger* debugger = globalObject->debugger();
    if (LIKELY(!debugger))
        return;

    DeferTermination deferScope(vm);
    auto catchScope = DECLARE_CATCH_SCOPE(vm);

    SuspendExceptionScope scope(&vm);
    if (callFrame->isAnyWasmCallee()
        || (callFrame->callee().isCell() && callFrame->callee().asCell()->inherits<JSFunction>(vm)))
        debugger->unwindEvent(callFrame);
    else
        debugger->didExecuteProgram(callFrame);
    catchScope.assertNoException();
}

CatchInfo::CatchInfo(const HandlerInfo* handler, CodeBlock* codeBlock)
{
    m_valid = !!handler;
    if (m_valid) {
        m_type = handler->type();
#if ENABLE(JIT)
        m_nativeCode = handler->nativeCode;
#endif

        // handler->target is meaningless for getting a code offset when catching
        // the exception in a DFG/FTL frame. This bytecode target offset could be
        // something that's in an inlined frame, which means an array access
        // with this bytecode offset in the machine frame is utterly meaningless
        // and can cause an overflow. OSR exit properly exits to handler->target
        // in the proper frame.
        if (!JITCode::isOptimizingJIT(codeBlock->jitType()))
            m_catchPCForInterpreter = codeBlock->instructions().at(handler->target).ptr();
        else
            m_catchPCForInterpreter = nullptr;
    }
}

#if ENABLE(WEBASSEMBLY)
CatchInfo::CatchInfo(const Wasm::HandlerInfo* handler, const Wasm::Callee* callee)
{
    m_valid = !!handler;
    if (m_valid) {
        m_type = HandlerType::Catch;
        m_nativeCode = handler->m_nativeCode;
        if (callee->compilationMode() == Wasm::CompilationMode::LLIntMode)
            m_catchPCForInterpreter = static_cast<const Wasm::LLIntCallee*>(callee)->instructions().at(handler->m_target).ptr();
        else
            m_catchPCForInterpreter = nullptr;
    }
}
#endif

class UnwindFunctor {
public:
    UnwindFunctor(VM& vm, CallFrame*& callFrame, bool isTermination, JSValue thrownValue, CodeBlock*& codeBlock, CatchInfo& handler)
        : m_vm(vm)
        , m_callFrame(callFrame)
        , m_isTermination(isTermination)
        , m_codeBlock(codeBlock)
        , m_handler(handler)
    {
#if ENABLE(WEBASSEMBLY)
        if (!m_isTermination) {
            if (JSWebAssemblyException* wasmException = jsDynamicCast<JSWebAssemblyException*>(m_vm, thrownValue)) {
                m_catchableFromWasm = true;
                m_wasmTag = &wasmException->tag();
            } else if (ErrorInstance* error = jsDynamicCast<ErrorInstance*>(m_vm, thrownValue))
                m_catchableFromWasm = error->isCatchableFromWasm();
        }
#else
        UNUSED_PARAM(thrownValue);
#endif
    }

    StackVisitor::Status operator()(StackVisitor& visitor) const
    {
        visitor.unwindToMachineCodeBlockFrame();
        m_callFrame = visitor->callFrame();
        m_codeBlock = visitor->codeBlock();

        m_handler.m_valid = false;
        if (m_codeBlock) {
            if (!m_isTermination) {
                m_handler = { findExceptionHandler(visitor, m_codeBlock, RequiredHandler::AnyHandler), m_codeBlock };
                if (m_handler.m_valid)
                    return StackVisitor::Done;
            }
        }

#if ENABLE(WEBASSEMBLY)
        CalleeBits callee = visitor->callee();
        if (callee.isCell()) {
            if (auto* jsToWasmICCallee = jsDynamicCast<JSToWasmICCallee*>(m_vm, callee.asCell()))
                m_vm.wasmContext.store(jsToWasmICCallee->function()->previousInstance(m_callFrame), m_vm.softStackLimit());
        }

        if (m_catchableFromWasm && callee.isWasm()) {
            Wasm::Callee* wasmCallee = callee.asWasmCallee();
            if (wasmCallee->hasExceptionHandlers()) {
                JSWebAssemblyInstance* jsInstance = jsCast<JSWebAssemblyInstance*>(m_callFrame->thisValue());
                unsigned exceptionHandlerIndex = m_callFrame->callSiteIndex().bits();
                m_handler = { wasmCallee->handlerForIndex(jsInstance->instance(), exceptionHandlerIndex, m_wasmTag), wasmCallee };
                if (m_handler.m_valid)
                    return StackVisitor::Done;
            }
        }
#endif

        notifyDebuggerOfUnwinding(m_vm, m_callFrame);

        copyCalleeSavesToEntryFrameCalleeSavesBuffer(visitor);

        bool shouldStopUnwinding = visitor->callerIsEntryFrame();
        if (shouldStopUnwinding)
            return StackVisitor::Done;

        return StackVisitor::Continue;
    }

private:
    void copyCalleeSavesToEntryFrameCalleeSavesBuffer(StackVisitor& visitor) const
    {
#if ENABLE(ASSEMBLER)
        std::optional<RegisterAtOffsetList> currentCalleeSaves = visitor->calleeSaveRegistersForUnwinding();

        if (!currentCalleeSaves)
            return;

        RegisterAtOffsetList* allCalleeSaves = RegisterSet::vmCalleeSaveRegisterOffsets();
        RegisterSet dontCopyRegisters = RegisterSet::stackRegisters();
        CPURegister* frame = reinterpret_cast<CPURegister*>(m_callFrame->registers());

        unsigned registerCount = currentCalleeSaves->size();
        VMEntryRecord* record = vmEntryRecord(m_vm.topEntryFrame);
        for (unsigned i = 0; i < registerCount; i++) {
            RegisterAtOffset currentEntry = currentCalleeSaves->at(i);
            if (dontCopyRegisters.get(currentEntry.reg()))
                continue;
            RegisterAtOffset* calleeSavesEntry = allCalleeSaves->find(currentEntry.reg());
            
            record->calleeSaveRegistersBuffer[calleeSavesEntry->offsetAsIndex()] = *(frame + currentEntry.offsetAsIndex());
        }
#else
        UNUSED_PARAM(visitor);
#endif
    }

    VM& m_vm;
    CallFrame*& m_callFrame;
    bool m_isTermination;
    CodeBlock*& m_codeBlock;
    CatchInfo& m_handler;
#if ENABLE(WEBASSEMBLY)
    const Wasm::Tag* m_wasmTag { nullptr };
    bool m_catchableFromWasm { false };
#endif
};

NEVER_INLINE CatchInfo Interpreter::unwind(VM& vm, CallFrame*& callFrame, Exception* exception)
{
    auto scope = DECLARE_CATCH_SCOPE(vm);

    ASSERT(reinterpret_cast<void*>(callFrame) != vm.topEntryFrame);
    CodeBlock* codeBlock = callFrame->codeBlock();

    JSValue exceptionValue = exception->value();
    ASSERT(!exceptionValue.isEmpty());
    ASSERT(!exceptionValue.isCell() || exceptionValue.asCell());
    // This shouldn't be possible (hence the assertions), but we're already in the slowest of
    // slow cases, so let's harden against it anyway to be safe.
    if (exceptionValue.isEmpty() || (exceptionValue.isCell() && !exceptionValue.asCell()))
        exceptionValue = jsNull();

    EXCEPTION_ASSERT_UNUSED(scope, scope.exception());

    // Calculate an exception handler vPC, unwinding call frames as necessary.
    CatchInfo catchInfo;
    UnwindFunctor functor(vm, callFrame, vm.isTerminationException(exception), exceptionValue, codeBlock, catchInfo);
    StackVisitor::visit<StackVisitor::TerminateIfTopEntryFrameIsEmpty>(callFrame, vm, functor);
    if (vm.hasCheckpointOSRSideState())
        vm.popAllCheckpointOSRSideStateUntil(callFrame);

    return catchInfo;
}

void Interpreter::notifyDebuggerOfExceptionToBeThrown(VM& vm, JSGlobalObject* globalObject, CallFrame* callFrame, Exception* exception)
{
    ASSERT(!vm.isTerminationException(exception));

    Debugger* debugger = globalObject->debugger();
    if (debugger && debugger->needsExceptionCallbacks() && !exception->didNotifyInspectorOfThrow()) {
        // This code assumes that if the debugger is enabled then there is no inlining.
        // If that assumption turns out to be false then we'll ignore the inlined call
        // frames.
        // https://bugs.webkit.org/show_bug.cgi?id=121754

        GetCatchHandlerFunctor functor;
        StackVisitor::visit(callFrame, vm, functor);
        HandlerInfo* handler = functor.handler();
        ASSERT(!handler || handler->isCatchHandler());
        bool hasCatchHandler = !!handler;

        debugger->exception(globalObject, callFrame, exception->value(), hasCatchHandler);
    }
    exception->setDidNotifyInspectorOfThrow();
}

JSValue Interpreter::executeProgram(const SourceCode& source, JSGlobalObject*, JSObject* thisObj)
{
    JSScope* scope = thisObj->globalObject()->globalScope();
    VM& vm = scope->vm();
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    JSGlobalObject* globalObject = scope->globalObject(vm);
    JSCallee* globalCallee = globalObject->globalCallee();

    VMEntryScope entryScope(vm, globalObject);

    auto clobberizeValidator = makeScopeExit([&] {
        vm.didEnterVM = true;
    });

    ProgramExecutable* program = ProgramExecutable::create(globalObject, source);
    EXCEPTION_ASSERT(throwScope.exception() || program);
    RETURN_IF_EXCEPTION(throwScope, { });

    ASSERT(!vm.isCollectorBusyOnCurrentThread());
    RELEASE_ASSERT(vm.currentThreadIsHoldingAPILock());
    if (vm.isCollectorBusyOnCurrentThread())
        return jsNull();

    if (UNLIKELY(!vm.isSafeToRecurseSoft()))
        return checkedReturn(throwStackOverflowError(globalObject, throwScope));

#if PLATFORM(WKC)
    if (IS_STACK_OVERFLOW(WKC_STACK_MARGIN_DEFAULT)) {
        return checkedReturn(throwStackOverflowError(globalObject, throwScope));
    }
#endif

    // First check if the "program" is actually just a JSON object. If so,
    // we'll handle the JSON object here. Else, we'll handle real JS code
    // below at failedJSONP.

    Vector<JSONPData> JSONPData;
    bool parseResult;
    StringView programSource = program->source().view();
    if (programSource.isNull())
        return jsUndefined();
    if (programSource.is8Bit()) {
        LiteralParser<LChar> literalParser(globalObject, programSource.characters8(), programSource.length(), JSONP);
        parseResult = literalParser.tryJSONPParse(JSONPData, globalObject->globalObjectMethodTable()->supportsRichSourceInfo(globalObject));
    } else {
        LiteralParser<UChar> literalParser(globalObject, programSource.characters16(), programSource.length(), JSONP);
        parseResult = literalParser.tryJSONPParse(JSONPData, globalObject->globalObjectMethodTable()->supportsRichSourceInfo(globalObject));
    }

    RETURN_IF_EXCEPTION(throwScope, { });
    if (parseResult) {
        JSValue result;
        for (unsigned entry = 0; entry < JSONPData.size(); entry++) {
            Vector<JSONPPathEntry> JSONPPath;
            JSONPPath.swap(JSONPData[entry].m_path);
            JSValue JSONPValue = JSONPData[entry].m_value.get();
            if (JSONPPath.size() == 1 && JSONPPath[0].m_type == JSONPPathEntryTypeDeclareVar) {
                globalObject->addVar(globalObject, JSONPPath[0].m_pathEntryName);
                RETURN_IF_EXCEPTION(throwScope, { });
                PutPropertySlot slot(globalObject);
                globalObject->methodTable(vm)->put(globalObject, globalObject, JSONPPath[0].m_pathEntryName, JSONPValue, slot);
                RETURN_IF_EXCEPTION(throwScope, { });
                result = jsUndefined();
                continue;
            }
            JSValue baseObject(globalObject);
            for (unsigned i = 0; i < JSONPPath.size() - 1; i++) {
                ASSERT(JSONPPath[i].m_type != JSONPPathEntryTypeDeclareVar);
                switch (JSONPPath[i].m_type) {
                case JSONPPathEntryTypeDot: {
                    if (i == 0) {
                        RELEASE_ASSERT(baseObject == globalObject);

                        auto doGet = [&] (JSSegmentedVariableObject* scope) {
                            PropertySlot slot(scope, PropertySlot::InternalMethodType::Get);
                            if (scope->getPropertySlot(globalObject, JSONPPath[i].m_pathEntryName, slot))
                                return slot.getValue(globalObject, JSONPPath[i].m_pathEntryName);
                            return JSValue();
                        };

                        JSValue result = doGet(globalObject->globalLexicalEnvironment());
                        RETURN_IF_EXCEPTION(throwScope, JSValue());
                        if (result) {
                            baseObject = result;
                            continue;
                        }

                        result = doGet(globalObject);
                        RETURN_IF_EXCEPTION(throwScope, JSValue());
                        if (result) {
                            baseObject = result;
                            continue;
                        }

                        if (entry)
                            return throwException(globalObject, throwScope, createUndefinedVariableError(globalObject, JSONPPath[i].m_pathEntryName));
                        goto failedJSONP;
                    }

                    baseObject = baseObject.get(globalObject, JSONPPath[i].m_pathEntryName);
                    RETURN_IF_EXCEPTION(throwScope, JSValue());
                    continue;
                }
                case JSONPPathEntryTypeLookup: {
                    baseObject = baseObject.get(globalObject, static_cast<unsigned>(JSONPPath[i].m_pathIndex));
                    RETURN_IF_EXCEPTION(throwScope, JSValue());
                    continue;
                }
                default:
                    RELEASE_ASSERT_NOT_REACHED();
                    return jsUndefined();
                }
            }

            if (JSONPPath.size() == 1 && JSONPPath.last().m_type != JSONPPathEntryTypeLookup) {
                RELEASE_ASSERT(baseObject == globalObject);
                JSGlobalLexicalEnvironment* scope = globalObject->globalLexicalEnvironment();
                if (scope->hasProperty(globalObject, JSONPPath.last().m_pathEntryName))
                    baseObject = scope;
                RETURN_IF_EXCEPTION(throwScope, JSValue());
            }

            PutPropertySlot slot(baseObject);
            switch (JSONPPath.last().m_type) {
            case JSONPPathEntryTypeCall: {
                JSValue function = baseObject.get(globalObject, JSONPPath.last().m_pathEntryName);
                RETURN_IF_EXCEPTION(throwScope, JSValue());
                auto callData = getCallData(vm, function);
                if (callData.type == CallData::Type::None)
                    return throwException(globalObject, throwScope, createNotAFunctionError(globalObject, function));
                MarkedArgumentBuffer jsonArg;
                jsonArg.append(JSONPValue);
                ASSERT(!jsonArg.hasOverflowed());
                JSValue thisValue = JSONPPath.size() == 1 ? jsUndefined() : baseObject;
                JSONPValue = JSC::call(globalObject, function, callData, thisValue, jsonArg);
                RETURN_IF_EXCEPTION(throwScope, JSValue());
                break;
            }
            case JSONPPathEntryTypeDot: {
                baseObject.put(globalObject, JSONPPath.last().m_pathEntryName, JSONPValue, slot);
                RETURN_IF_EXCEPTION(throwScope, JSValue());
                break;
            }
            case JSONPPathEntryTypeLookup: {
                baseObject.putByIndex(globalObject, JSONPPath.last().m_pathIndex, JSONPValue, slot.isStrictMode());
                RETURN_IF_EXCEPTION(throwScope, JSValue());
                break;
            }
            default:
                RELEASE_ASSERT_NOT_REACHED();
                return jsUndefined();
            }
            result = JSONPValue;
        }
        return result;
    }
failedJSONP:
    // If we get here, then we have already proven that the script is not a JSON
    // object.

    // Compile source to bytecode if necessary:
    JSObject* error = program->initializeGlobalProperties(vm, globalObject, scope);
    EXCEPTION_ASSERT(!throwScope.exception() || !error || vm.hasPendingTerminationException());
    RETURN_IF_EXCEPTION(throwScope, checkedReturn(throwScope.exception()));
    if (UNLIKELY(error))
        return checkedReturn(throwException(globalObject, throwScope, error));

    if (UNLIKELY(vm.traps().needHandling(VMTraps::NonDebuggerAsyncEvents))) {
        if (vm.hasExceptionsAfterHandlingTraps())
            return throwScope.exception();
    }

    if (scope->structure(vm)->isUncacheableDictionary())
        scope->flattenDictionaryObject(vm);

    RefPtr<JITCode> jitCode;
    ProtoCallFrame protoCallFrame;
    {
        DeferTraps deferTraps(vm); // We can't jettison this code if we're about to run it.

        ProgramCodeBlock* codeBlock;
        {
            CodeBlock* tempCodeBlock;
            program->prepareForExecution<ProgramExecutable>(vm, nullptr, scope, CodeForCall, tempCodeBlock);
            RETURN_IF_EXCEPTION(throwScope, checkedReturn(throwScope.exception()));

            codeBlock = jsCast<ProgramCodeBlock*>(tempCodeBlock);
            ASSERT(codeBlock && codeBlock->numParameters() == 1); // 1 parameter for 'this'.
        }

        {
            DisallowGC disallowGC; // Ensure no GC happens. GC can replace CodeBlock in Executable.
            jitCode = program->generatedJITCode();
            protoCallFrame.init(codeBlock, globalObject, globalCallee, thisObj, 1);
        }
    }

    // Execute the code:
    throwScope.release();
    ASSERT(jitCode == program->generatedJITCode().ptr());
    JSValue result = jitCode->execute(&vm, &protoCallFrame);
    return checkedReturn(result);
}

JSValue Interpreter::executeCall(JSGlobalObject* lexicalGlobalObject, JSObject* function, const CallData& callData, JSValue thisValue, const ArgList& args)
{
    VM& vm = lexicalGlobalObject->vm();
    auto throwScope = DECLARE_THROW_SCOPE(vm);

    auto clobberizeValidator = makeScopeExit([&] {
        vm.didEnterVM = true;
    });

    throwScope.assertNoException();
    ASSERT(!vm.isCollectorBusyOnCurrentThread());
    if (vm.isCollectorBusyOnCurrentThread())
        return jsNull();

    bool isJSCall = (callData.type == CallData::Type::JS);
    JSScope* scope = nullptr;
    size_t argsCount = 1 + args.size(); // implicit "this" parameter

    JSGlobalObject* globalObject;

    if (isJSCall) {
        scope = callData.js.scope;
        globalObject = scope->globalObject(vm);
    } else {
        ASSERT(callData.type == CallData::Type::Native);
        globalObject = function->globalObject(vm);
    }

    VMEntryScope entryScope(vm, globalObject);
    if (UNLIKELY(!vm.isSafeToRecurseSoft() || args.size() > maxArguments))
        return checkedReturn(throwStackOverflowError(globalObject, throwScope));

#if PLATFORM(WKC)
    if (IS_STACK_OVERFLOW(WKC_STACK_MARGIN_DEFAULT)) {
        return checkedReturn(throwStackOverflowError(globalObject, throwScope));
    }
#endif

    if (UNLIKELY(vm.traps().needHandling(VMTraps::NonDebuggerAsyncEvents))) {
        if (vm.hasExceptionsAfterHandlingTraps())
            return throwScope.exception();
    }

    RefPtr<JITCode> jitCode;
    ProtoCallFrame protoCallFrame;
    {
        DeferTraps deferTraps(vm); // We can't jettison this code if we're about to run it.

        CodeBlock* newCodeBlock = nullptr;
        if (isJSCall) {
            // Compile the callee:
            callData.js.functionExecutable->prepareForExecution<FunctionExecutable>(vm, jsCast<JSFunction*>(function), scope, CodeForCall, newCodeBlock);
            RETURN_IF_EXCEPTION(throwScope, checkedReturn(throwScope.exception()));

            ASSERT(newCodeBlock);
            newCodeBlock->m_shouldAlwaysBeInlined = false;
        }

        {
            DisallowGC disallowGC; // Ensure no GC happens. GC can replace CodeBlock in Executable.
            if (isJSCall)
                jitCode = callData.js.functionExecutable->generatedJITCodeForCall();
            protoCallFrame.init(newCodeBlock, globalObject, function, thisValue, argsCount, args.data());
        }
    }

    JSValue result;
    // Execute the code:
    if (isJSCall) {
        throwScope.release();
        ASSERT(jitCode == callData.js.functionExecutable->generatedJITCodeForCall().ptr());
        result = jitCode->execute(&vm, &protoCallFrame);
    } else {
        result = JSValue::decode(vmEntryToNative(callData.native.function.rawPointer(), &vm, &protoCallFrame));
        RETURN_IF_EXCEPTION(throwScope, JSValue());
    }

    return checkedReturn(result);
}

JSObject* Interpreter::executeConstruct(JSGlobalObject* lexicalGlobalObject, JSObject* constructor, const CallData& constructData, const ArgList& args, JSValue newTarget)
{
    VM& vm = lexicalGlobalObject->vm();
    auto throwScope = DECLARE_THROW_SCOPE(vm);

    auto clobberizeValidator = makeScopeExit([&] {
        vm.didEnterVM = true;
    });

    throwScope.assertNoException();
    ASSERT(!vm.isCollectorBusyOnCurrentThread());
    // We throw in this case because we have to return something "valid" but we're
    // already in an invalid state.
    if (UNLIKELY(vm.isCollectorBusyOnCurrentThread())) {
        throwStackOverflowError(lexicalGlobalObject, throwScope);
        return nullptr;
    }

    bool isJSConstruct = (constructData.type == CallData::Type::JS);
    JSScope* scope = nullptr;
    size_t argsCount = 1 + args.size(); // implicit "this" parameter

    JSGlobalObject* globalObject;

    if (isJSConstruct) {
        scope = constructData.js.scope;
        globalObject = scope->globalObject(vm);
    } else {
        ASSERT(constructData.type == CallData::Type::Native);
        globalObject = constructor->globalObject(vm);
    }

    VMEntryScope entryScope(vm, globalObject);
    if (UNLIKELY(!vm.isSafeToRecurseSoft() || args.size() > maxArguments)) {
        throwStackOverflowError(globalObject, throwScope);
        return nullptr;
    }

#if PLATFORM(WKC)
    if (IS_STACK_OVERFLOW(WKC_STACK_MARGIN_DEFAULT)) {
        return asObject(checkedReturn(throwStackOverflowError(globalObject, throwScope)));
    }
#endif

    if (UNLIKELY(vm.traps().needHandling(VMTraps::NonDebuggerAsyncEvents))) {
        if (vm.hasExceptionsAfterHandlingTraps())
            return nullptr;
    }

    RefPtr<JITCode> jitCode;
    ProtoCallFrame protoCallFrame;
    {
        DeferTraps deferTraps(vm); // We can't jettison this code if we're about to run it.

        CodeBlock* newCodeBlock = nullptr;
        if (isJSConstruct) {
            // Compile the callee:
            constructData.js.functionExecutable->prepareForExecution<FunctionExecutable>(vm, jsCast<JSFunction*>(constructor), scope, CodeForConstruct, newCodeBlock);
            RETURN_IF_EXCEPTION(throwScope, nullptr);

            ASSERT(newCodeBlock);
            newCodeBlock->m_shouldAlwaysBeInlined = false;
        }

        {
            DisallowGC disallowGC; // Ensure no GC happens. GC can replace CodeBlock in Executable.
            if (isJSConstruct)
                jitCode = constructData.js.functionExecutable->generatedJITCodeForConstruct();
            protoCallFrame.init(newCodeBlock, globalObject, constructor, newTarget, argsCount, args.data());
        }
    }

    JSValue result;
    // Execute the code.
    if (isJSConstruct) {
        ASSERT(jitCode == constructData.js.functionExecutable->generatedJITCodeForConstruct().ptr());
        result = jitCode->execute(&vm, &protoCallFrame);
    } else {
        result = JSValue::decode(vmEntryToNative(constructData.native.function.rawPointer(), &vm, &protoCallFrame));

        if (LIKELY(!throwScope.exception()))
            RELEASE_ASSERT(result.isObject());
    }

    RETURN_IF_EXCEPTION(throwScope, nullptr);
    ASSERT(result.isObject());
    return checkedReturn(asObject(result));
}

CallFrameClosure Interpreter::prepareForRepeatCall(FunctionExecutable* functionExecutable, CallFrame* callFrame, ProtoCallFrame* protoCallFrame, JSFunction* function, int argumentCountIncludingThis, JSScope* scope, const ArgList& args)
{
    VM& vm = scope->vm();
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    throwScope.assertNoException();

    if (vm.isCollectorBusyOnCurrentThread())
        return { };

#if PLATFORM(WKC)
    if (IS_STACK_OVERFLOW(WKC_STACK_MARGIN_DEFAULT)) {
        throwStackOverflowError(callFrame->lexicalGlobalObject(vm), throwScope);
        return CallFrameClosure();
    }
#endif

    // Compile the callee:
    CodeBlock* newCodeBlock;
    functionExecutable->prepareForExecution<FunctionExecutable>(vm, function, scope, CodeForCall, newCodeBlock);
    RETURN_IF_EXCEPTION(throwScope, { });

    ASSERT(newCodeBlock);
    newCodeBlock->m_shouldAlwaysBeInlined = false;

    size_t argsCount = argumentCountIncludingThis;

    protoCallFrame->init(newCodeBlock, function->globalObject(), function, jsUndefined(), argsCount, args.data());
    // Return the successful closure:
    CallFrameClosure result = { callFrame, protoCallFrame, function, functionExecutable, &vm, scope, newCodeBlock->numParameters(), argumentCountIncludingThis };
    return result;
}

JSValue Interpreter::execute(EvalExecutable* eval, JSGlobalObject* lexicalGlobalObject, JSValue thisValue, JSScope* scope)
{
    VM& vm = scope->vm();
    auto throwScope = DECLARE_THROW_SCOPE(vm);

    auto clobberizeValidator = makeScopeExit([&] {
        vm.didEnterVM = true;
    });

    ASSERT_UNUSED(lexicalGlobalObject, &vm == &lexicalGlobalObject->vm());
    throwScope.assertNoException();
    ASSERT(!vm.isCollectorBusyOnCurrentThread());
    RELEASE_ASSERT(vm.currentThreadIsHoldingAPILock());
    if (vm.isCollectorBusyOnCurrentThread())
        return jsNull();

    JSGlobalObject* globalObject = scope->globalObject(vm);
    VMEntryScope entryScope(vm, globalObject);
    if (UNLIKELY(!vm.isSafeToRecurseSoft()))
        return checkedReturn(throwStackOverflowError(globalObject, throwScope));

#if PLATFORM(WKC)
    if (IS_STACK_OVERFLOW(WKC_STACK_MARGIN_DEFAULT)) {
        return checkedReturn(throwStackOverflowError(globalObject, throwScope));
    }
#endif

    unsigned numVariables = eval->numVariables();
    unsigned numTopLevelFunctionDecls = eval->numTopLevelFunctionDecls();
    unsigned numFunctionHoistingCandidates = eval->numFunctionHoistingCandidates();

    JSScope* variableObject;
    if ((numVariables || numTopLevelFunctionDecls) && eval->isInStrictContext()) {
        scope = StrictEvalActivation::create(vm, globalObject->strictEvalActivationStructure(), scope);
        variableObject = scope;
    } else {
        for (JSScope* node = scope; ; node = node->next()) {
            RELEASE_ASSERT(node);
            if (node->isGlobalObject()) {
                variableObject = node;
                break;
            } 
            if (node->isJSLexicalEnvironment()) {
                JSLexicalEnvironment* lexicalEnvironment = jsCast<JSLexicalEnvironment*>(node);
                if (lexicalEnvironment->symbolTable()->scopeType() == SymbolTable::ScopeType::VarScope) {
                    variableObject = node;
                    break;
                }
            }
        }
    }

    if (UNLIKELY(vm.traps().needHandling(VMTraps::NonDebuggerAsyncEvents))) {
        if (vm.hasExceptionsAfterHandlingTraps())
            return throwScope.exception();
    }

    EvalCodeBlock* codeBlock;
    {
        CodeBlock* tempCodeBlock;
        eval->prepareForExecution<EvalExecutable>(vm, nullptr, scope, CodeForCall, tempCodeBlock);
        RETURN_IF_EXCEPTION(throwScope, checkedReturn(throwScope.exception()));

        codeBlock = jsCast<EvalCodeBlock*>(tempCodeBlock);
        ASSERT(codeBlock && codeBlock->numParameters() == 1); // 1 parameter for 'this'.
    }
    UnlinkedEvalCodeBlock* unlinkedCodeBlock = codeBlock->unlinkedEvalCodeBlock();

    // We can't declare a "var"/"function" that overwrites a global "let"/"const"/"class" in a sloppy-mode eval.
    if (variableObject->isGlobalObject() && !eval->isInStrictContext() && (numVariables || numTopLevelFunctionDecls)) {
        JSGlobalLexicalEnvironment* globalLexicalEnvironment = jsCast<JSGlobalObject*>(variableObject)->globalLexicalEnvironment();
        for (unsigned i = 0; i < numVariables; ++i) {
            const Identifier& ident = unlinkedCodeBlock->variable(i);
            PropertySlot slot(globalLexicalEnvironment, PropertySlot::InternalMethodType::VMInquiry, &vm);
            if (JSGlobalLexicalEnvironment::getOwnPropertySlot(globalLexicalEnvironment, globalObject, ident, slot)) {
                return checkedReturn(throwTypeError(globalObject, throwScope, makeString("Can't create duplicate global variable in eval: '", String(ident.impl()), "'")));
            }
        }

        for (unsigned i = 0; i < numTopLevelFunctionDecls; ++i) {
            FunctionExecutable* function = codeBlock->functionDecl(i);
            PropertySlot slot(globalLexicalEnvironment, PropertySlot::InternalMethodType::VMInquiry, &vm);
            if (JSGlobalLexicalEnvironment::getOwnPropertySlot(globalLexicalEnvironment, globalObject, function->name(), slot)) {
                return checkedReturn(throwTypeError(globalObject, throwScope, makeString("Can't create duplicate global variable in eval: '", String(function->name().impl()), "'")));
            }
        }
    }

    if (variableObject->structure(vm)->isUncacheableDictionary())
        variableObject->flattenDictionaryObject(vm);

    if (numVariables || numTopLevelFunctionDecls || numFunctionHoistingCandidates) {
        BatchedTransitionOptimizer optimizer(vm, variableObject);
        if (variableObject->next() && !eval->isInStrictContext())
            variableObject->globalObject(vm)->varInjectionWatchpoint()->fireAll(vm, "Executed eval, fired VarInjection watchpoint");

        for (unsigned i = 0; i < numVariables; ++i) {
            const Identifier& ident = unlinkedCodeBlock->variable(i);
            bool hasProperty = variableObject->hasProperty(globalObject, ident);
            RETURN_IF_EXCEPTION(throwScope, checkedReturn(throwScope.exception()));
            if (!hasProperty) {
                PutPropertySlot slot(variableObject);
                if (!variableObject->isExtensible(globalObject))
                    return checkedReturn(throwTypeError(globalObject, throwScope, NonExtensibleObjectPropertyDefineError));
                variableObject->methodTable(vm)->put(variableObject, globalObject, ident, jsUndefined(), slot);
                RETURN_IF_EXCEPTION(throwScope, checkedReturn(throwScope.exception()));
            }
        }
        
        if (eval->isInStrictContext()) {
            for (unsigned i = 0; i < numTopLevelFunctionDecls; ++i) {
                FunctionExecutable* function = codeBlock->functionDecl(i);
                PutPropertySlot slot(variableObject);
                // We need create this variables because it will be used to emits code by bytecode generator
                variableObject->methodTable(vm)->put(variableObject, globalObject, function->name(), jsUndefined(), slot);
                RETURN_IF_EXCEPTION(throwScope, checkedReturn(throwScope.exception()));
            }
        } else {
            for (unsigned i = 0; i < numTopLevelFunctionDecls; ++i) {
                FunctionExecutable* function = codeBlock->functionDecl(i);
                JSValue resolvedScope = JSScope::resolveScopeForHoistingFuncDeclInEval(globalObject, scope, function->name());
                RETURN_IF_EXCEPTION(throwScope, checkedReturn(throwScope.exception()));
                if (resolvedScope.isUndefined())
                    return checkedReturn(throwSyntaxError(globalObject, throwScope, makeString("Can't create duplicate variable in eval: '", String(function->name().impl()), "'")));
                PutPropertySlot slot(variableObject);
                // We need create this variables because it will be used to emits code by bytecode generator
                variableObject->methodTable(vm)->put(variableObject, globalObject, function->name(), jsUndefined(), slot);
                RETURN_IF_EXCEPTION(throwScope, checkedReturn(throwScope.exception()));
            }

            for (unsigned i = 0; i < numFunctionHoistingCandidates; ++i) {
                const Identifier& ident = unlinkedCodeBlock->functionHoistingCandidate(i);
                JSValue resolvedScope = JSScope::resolveScopeForHoistingFuncDeclInEval(globalObject, scope, ident);
                RETURN_IF_EXCEPTION(throwScope, checkedReturn(throwScope.exception()));
                if (!resolvedScope.isUndefined()) {
                    bool hasProperty = variableObject->hasProperty(globalObject, ident);
                    RETURN_IF_EXCEPTION(throwScope, checkedReturn(throwScope.exception()));
                    if (!hasProperty) {
                        PutPropertySlot slot(variableObject);
                        variableObject->methodTable(vm)->put(variableObject, globalObject, ident, jsUndefined(), slot);
                        RETURN_IF_EXCEPTION(throwScope, checkedReturn(throwScope.exception()));
                    }
                }
            }
        }
    }

    JSCallee* callee = nullptr;
    if (scope == globalObject->globalScope())
        callee = globalObject->globalCallee();
    else
        callee = JSCallee::create(vm, globalObject, scope);

    RefPtr<JITCode> jitCode;
    ProtoCallFrame protoCallFrame;
    {
        DeferTraps deferTraps(vm); // We can't jettison this code if we're about to run it.

        // Reload CodeBlock. It is possible that we replaced CodeBlock while setting up the environment.
        {
            CodeBlock* tempCodeBlock;
            eval->prepareForExecution<EvalExecutable>(vm, nullptr, scope, CodeForCall, tempCodeBlock);
            RETURN_IF_EXCEPTION(throwScope, checkedReturn(throwScope.exception()));

            codeBlock = jsCast<EvalCodeBlock*>(tempCodeBlock);
            ASSERT(codeBlock && codeBlock->numParameters() == 1); // 1 parameter for 'this'.
        }

        {
            DisallowGC disallowGC; // Ensure no GC happens. GC can replace CodeBlock in Executable.
            jitCode = eval->generatedJITCode();
            protoCallFrame.init(codeBlock, globalObject, callee, thisValue, 1);
        }
    }

    // Execute the code:
    throwScope.release();
    ASSERT(jitCode == eval->generatedJITCode().ptr());
    JSValue result = jitCode->execute(&vm, &protoCallFrame);

    return checkedReturn(result);
}

JSValue Interpreter::executeModuleProgram(JSModuleRecord* record, ModuleProgramExecutable* executable, JSGlobalObject* lexicalGlobalObject, JSModuleEnvironment* scope, JSValue sentValue, JSValue resumeMode)
{
    VM& vm = scope->vm();
    auto throwScope = DECLARE_THROW_SCOPE(vm);

    auto clobberizeValidator = makeScopeExit([&] {
        vm.didEnterVM = true;
    });

    ASSERT_UNUSED(lexicalGlobalObject, &vm == &lexicalGlobalObject->vm());
    throwScope.assertNoException();
    ASSERT(!vm.isCollectorBusyOnCurrentThread());
    RELEASE_ASSERT(vm.currentThreadIsHoldingAPILock());
    if (vm.isCollectorBusyOnCurrentThread())
        return jsNull();

    JSGlobalObject* globalObject = scope->globalObject(vm);
    VMEntryScope entryScope(vm, scope->globalObject(vm));
    if (UNLIKELY(!vm.isSafeToRecurseSoft()))
        return checkedReturn(throwStackOverflowError(globalObject, throwScope));

#if PLATFORM(WKC)
    if (IS_STACK_OVERFLOW(WKC_STACK_MARGIN_DEFAULT)) {
        return checkedReturn(throwStackOverflowError(globalObject, throwScope));
    }
#endif

    if (UNLIKELY(vm.traps().needHandling(VMTraps::NonDebuggerAsyncEvents))) {
        if (vm.hasExceptionsAfterHandlingTraps())
            return throwScope.exception();
    }

    if (scope->structure(vm)->isUncacheableDictionary())
        scope->flattenDictionaryObject(vm);

    const unsigned numberOfArguments = static_cast<unsigned>(AbstractModuleRecord::Argument::NumberOfArguments);
    JSCallee* callee = JSCallee::create(vm, globalObject, scope);
    RefPtr<JITCode> jitCode;

    ProtoCallFrame protoCallFrame;
    JSValue args[numberOfArguments] = {
        record,
        record->internalField(JSModuleRecord::Field::State).get(),
        sentValue,
        resumeMode,
        scope,
    };

    {
        DeferTraps deferTraps(vm); // We can't jettison this code if we're about to run it.

        ModuleProgramCodeBlock* codeBlock;
        {
            CodeBlock* tempCodeBlock;
            executable->prepareForExecution<ModuleProgramExecutable>(vm, nullptr, scope, CodeForCall, tempCodeBlock);
            RETURN_IF_EXCEPTION(throwScope, checkedReturn(throwScope.exception()));

            codeBlock = jsCast<ModuleProgramCodeBlock*>(tempCodeBlock);
            ASSERT(codeBlock && codeBlock->numParameters() == numberOfArguments + 1);
        }


        {
            DisallowGC disallowGC; // Ensure no GC happens. GC can replace CodeBlock in Executable.
            jitCode = executable->generatedJITCode();

            // The |this| of the module is always `undefined`.
            // http://www.ecma-international.org/ecma-262/6.0/#sec-module-environment-records-hasthisbinding
            // http://www.ecma-international.org/ecma-262/6.0/#sec-module-environment-records-getthisbinding
            protoCallFrame.init(codeBlock, globalObject, callee, jsUndefined(), numberOfArguments + 1, args);
        }

        record->internalField(JSModuleRecord::Field::State).set(vm, record, jsNumber(static_cast<int>(JSModuleRecord::State::Executing)));
    }

    // Execute the code:
    throwScope.release();
    ASSERT(jitCode == executable->generatedJITCode().ptr());
    JSValue result = jitCode->execute(&vm, &protoCallFrame);

    return checkedReturn(result);
}

NEVER_INLINE void Interpreter::debug(CallFrame* callFrame, DebugHookType debugHookType)
{
    VM& vm = callFrame->deprecatedVM();
    DeferTermination deferScope(vm);
    auto scope = DECLARE_CATCH_SCOPE(vm);

    if (UNLIKELY(Options::debuggerTriggersBreakpointException()) && debugHookType == DidReachDebuggerStatement)
        WTFBreakpointTrap();

    Debugger* debugger = callFrame->lexicalGlobalObject(vm)->debugger();
    if (!debugger)
        return;

    ASSERT(callFrame->codeBlock()->hasDebuggerRequests());
    scope.assertNoException();

    switch (debugHookType) {
        case DidEnterCallFrame:
            debugger->callEvent(callFrame);
            break;
        case WillLeaveCallFrame:
            debugger->returnEvent(callFrame);
            break;
        case WillExecuteStatement:
            debugger->atStatement(callFrame);
            break;
        case WillExecuteExpression:
            debugger->atExpression(callFrame);
            break;
        case WillExecuteProgram:
            debugger->willExecuteProgram(callFrame);
            break;
        case DidExecuteProgram:
            debugger->didExecuteProgram(callFrame);
            break;
        case DidReachDebuggerStatement:
            debugger->didReachDebuggerStatement(callFrame);
            break;
    }
    scope.assertNoException();
}

} // namespace JSC

namespace WTF {

void printInternal(PrintStream& out, JSC::DebugHookType type)
{
    switch (type) {
    case JSC::WillExecuteProgram:
        out.print("WillExecuteProgram");
        return;
    case JSC::DidExecuteProgram:
        out.print("DidExecuteProgram");
        return;
    case JSC::DidEnterCallFrame:
        out.print("DidEnterCallFrame");
        return;
    case JSC::DidReachDebuggerStatement:
        out.print("DidReachDebuggerStatement");
        return;
    case JSC::WillLeaveCallFrame:
        out.print("WillLeaveCallFrame");
        return;
    case JSC::WillExecuteStatement:
        out.print("WillExecuteStatement");
        return;
    case JSC::WillExecuteExpression:
        out.print("WillExecuteExpression");
        return;
    }
}

} // namespace WTF
