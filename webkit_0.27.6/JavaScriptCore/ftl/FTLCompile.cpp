/*
 * Copyright (C) 2013-2015 Apple Inc. All rights reserved.
 * Copyright (C) 2014 Samsung Electronics
 * Copyright (C) 2014 University of Szeged
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "FTLCompile.h"

#if ENABLE(FTL_JIT)

#include "CodeBlockWithJITType.h"
#include "CCallHelpers.h"
#include "DFGCommon.h"
#include "DFGGraphSafepoint.h"
#include "DataView.h"
#include "Disassembler.h"
#include "FTLExitThunkGenerator.h"
#include "FTLInlineCacheSize.h"
#include "FTLJITCode.h"
#include "FTLThunks.h"
#include "FTLUnwindInfo.h"
#include "JITStubs.h"
#include "LLVMAPI.h"
#include "LinkBuffer.h"
#include "RepatchBuffer.h"

namespace JSC { namespace FTL {

using namespace DFG;

static uint8_t* mmAllocateCodeSection(
    void* opaqueState, uintptr_t size, unsigned alignment, unsigned, const char* sectionName)
{
    State& state = *static_cast<State*>(opaqueState);
    
    RELEASE_ASSERT(alignment <= jitAllocationGranule);
    
    RefPtr<ExecutableMemoryHandle> result =
        state.graph.m_vm.executableAllocator.allocate(
            state.graph.m_vm, size, state.graph.m_codeBlock, JITCompilationCanFail);
    
    if (!result) {
        // Signal failure. This compilation will get tossed.
        state.allocationFailed = true;
        
        // Fake an allocation, since LLVM cannot handle failures in the memory manager.
        RefPtr<DataSection> fakeSection = adoptRef(new DataSection(size, jitAllocationGranule));
        state.jitCode->addDataSection(fakeSection);
        return bitwise_cast<uint8_t*>(fakeSection->base());
    }
    
    // LLVM used to put __compact_unwind in a code section. We keep this here defensively,
    // for clients that use older LLVMs.
    if (!strcmp(sectionName, SECTION_NAME("compact_unwind"))) {
        state.unwindDataSection = result->start();
        state.unwindDataSectionSize = result->sizeInBytes();
    }
    
    state.jitCode->addHandle(result);
    state.codeSectionNames.append(sectionName);
    
    return static_cast<uint8_t*>(result->start());
}

static uint8_t* mmAllocateDataSection(
    void* opaqueState, uintptr_t size, unsigned alignment, unsigned sectionID,
    const char* sectionName, LLVMBool isReadOnly)
{
    UNUSED_PARAM(sectionID);
    UNUSED_PARAM(isReadOnly);

    // Allocate the GOT in the code section to make it reachable for all code.
    if (!strcmp(sectionName, SECTION_NAME("got")))
        return mmAllocateCodeSection(opaqueState, size, alignment, sectionID, sectionName);

    State& state = *static_cast<State*>(opaqueState);

    RefPtr<DataSection> section = adoptRef(new DataSection(size, alignment));

    if (!strcmp(sectionName, SECTION_NAME("llvm_stackmaps")))
        state.stackmapsSection = section;
    else {
        state.jitCode->addDataSection(section);
        state.dataSectionNames.append(sectionName);
#if OS(DARWIN)
        if (!strcmp(sectionName, SECTION_NAME("compact_unwind"))) {
#elif OS(LINUX)
        if (!strcmp(sectionName, SECTION_NAME("eh_frame"))) {
#else
#error "Unrecognized OS"
#endif
            state.unwindDataSection = section->base();
            state.unwindDataSectionSize = size;
        }
    }

    return bitwise_cast<uint8_t*>(section->base());
}

static LLVMBool mmApplyPermissions(void*, char**)
{
    return false;
}

static void mmDestroy(void*)
{
}

static void dumpDataSection(DataSection* section, const char* prefix)
{
    for (unsigned j = 0; j < section->size() / sizeof(int64_t); ++j) {
        char buf[32];
        int64_t* wordPointer = static_cast<int64_t*>(section->base()) + j;
        snprintf(buf, sizeof(buf), "0x%lx", static_cast<unsigned long>(bitwise_cast<uintptr_t>(wordPointer)));
        dataLogF("%s%16s: 0x%016llx\n", prefix, buf, static_cast<long long>(*wordPointer));
    }
}

static int offsetOfStackRegion(StackMaps::RecordMap& recordMap, uint32_t stackmapID)
{
    if (stackmapID == UINT_MAX)
        return 0;
    
    StackMaps::RecordMap::iterator iter = recordMap.find(stackmapID);
    RELEASE_ASSERT(iter != recordMap.end());
    RELEASE_ASSERT(iter->value.size() == 1);
    RELEASE_ASSERT(iter->value[0].locations.size() == 1);
    Location capturedLocation =
        Location::forStackmaps(nullptr, iter->value[0].locations[0]);
    RELEASE_ASSERT(capturedLocation.kind() == Location::Register);
    RELEASE_ASSERT(capturedLocation.gpr() == GPRInfo::callFrameRegister);
    RELEASE_ASSERT(!(capturedLocation.addend() % sizeof(Register)));
    return capturedLocation.addend() / sizeof(Register);
}

#if PLATFORM(WKC)
static void generateInlineIfPossibleOutOfLineIfNot(State& state, VM& vm, CodeBlock* codeBlock, CCallHelpers& code, char* startOfInlineCode, size_t sizeOfInlineCode, const char* codeDescription, const WTF::Function<void(LinkBuffer&, CCallHelpers&, bool wasCompiledInline)>& callback)
#else
static void generateInlineIfPossibleOutOfLineIfNot(State& state, VM& vm, CodeBlock* codeBlock, CCallHelpers& code, char* startOfInlineCode, size_t sizeOfInlineCode, const char* codeDescription, const std::function<void(LinkBuffer&, CCallHelpers&, bool wasCompiledInline)>& callback)
#endif
{
    std::unique_ptr<LinkBuffer> codeLinkBuffer;
    size_t actualCodeSize = code.m_assembler.buffer().codeSize();

    if (actualCodeSize <= sizeOfInlineCode) {
        LinkBuffer codeLinkBuffer(vm, code, startOfInlineCode, sizeOfInlineCode);

        // Fill the remainder of the inline space with nops to avoid confusing the disassembler.
        MacroAssembler::AssemblerType_T::fillNops(bitwise_cast<char*>(startOfInlineCode) + actualCodeSize, sizeOfInlineCode - actualCodeSize);

        callback(codeLinkBuffer, code, true);

        return;
    }

    // If there isn't enough space in the provided inline code area, allocate out of line
    // executable memory to link the provided code. Place a jump at the beginning of the
    // inline area and jump to the out of line code. Similarly return by appending a jump
    // to the provided code that goes to the instruction after the inline code.
    // Fill the middle with nop's.
    MacroAssembler::Jump returnToMainline = code.jump();

    // Allocate out of line executable memory and link the provided code there.
    codeLinkBuffer = std::make_unique<LinkBuffer>(vm, code, codeBlock, JITCompilationMustSucceed);

    // Plant a jmp in the inline buffer to the out of line code.
    MacroAssembler callToOutOfLineCode;
    MacroAssembler::Jump jumpToOutOfLine = callToOutOfLineCode.jump();
    LinkBuffer inlineBuffer(vm, callToOutOfLineCode, startOfInlineCode, sizeOfInlineCode);
    inlineBuffer.link(jumpToOutOfLine, codeLinkBuffer->entrypoint());

    // Fill the remainder of the inline space with nops to avoid confusing the disassembler.
    MacroAssembler::AssemblerType_T::fillNops(bitwise_cast<char*>(startOfInlineCode) + inlineBuffer.size(), sizeOfInlineCode - inlineBuffer.size());

    // Link the end of the out of line code to right after the inline area.
    codeLinkBuffer->link(returnToMainline, CodeLocationLabel(MacroAssemblerCodePtr::createFromExecutableAddress(startOfInlineCode)).labelAtOffset(sizeOfInlineCode));

    callback(*codeLinkBuffer.get(), code, false);

    state.finalizer->outOfLineCodeInfos.append(OutOfLineCodeInfo(WTF::move(codeLinkBuffer), codeDescription));
}

template<typename DescriptorType>
void generateICFastPath(
    State& state, CodeBlock* codeBlock, GeneratedFunction generatedFunction,
    StackMaps::RecordMap& recordMap, DescriptorType& ic, size_t sizeOfIC)
{
    VM& vm = state.graph.m_vm;

    StackMaps::RecordMap::iterator iter = recordMap.find(ic.stackmapID());
    if (iter == recordMap.end()) {
        // It was optimized out.
        return;
    }
    
    Vector<StackMaps::Record>& records = iter->value;
    
    RELEASE_ASSERT(records.size() == ic.m_generators.size());
    
    for (unsigned i = records.size(); i--;) {
        StackMaps::Record& record = records[i];
        auto generator = ic.m_generators[i];

        CCallHelpers fastPathJIT(&vm, codeBlock);
        generator.generateFastPath(fastPathJIT);
        
        char* startOfIC =
            bitwise_cast<char*>(generatedFunction) + record.instructionOffset;

        generateInlineIfPossibleOutOfLineIfNot(state, vm, codeBlock, fastPathJIT, startOfIC, sizeOfIC, "inline cache fast path", [&] (LinkBuffer& linkBuffer, CCallHelpers&, bool) {
            state.finalizer->sideCodeLinkBuffer->link(ic.m_slowPathDone[i],
                CodeLocationLabel(startOfIC + sizeOfIC));

            linkBuffer.link(generator.slowPathJump(),
                state.finalizer->sideCodeLinkBuffer->locationOf(generator.slowPathBegin()));

            generator.finalize(linkBuffer, *state.finalizer->sideCodeLinkBuffer);
        });
    }
}

static void generateCheckInICFastPath(
    State& state, CodeBlock* codeBlock, GeneratedFunction generatedFunction,
    StackMaps::RecordMap& recordMap, CheckInDescriptor& ic, size_t sizeOfIC)
{
    VM& vm = state.graph.m_vm;

    StackMaps::RecordMap::iterator iter = recordMap.find(ic.stackmapID());
    if (iter == recordMap.end()) {
        // It was optimized out.
        return;
    }
    
    Vector<StackMaps::Record>& records = iter->value;
    
    RELEASE_ASSERT(records.size() == ic.m_generators.size());

    for (unsigned i = records.size(); i--;) {
        StackMaps::Record& record = records[i];
        auto generator = ic.m_generators[i];

        StructureStubInfo& stubInfo = *generator.m_stub;
        auto call = generator.m_slowCall;
        auto slowPathBegin = generator.m_beginLabel;

        CCallHelpers fastPathJIT(&vm, codeBlock);
        
        auto jump = fastPathJIT.patchableJump();
        auto done = fastPathJIT.label();

        char* startOfIC =
            bitwise_cast<char*>(generatedFunction) + record.instructionOffset;

        auto postLink = [&] (LinkBuffer& fastPath, CCallHelpers&, bool) {
            LinkBuffer& slowPath = *state.finalizer->sideCodeLinkBuffer;

            state.finalizer->sideCodeLinkBuffer->link(
                ic.m_slowPathDone[i], CodeLocationLabel(startOfIC + sizeOfIC));

            CodeLocationLabel slowPathBeginLoc = slowPath.locationOf(slowPathBegin);
            fastPath.link(jump, slowPathBeginLoc);

            CodeLocationCall callReturnLocation = slowPath.locationOf(call);

            stubInfo.patch.deltaCallToDone = MacroAssembler::differenceBetweenCodePtr(
                callReturnLocation, fastPath.locationOf(done));

            stubInfo.patch.deltaCallToJump = MacroAssembler::differenceBetweenCodePtr(
                callReturnLocation, fastPath.locationOf(jump));
            stubInfo.callReturnLocation = callReturnLocation;
            stubInfo.patch.deltaCallToSlowCase = MacroAssembler::differenceBetweenCodePtr(
                callReturnLocation, slowPathBeginLoc);
        };

        generateInlineIfPossibleOutOfLineIfNot(state, vm, codeBlock, fastPathJIT, startOfIC, sizeOfIC, "CheckIn inline cache", postLink);
    }
}


static RegisterSet usedRegistersFor(const StackMaps::Record& record)
{
    if (Options::assumeAllRegsInFTLICAreLive())
        return RegisterSet::allRegisters();
    return RegisterSet(record.usedRegisterSet(), RegisterSet::calleeSaveRegisters());
}

template<typename CallType>
void adjustCallICsForStackmaps(Vector<CallType>& calls, StackMaps::RecordMap& recordMap)
{
    // Handling JS calls is weird: we need to ensure that we sort them by the PC in LLVM
    // generated code. That implies first pruning the ones that LLVM didn't generate.

    Vector<CallType> oldCalls;
    oldCalls.swap(calls);
    
    for (unsigned i = 0; i < oldCalls.size(); ++i) {
        CallType& call = oldCalls[i];
        
        StackMaps::RecordMap::iterator iter = recordMap.find(call.stackmapID());
        if (iter == recordMap.end())
            continue;
        
        for (unsigned j = 0; j < iter->value.size(); ++j) {
            CallType copy = call;
            copy.m_instructionOffset = iter->value[j].instructionOffset;
            calls.append(copy);
        }
    }

    std::sort(calls.begin(), calls.end());
}

static void fixFunctionBasedOnStackMaps(
    State& state, CodeBlock* codeBlock, JITCode* jitCode, GeneratedFunction generatedFunction,
    StackMaps::RecordMap& recordMap)
{
    Graph& graph = state.graph;
    VM& vm = graph.m_vm;
    StackMaps stackmaps = jitCode->stackmaps;
    
    int localsOffset = offsetOfStackRegion(recordMap, state.capturedStackmapID) + graph.m_nextMachineLocal;
    int varargsSpillSlotsOffset = offsetOfStackRegion(recordMap, state.varargsSpillSlotsStackmapID);
    
    for (unsigned i = graph.m_inlineVariableData.size(); i--;) {
        InlineCallFrame* inlineCallFrame = graph.m_inlineVariableData[i].inlineCallFrame;
        
        if (inlineCallFrame->argumentCountRegister.isValid())
            inlineCallFrame->argumentCountRegister += localsOffset;
        
        for (unsigned argument = inlineCallFrame->arguments.size(); argument-- > 1;) {
            inlineCallFrame->arguments[argument] =
                inlineCallFrame->arguments[argument].withLocalsOffset(localsOffset);
        }
        
        if (inlineCallFrame->isClosureCall) {
            inlineCallFrame->calleeRecovery =
                inlineCallFrame->calleeRecovery.withLocalsOffset(localsOffset);
        }

        if (graph.hasDebuggerEnabled())
            codeBlock->setScopeRegister(codeBlock->scopeRegister() + localsOffset);
    }
    
    MacroAssembler::Label stackOverflowException;

    {
        CCallHelpers checkJIT(&vm, codeBlock);
        
        // At this point it's perfectly fair to just blow away all state and restore the
        // JS JIT view of the universe.
        checkJIT.copyCalleeSavesToVMCalleeSavesBuffer();
        checkJIT.move(MacroAssembler::TrustedImmPtr(&vm), GPRInfo::argumentGPR0);
        checkJIT.move(GPRInfo::callFrameRegister, GPRInfo::argumentGPR1);
        MacroAssembler::Call callLookupExceptionHandler = checkJIT.call();
        checkJIT.jumpToExceptionHandler();

        stackOverflowException = checkJIT.label();
        checkJIT.copyCalleeSavesToVMCalleeSavesBuffer();
        checkJIT.move(MacroAssembler::TrustedImmPtr(&vm), GPRInfo::argumentGPR0);
        checkJIT.move(GPRInfo::callFrameRegister, GPRInfo::argumentGPR1);
        MacroAssembler::Call callLookupExceptionHandlerFromCallerFrame = checkJIT.call();
        checkJIT.jumpToExceptionHandler();

        auto linkBuffer = std::make_unique<LinkBuffer>(
            vm, checkJIT, codeBlock, JITCompilationCanFail);
        if (linkBuffer->didFailToAllocate()) {
            state.allocationFailed = true;
            return;
        }
        linkBuffer->link(callLookupExceptionHandler, FunctionPtr(lookupExceptionHandler));
        linkBuffer->link(callLookupExceptionHandlerFromCallerFrame, FunctionPtr(lookupExceptionHandlerFromCallerFrame));

        state.finalizer->handleExceptionsLinkBuffer = WTF::move(linkBuffer);
    }

    ExitThunkGenerator exitThunkGenerator(state);
    exitThunkGenerator.emitThunks();
    if (exitThunkGenerator.didThings()) {
        RELEASE_ASSERT(state.finalizer->osrExit.size());
        
        auto linkBuffer = std::make_unique<LinkBuffer>(
            vm, exitThunkGenerator, codeBlock, JITCompilationCanFail);
        if (linkBuffer->didFailToAllocate()) {
            state.allocationFailed = true;
            return;
        }
        
        RELEASE_ASSERT(state.finalizer->osrExit.size() == state.jitCode->osrExit.size());
        
        for (unsigned i = 0; i < state.jitCode->osrExit.size(); ++i) {
            OSRExitCompilationInfo& info = state.finalizer->osrExit[i];
            OSRExit& exit = jitCode->osrExit[i];
            
            if (verboseCompilationEnabled())
                dataLog("Handling OSR stackmap #", exit.m_stackmapID, " for ", exit.m_codeOrigin, "\n");

            auto iter = recordMap.find(exit.m_stackmapID);
            if (iter == recordMap.end()) {
                // It was optimized out.
                continue;
            }
            
            info.m_thunkAddress = linkBuffer->locationOf(info.m_thunkLabel);
            exit.m_patchableCodeOffset = linkBuffer->offsetOf(info.m_thunkJump);
            
            for (unsigned j = exit.m_values.size(); j--;)
                exit.m_values[j] = exit.m_values[j].withLocalsOffset(localsOffset);
            for (ExitTimeObjectMaterialization* materialization : exit.m_materializations)
                materialization->accountForLocalsOffset(localsOffset);
            
            if (verboseCompilationEnabled()) {
                DumpContext context;
                dataLog("    Exit values: ", inContext(exit.m_values, &context), "\n");
                if (!exit.m_materializations.isEmpty()) {
                    dataLog("    Materializations: \n");
                    for (ExitTimeObjectMaterialization* materialization : exit.m_materializations)
                        dataLog("        Materialize(", pointerDump(materialization), ")\n");
                }
            }
        }
        
        state.finalizer->exitThunksLinkBuffer = WTF::move(linkBuffer);
    }

    if (!state.getByIds.isEmpty() || !state.putByIds.isEmpty() || !state.checkIns.isEmpty()) {
        CCallHelpers slowPathJIT(&vm, codeBlock);
        
        CCallHelpers::JumpList exceptionTarget;
        
        for (unsigned i = state.getByIds.size(); i--;) {
            GetByIdDescriptor& getById = state.getByIds[i];
            
            if (verboseCompilationEnabled())
                dataLog("Handling GetById stackmap #", getById.stackmapID(), "\n");
            
            auto iter = recordMap.find(getById.stackmapID());
            if (iter == recordMap.end()) {
                // It was optimized out.
                continue;
            }
            
            for (unsigned i = 0; i < iter->value.size(); ++i) {
                StackMaps::Record& record = iter->value[i];
            
                RegisterSet usedRegisters = usedRegistersFor(record);
                
                GPRReg result = record.locations[0].directGPR();
                GPRReg base = record.locations[1].directGPR();
                
                JITGetByIdGenerator gen(
                    codeBlock, getById.codeOrigin(), usedRegisters, JSValueRegs(base),
                    JSValueRegs(result), NeedToSpill);
                
                MacroAssembler::Label begin = slowPathJIT.label();

                MacroAssembler::Call call = callOperation(
                    state, usedRegisters, slowPathJIT, getById.codeOrigin(), &exceptionTarget,
                    operationGetByIdOptimize, result, gen.stubInfo(), base, getById.uid());

                gen.reportSlowPathCall(begin, call);

                getById.m_slowPathDone.append(slowPathJIT.jump());
                getById.m_generators.append(gen);
            }
        }
        
        for (unsigned i = state.putByIds.size(); i--;) {
            PutByIdDescriptor& putById = state.putByIds[i];
            
            if (verboseCompilationEnabled())
                dataLog("Handling PutById stackmap #", putById.stackmapID(), "\n");
            
            auto iter = recordMap.find(putById.stackmapID());
            if (iter == recordMap.end()) {
                // It was optimized out.
                continue;
            }
            
            for (unsigned i = 0; i < iter->value.size(); ++i) {
                StackMaps::Record& record = iter->value[i];
                
                RegisterSet usedRegisters = usedRegistersFor(record);
                
                GPRReg base = record.locations[0].directGPR();
                GPRReg value = record.locations[1].directGPR();
                
                JITPutByIdGenerator gen(
                    codeBlock, putById.codeOrigin(), usedRegisters, JSValueRegs(base),
                    JSValueRegs(value), GPRInfo::patchpointScratchRegister, NeedToSpill,
                    putById.ecmaMode(), putById.putKind());
                
                MacroAssembler::Label begin = slowPathJIT.label();
                
                MacroAssembler::Call call = callOperation(
                    state, usedRegisters, slowPathJIT, putById.codeOrigin(), &exceptionTarget,
                    gen.slowPathFunction(), gen.stubInfo(), value, base, putById.uid());
                
                gen.reportSlowPathCall(begin, call);
                
                putById.m_slowPathDone.append(slowPathJIT.jump());
                putById.m_generators.append(gen);
            }
        }

        for (unsigned i = state.checkIns.size(); i--;) {
            CheckInDescriptor& checkIn = state.checkIns[i];
            
            if (verboseCompilationEnabled())
                dataLog("Handling checkIn stackmap #", checkIn.stackmapID(), "\n");
            
            auto iter = recordMap.find(checkIn.stackmapID());
            if (iter == recordMap.end()) {
                // It was optimized out.
                continue;
            }
            
            for (unsigned i = 0; i < iter->value.size(); ++i) {
                StackMaps::Record& record = iter->value[i];
                RegisterSet usedRegisters = usedRegistersFor(record);
                GPRReg result = record.locations[0].directGPR();
                GPRReg obj = record.locations[1].directGPR();
                StructureStubInfo* stubInfo = codeBlock->addStubInfo(); 
                stubInfo->codeOrigin = checkIn.codeOrigin();
                stubInfo->patch.baseGPR = static_cast<int8_t>(obj);
                stubInfo->patch.valueGPR = static_cast<int8_t>(result);
                stubInfo->patch.usedRegisters = usedRegisters;
                stubInfo->patch.spillMode = NeedToSpill;

                MacroAssembler::Label begin = slowPathJIT.label();

                MacroAssembler::Call slowCall = callOperation(
                    state, usedRegisters, slowPathJIT, checkIn.codeOrigin(), &exceptionTarget,
                    operationInOptimize, result, stubInfo, obj, checkIn.m_uid);

                checkIn.m_slowPathDone.append(slowPathJIT.jump());
                
                checkIn.m_generators.append(CheckInGenerator(stubInfo, slowCall, begin));
            }
        }
        
        exceptionTarget.link(&slowPathJIT);
        MacroAssembler::Jump exceptionJump = slowPathJIT.jump();
        
        state.finalizer->sideCodeLinkBuffer = std::make_unique<LinkBuffer>(vm, slowPathJIT, codeBlock, JITCompilationCanFail);
        if (state.finalizer->sideCodeLinkBuffer->didFailToAllocate()) {
            state.allocationFailed = true;
            return;
        }
        state.finalizer->sideCodeLinkBuffer->link(
            exceptionJump, state.finalizer->handleExceptionsLinkBuffer->entrypoint());
        
        for (unsigned i = state.getByIds.size(); i--;) {
            generateICFastPath(
                state, codeBlock, generatedFunction, recordMap, state.getByIds[i],
                sizeOfGetById());
        }
        for (unsigned i = state.putByIds.size(); i--;) {
            generateICFastPath(
                state, codeBlock, generatedFunction, recordMap, state.putByIds[i],
                sizeOfPutById());
        }

        for (unsigned i = state.checkIns.size(); i--;) {
            generateCheckInICFastPath(
                state, codeBlock, generatedFunction, recordMap, state.checkIns[i],
                sizeOfIn()); 
        } 
    }
    
    adjustCallICsForStackmaps(state.jsCalls, recordMap);
    
    for (unsigned i = state.jsCalls.size(); i--;) {
        JSCall& call = state.jsCalls[i];

        CCallHelpers fastPathJIT(&vm, codeBlock);
        call.emit(fastPathJIT);

        char* startOfIC = bitwise_cast<char*>(generatedFunction) + call.m_instructionOffset;

        generateInlineIfPossibleOutOfLineIfNot(state, vm, codeBlock, fastPathJIT, startOfIC, sizeOfCall(), "JSCall inline cache", [&] (LinkBuffer& linkBuffer, CCallHelpers&, bool) {
            call.link(vm, linkBuffer);
        });
    }
    
    adjustCallICsForStackmaps(state.jsCallVarargses, recordMap);
    
    for (unsigned i = state.jsCallVarargses.size(); i--;) {
        JSCallVarargs& call = state.jsCallVarargses[i];
        
        CCallHelpers fastPathJIT(&vm, codeBlock);
        call.emit(fastPathJIT, varargsSpillSlotsOffset);

        char* startOfIC = bitwise_cast<char*>(generatedFunction) + call.m_instructionOffset;
        size_t sizeOfIC = sizeOfICFor(call.node());

        generateInlineIfPossibleOutOfLineIfNot(state, vm, codeBlock, fastPathJIT, startOfIC, sizeOfIC, "varargs call inline cache", [&] (LinkBuffer& linkBuffer, CCallHelpers&, bool) {
            call.link(vm, linkBuffer, state.finalizer->handleExceptionsLinkBuffer->entrypoint());
        });
    }
    
    RepatchBuffer repatchBuffer(codeBlock);

    auto iter = recordMap.find(state.handleStackOverflowExceptionStackmapID);
    // It's sort of remotely possible that we won't have an in-band exception handling
    // path, for some kinds of functions.
    if (iter != recordMap.end()) {
        for (unsigned i = iter->value.size(); i--;) {
            StackMaps::Record& record = iter->value[i];
            
            CodeLocationLabel source = CodeLocationLabel(
                bitwise_cast<char*>(generatedFunction) + record.instructionOffset);

            RELEASE_ASSERT(stackOverflowException.isSet());

            repatchBuffer.replaceWithJump(source, state.finalizer->handleExceptionsLinkBuffer->locationOf(stackOverflowException));
        }
    }
    
    iter = recordMap.find(state.handleExceptionStackmapID);
    // It's sort of remotely possible that we won't have an in-band exception handling
    // path, for some kinds of functions.
    if (iter != recordMap.end()) {
        for (unsigned i = iter->value.size(); i--;) {
            StackMaps::Record& record = iter->value[i];
            
            CodeLocationLabel source = CodeLocationLabel(
                bitwise_cast<char*>(generatedFunction) + record.instructionOffset);
            
            repatchBuffer.replaceWithJump(source, state.finalizer->handleExceptionsLinkBuffer->entrypoint());
        }
    }
    
    for (unsigned exitIndex = 0; exitIndex < jitCode->osrExit.size(); ++exitIndex) {
        OSRExitCompilationInfo& info = state.finalizer->osrExit[exitIndex];
        OSRExit& exit = jitCode->osrExit[exitIndex];
        iter = recordMap.find(exit.m_stackmapID);
        
        Vector<const void*> codeAddresses;
        
        if (iter != recordMap.end()) {
            for (unsigned i = iter->value.size(); i--;) {
                StackMaps::Record& record = iter->value[i];
                
                CodeLocationLabel source = CodeLocationLabel(
                    bitwise_cast<char*>(generatedFunction) + record.instructionOffset);
                
                codeAddresses.append(bitwise_cast<char*>(generatedFunction) + record.instructionOffset + MacroAssembler::maxJumpReplacementSize());
                
                if (info.m_isInvalidationPoint)
                    jitCode->common.jumpReplacements.append(JumpReplacement(source, info.m_thunkAddress));
                else
                    repatchBuffer.replaceWithJump(source, info.m_thunkAddress);
            }
        }
        
        if (graph.compilation())
            graph.compilation()->addOSRExitSite(codeAddresses);
    }
}

void compile(State& state, Safepoint::Result& safepointResult)
{
    char* error = 0;
    
    {
        GraphSafepoint safepoint(state.graph, safepointResult);
        
        LLVMMCJITCompilerOptions options;
        llvm->InitializeMCJITCompilerOptions(&options, sizeof(options));
        options.OptLevel = Options::llvmBackendOptimizationLevel();
        options.NoFramePointerElim = true;
        if (Options::useLLVMSmallCodeModel())
            options.CodeModel = LLVMCodeModelSmall;
        options.EnableFastISel = enableLLVMFastISel;
        options.MCJMM = llvm->CreateSimpleMCJITMemoryManager(
            &state, mmAllocateCodeSection, mmAllocateDataSection, mmApplyPermissions, mmDestroy);
    
        LLVMExecutionEngineRef engine;
        
        if (isARM64()) {
#if OS(DARWIN)
            llvm->SetTarget(state.module, "arm64-apple-ios");
#elif OS(LINUX)
            llvm->SetTarget(state.module, "aarch64-linux-gnu");
#else
#error "Unrecognized OS"
#endif
        }

        if (llvm->CreateMCJITCompilerForModule(&engine, state.module, &options, sizeof(options), &error)) {
            dataLog("FATAL: Could not create LLVM execution engine: ", error, "\n");
            CRASH();
        }
        
        // At this point we no longer own the module.
        LModule module = state.module;
        state.module = nullptr;

        // The data layout also has to be set in the module. Get the data layout from the MCJIT and apply
        // it to the module.
        LLVMTargetMachineRef targetMachine = llvm->GetExecutionEngineTargetMachine(engine);
        LLVMTargetDataRef targetData = llvm->GetExecutionEngineTargetData(engine);
        char* stringRepOfTargetData = llvm->CopyStringRepOfTargetData(targetData);
        llvm->SetDataLayout(module, stringRepOfTargetData);
        free(stringRepOfTargetData);

        LLVMPassManagerRef functionPasses = 0;
        LLVMPassManagerRef modulePasses;

        if (Options::llvmSimpleOpt()) {
            modulePasses = llvm->CreatePassManager();
            llvm->AddTargetData(targetData, modulePasses);
            llvm->AddAnalysisPasses(targetMachine, modulePasses);
            llvm->AddPromoteMemoryToRegisterPass(modulePasses);
            llvm->AddGlobalOptimizerPass(modulePasses);
            llvm->AddFunctionInliningPass(modulePasses);
            llvm->AddPruneEHPass(modulePasses);
            llvm->AddGlobalDCEPass(modulePasses);
            llvm->AddConstantPropagationPass(modulePasses);
            llvm->AddAggressiveDCEPass(modulePasses);
            llvm->AddInstructionCombiningPass(modulePasses);
            // BEGIN - DO NOT CHANGE THE ORDER OF THE ALIAS ANALYSIS PASSES
            llvm->AddTypeBasedAliasAnalysisPass(modulePasses);
            llvm->AddBasicAliasAnalysisPass(modulePasses);
            // END - DO NOT CHANGE THE ORDER OF THE ALIAS ANALYSIS PASSES
            llvm->AddGVNPass(modulePasses);
            llvm->AddCFGSimplificationPass(modulePasses);
            llvm->AddDeadStoreEliminationPass(modulePasses);
            
            if (enableLLVMFastISel)
                llvm->AddLowerSwitchPass(modulePasses);

            llvm->RunPassManager(modulePasses, module);
        } else {
            LLVMPassManagerBuilderRef passBuilder = llvm->PassManagerBuilderCreate();
            llvm->PassManagerBuilderSetOptLevel(passBuilder, Options::llvmOptimizationLevel());
            llvm->PassManagerBuilderUseInlinerWithThreshold(passBuilder, 275);
            llvm->PassManagerBuilderSetSizeLevel(passBuilder, Options::llvmSizeLevel());
        
            functionPasses = llvm->CreateFunctionPassManagerForModule(module);
            modulePasses = llvm->CreatePassManager();
        
            llvm->AddTargetData(llvm->GetExecutionEngineTargetData(engine), modulePasses);
        
            llvm->PassManagerBuilderPopulateFunctionPassManager(passBuilder, functionPasses);
            llvm->PassManagerBuilderPopulateModulePassManager(passBuilder, modulePasses);
        
            llvm->PassManagerBuilderDispose(passBuilder);
        
            llvm->InitializeFunctionPassManager(functionPasses);
            for (LValue function = llvm->GetFirstFunction(module); function; function = llvm->GetNextFunction(function))
                llvm->RunFunctionPassManager(functionPasses, function);
            llvm->FinalizeFunctionPassManager(functionPasses);
        
            llvm->RunPassManager(modulePasses, module);
        }

        if (shouldDumpDisassembly() || verboseCompilationEnabled())
            state.dumpState(module, "after optimization");
        
        // FIXME: Need to add support for the case where JIT memory allocation failed.
        // https://bugs.webkit.org/show_bug.cgi?id=113620
        state.generatedFunction = reinterpret_cast<GeneratedFunction>(llvm->GetPointerToGlobal(engine, state.function));
        if (functionPasses)
            llvm->DisposePassManager(functionPasses);
        llvm->DisposePassManager(modulePasses);
        llvm->DisposeExecutionEngine(engine);
    }

    if (safepointResult.didGetCancelled())
        return;
    RELEASE_ASSERT(!state.graph.m_vm.heap.isCollecting());
    
    if (state.allocationFailed)
        return;
    
    if (shouldDumpDisassembly()) {
        for (unsigned i = 0; i < state.jitCode->handles().size(); ++i) {
            ExecutableMemoryHandle* handle = state.jitCode->handles()[i].get();
            dataLog(
                "Generated LLVM code for ",
                CodeBlockWithJITType(state.graph.m_codeBlock, JITCode::FTLJIT),
                " #", i, ", ", state.codeSectionNames[i], ":\n");
            disassemble(
                MacroAssemblerCodePtr(handle->start()), handle->sizeInBytes(),
                "    ", WTF::dataFile(), LLVMSubset);
        }
        
        for (unsigned i = 0; i < state.jitCode->dataSections().size(); ++i) {
            DataSection* section = state.jitCode->dataSections()[i].get();
            dataLog(
                "Generated LLVM data section for ",
                CodeBlockWithJITType(state.graph.m_codeBlock, JITCode::FTLJIT),
                " #", i, ", ", state.dataSectionNames[i], ":\n");
            dumpDataSection(section, "    ");
        }
    }
    
    std::unique_ptr<RegisterAtOffsetList> registerOffsets = parseUnwindInfo(
        state.unwindDataSection, state.unwindDataSectionSize,
        state.generatedFunction);
    if (shouldDumpDisassembly()) {
        dataLog("Unwind info for ", CodeBlockWithJITType(state.graph.m_codeBlock, JITCode::FTLJIT), ":\n");
        dataLog("    ", *registerOffsets, "\n");
    }
    state.graph.m_codeBlock->setCalleeSaveRegisters(WTF::move(registerOffsets));
    
    if (state.stackmapsSection && state.stackmapsSection->size()) {
        if (shouldDumpDisassembly()) {
            dataLog(
                "Generated LLVM stackmaps section for ",
                CodeBlockWithJITType(state.graph.m_codeBlock, JITCode::FTLJIT), ":\n");
            dataLog("    Raw data:\n");
            dumpDataSection(state.stackmapsSection.get(), "    ");
        }
        
        RefPtr<DataView> stackmapsData = DataView::create(
            ArrayBuffer::create(state.stackmapsSection->base(), state.stackmapsSection->size()));
        state.jitCode->stackmaps.parse(stackmapsData.get());
    
        if (shouldDumpDisassembly()) {
            dataLog("    Structured data:\n");
            state.jitCode->stackmaps.dumpMultiline(WTF::dataFile(), "        ");
        }
        
        StackMaps::RecordMap recordMap = state.jitCode->stackmaps.computeRecordMap();
        fixFunctionBasedOnStackMaps(
            state, state.graph.m_codeBlock, state.jitCode.get(), state.generatedFunction,
            recordMap);
        if (state.allocationFailed)
            return;
        
        if (shouldDumpDisassembly() || Options::asyncDisassembly()) {
            for (unsigned i = 0; i < state.jitCode->handles().size(); ++i) {
                if (state.codeSectionNames[i] != SECTION_NAME("text"))
                    continue;
                
                ExecutableMemoryHandle* handle = state.jitCode->handles()[i].get();
                
                CString header = toCString(
                    "Generated LLVM code after stackmap-based fix-up for ",
                    CodeBlockWithJITType(state.graph.m_codeBlock, JITCode::FTLJIT),
                    " in ", state.graph.m_plan.mode, " #", i, ", ",
                    state.codeSectionNames[i], ":\n");
                
                if (Options::asyncDisassembly()) {
                    disassembleAsynchronously(
                        header, MacroAssemblerCodeRef(handle), handle->sizeInBytes(), "    ",
                        LLVMSubset);
                    continue;
                }
                
                dataLog(header);
                disassemble(
                    MacroAssemblerCodePtr(handle->start()), handle->sizeInBytes(),
                    "    ", WTF::dataFile(), LLVMSubset);
            }
        }
    }
}

} } // namespace JSC::FTL

#endif // ENABLE(FTL_JIT)

