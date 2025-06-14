set(WTF_PUBLIC_HEADERS
    ASCIICType.h
    AggregateLogger.h
    Algorithms.h
    AnsiColors.h
    ApproximateTime.h
    Assertions.h
    Atomics.h
    AutodrainedPool.h
    AutomaticThread.h
    BackwardsGraph.h
    Bag.h
    BagToHashMap.h
    BitVector.h
    Bitmap.h
    BlockObjCExceptions.h
    BlockPtr.h
    BloomFilter.h
    BooleanLattice.h
    Box.h
    BoxPtr.h
    Brigand.h
    BubbleSort.h
    BumpPointerAllocator.h
    ByteOrder.h
    CPUTime.h
    CagedPtr.h
    CagedUniquePtr.h
    CallbackAggregator.h
    CancellableTask.h
    CheckedArithmetic.h
    CheckedBoolean.h
    CheckedPtr.h
    CheckedRef.h
    ClockType.h
    CommaPrinter.h
    CompactPointerTuple.h
    CompactRefPtrTuple.h
    CompactUniquePtrTuple.h
    CompilationThread.h
    Compiler.h
    CompletionHandler.h
    ConcurrentBuffer.h
    ConcurrentPtrHashSet.h
    ConcurrentVector.h
    Condition.h
    CountingLock.h
    CrossThreadCopier.h
    CrossThreadQueue.h
    CrossThreadTask.h
    CrossThreadTaskHandler.h
    CryptographicUtilities.h
    CryptographicallyRandomNumber.h
    DataLog.h
    DataMutex.h
    DataRef.h
    DateMath.h
    DebugHeap.h
    DebugUtilities.h
    DeferrableRefCounted.h
    Deque.h
    DisallowCType.h
    Dominators.h
    DoublyLinkedList.h
    EmbeddedFixedVector.h
    EnumClassOperatorOverloads.h
    EnumTraits.h
    Expected.h
    ExportMacros.h
    FailureAction.h
    FastBitVector.h
    FastMalloc.h
    FastTLS.h
    FilePrintStream.h
    FileSystem.h
    FixedVector.h
    FlipBytes.h
    ForbidHeapAllocation.h
    Forward.h
    Function.h
    FunctionDispatcher.h
    FunctionTraits.h
    GenericTimeMixin.h
    GetPtr.h
    Gigacage.h
    GlobalVersion.h
    GraphNodeWorklist.h
    GregorianDateTime.h
    HashCountedSet.h
    HashFunctions.h
    HashIterators.h
    HashMap.h
    HashMethod.h
    HashSet.h
    HashTable.h
    HashTraits.h
    Hasher.h
    HexNumber.h
    Identified.h
    Indenter.h
    IndexKeyType.h
    IndexMap.h
    IndexSet.h
    IndexSparseSet.h
    IndexedContainerIterator.h
    InlineASM.h
    Insertion.h
    InstanceCounted.h
    Int128.h
    InterferenceGraph.h
    IsoMalloc.h
    IsoMallocInlines.h
    IterationStatus.h
    IteratorAdaptors.h
    IteratorRange.h
    JSONValues.h
    JSValueMalloc.h
    KeyValuePair.h
    LEBDecoder.h
    Language.h
    LikelyDenseUnsignedIntegerSet.h
    ListDump.h
    ListHashSet.h
    Liveness.h
    Lock.h
    LockAlgorithm.h
    LockAlgorithmInlines.h
    LockedPrintStream.h
    Locker.h
    LocklessBag.h
    LogChannels.h
    LogInitialization.h
    Logger.h
    LoggerHelper.h
    Logging.h
    LoggingAccumulator.h
    LoggingHashID.h
    LoggingHashMap.h
    LoggingHashSet.h
    LoggingHashTraits.h
    MachSendRight.h
    MainThread.h
    MainThreadData.h
    MallocPtr.h
    Markable.h
    MathExtras.h
    MediaTime.h
    MemoryFootprint.h
    MemoryPressureHandler.h
    MessageQueue.h
    MetaAllocator.h
    MetaAllocatorHandle.h
    MetaAllocatorPtr.h
    MonotonicTime.h
    NakedPtr.h
    NakedRef.h
    NaturalLoops.h
    NeverDestroyed.h
    NoLock.h
    NoTailCalls.h
    Noncopyable.h
    Nonmovable.h
    NotFound.h
    NumberOfCores.h
    OSAllocator.h
    OSLogPrintStream.h
    OSObjectPtr.h
    OSRandomSource.h
    ObjCRuntimeExtras.h
    ObjectIdentifier.h
    Observer.h
    OptionSet.h
    OptionSetHash.h
    OrderMaker.h
    Packed.h
    PackedRef.h
    PackedRefPtr.h
    PageAllocation.h
    PageBlock.h
    PageReservation.h
    ParallelHelperPool.h
    ParallelJobs.h
    ParallelJobsGeneric.h
    ParallelJobsLibdispatch.h
    ParallelJobsOpenMP.h
    ParallelVectorIterator.h
    ParkingLot.h
    Platform.h
    PlatformCallingConventions.h
    PlatformCPU.h
    PlatformEnable.h
    PlatformEnableCocoa.h
    PlatformEnablePlayStation.h
    PlatformEnableWinApple.h
    PlatformEnableWinCairo.h
    PlatformHave.h
    PlatformLegacy.h
    PlatformOS.h
    PlatformRegisters.h
    PlatformUse.h
    PointerComparison.h
    PointerPreparations.h
    PrintStream.h
    PriorityQueue.h
    ProcessID.h
    ProcessPrivilege.h
    PtrTag.h
    RAMSize.h
    RandomDevice.h
    RandomNumber.h
    RandomNumberSeed.h
    Range.h
    RangeSet.h
    RawPointer.h
    RawPtrTraits.h
    RawValueTraits.h
    ReadWriteLock.h
    RecursableLambda.h
    RecursiveLockAdapter.h
    RedBlackTree.h
    Ref.h
    RefCounted.h
    RefCountedFixedVector.h
    RefCountedLeakCounter.h
    RefCounter.h
    RefPtr.h
    ResourceUsage.h
    RetainPtr.h
    RobinHoodHashMap.h
    RobinHoodHashSet.h
    RobinHoodHashTable.h
    RunLoop.h
    SHA1.h
    SafeStrerror.h
    SaturatedArithmetic.h
    SchedulePair.h
    Scope.h
    ScopedLambda.h
    Seconds.h
    SegmentedVector.h
    SentinelLinkedList.h
    SetForScope.h
    SharedTask.h
    SignedPtr.h
    SimpleStats.h
    SingleRootGraph.h
    SinglyLinkedList.h
    SinglyLinkedListWithTail.h
    SixCharacterHash.h
    SmallSet.h
    SoftLinking.h
    SortedArrayMap.h
    Span.h
    SpanningTree.h
    Spectrum.h
    StackBounds.h
    StackCheck.h
    StackPointer.h
    StackShot.h
    StackShotProfiler.h
    StackStats.h
    StackTrace.h
    StdFilesystem.h
    StdIntExtras.h
    StdLibExtras.h
    StdList.h
    StdMap.h
    StdSet.h
    StdUnorderedMap.h
    StdUnorderedSet.h
    Stopwatch.h
    StreamBuffer.h
    StringExtras.h
    StringHashDumpContext.h
    StringPrintStream.h
    SuspendableWorkQueue.h
    SynchronizedFixedQueue.h
    SystemFree.h
    SystemMalloc.h
    SystemTracing.h
    TaggedArrayStoragePtr.h
    ThreadAssertions.h
    ThreadGroup.h
    ThreadMessage.h
    ThreadSafetyAnalysis.h
    ThreadSafeRefCounted.h
    ThreadSanitizerSupport.h
    ThreadSpecific.h
    Threading.h
    ThreadingPrimitives.h
    TimeWithDynamicClockType.h
    TimingScope.h
    TinyLRUCache.h
    TinyPtrSet.h
    TrailingArray.h
    TranslatedProcess.h
    TriState.h
    TypeCasts.h
    URL.h
    URLHelpers.h
    URLHash.h
    URLParser.h
    UUID.h
    UnalignedAccess.h
    Unexpected.h
    UniStdExtras.h
    UnionFind.h
    UniqueArray.h
    UniqueRef.h
    UnsafePointer.h
    VMTags.h
    ValueCheck.h
    Vector.h
    VectorHash.h
    VectorTraits.h
    WTFConfig.h
    WTFSemaphore.h
    WallTime.h
    WeakHashCountedSet.h
    WeakHashMap.h
    WeakHashSet.h
    WeakPtr.h
    WeakRandom.h
    WindowsExtras.h
    WordLock.h
    WorkQueue.h
    WorkerPool.h
    dtoa.h

    dtoa/bignum-dtoa.h
    dtoa/bignum.h
    dtoa/cached-powers.h
    dtoa/diy-fp.h
    dtoa/double-conversion.h
    dtoa/fast-dtoa.h
    dtoa/fixed-dtoa.h
    dtoa/ieee.h
    dtoa/strtod.h
    dtoa/utils.h

    persistence/PersistentCoder.h
    persistence/PersistentCoders.h
    persistence/PersistentDecoder.h
    persistence/PersistentEncoder.h

    text/ASCIIFastPath.h
    text/ASCIILiteral.h
    text/AtomString.h
    text/AtomStringHash.h
    text/AtomStringImpl.h
    text/AtomStringTable.h
    text/Base64.h
    text/CString.h
    text/CodePointIterator.h
    text/ConversionMode.h
    text/ExternalStringImpl.h
    text/IntegerToStringConversion.h
    text/LChar.h
    text/LineBreakIteratorPoolICU.h
    text/LineEnding.h
    text/NullTextBreakIterator.h
    text/OrdinalNumber.h
    text/StringBuffer.h
    text/StringBuilder.h
    text/StringBuilderInternals.h
    text/StringCommon.h
    text/StringConcatenate.h
    text/StringConcatenateNumbers.h
    text/StringHash.h
    text/StringHasher.h
    text/StringImpl.h
    text/StringOperators.h
    text/StringParsingBuffer.h
    text/StringToIntegerConversion.h
    text/StringView.h
    text/SymbolImpl.h
    text/SymbolRegistry.h
    text/TextBreakIterator.h
    text/TextBreakIteratorInternalICU.h
    text/TextPosition.h
    text/TextStream.h
    text/UTF8ConversionError.h
    text/UniquedStringImpl.h
    text/WTFString.h

    text/icu/TextBreakIteratorICU.h
    text/icu/UTextProvider.h
    text/icu/UTextProviderLatin1.h
    text/icu/UTextProviderUTF16.h

    threads/BinarySemaphore.h
    threads/Signals.h

    unicode/CharacterNames.h
    unicode/Collator.h
    unicode/UTF8Conversion.h

    unicode/icu/ICUHelpers.h
)

set(WTF_SOURCES
    ASCIICType.cpp
    ApproximateTime.cpp
    Assertions.cpp
    AutomaticThread.cpp
    Bag.cpp
    BitVector.cpp
    CPUTime.cpp
    ClockType.cpp
    CompilationThread.cpp
    ConcurrentBuffer.cpp
    ConcurrentPtrHashSet.cpp
    CountingLock.cpp
    CrossThreadCopier.cpp
    CrossThreadTaskHandler.cpp
    CryptographicUtilities.cpp
    CryptographicallyRandomNumber.cpp
    CurrentTime.cpp
    DataLog.cpp
    DateMath.cpp
    DebugHeap.cpp
    FastBitVector.cpp
    FastMalloc.cpp
    FilePrintStream.cpp
    FileSystem.cpp
    FunctionDispatcher.cpp
    Gigacage.cpp
    GlobalVersion.cpp
    GregorianDateTime.cpp
    HashTable.cpp
    HexNumber.cpp
    Int128.cpp
    JSONValues.cpp
    JSValueMalloc.cpp
    Language.cpp
    LikelyDenseUnsignedIntegerSet.cpp
    Lock.cpp
    LockedPrintStream.cpp
    LogChannels.cpp
    LogInitialization.cpp
    Logger.cpp
    Logging.cpp
    MainThread.cpp
    MediaTime.cpp
    MemoryPressureHandler.cpp
    MetaAllocator.cpp
    MonotonicTime.cpp
    NumberOfCores.cpp
    OSLogPrintStream.cpp
    OSRandomSource.cpp
    ObjectIdentifier.cpp
    PageBlock.cpp
    ParallelHelperPool.cpp
    ParallelJobsGeneric.cpp
    ParkingLot.cpp
    PrintStream.cpp
    ProcessPrivilege.cpp
    RAMSize.cpp
    RandomDevice.cpp
    RandomNumber.cpp
    ReadWriteLock.cpp
    RefCounted.cpp
    RefCountedLeakCounter.cpp
    RunLoop.cpp
    SHA1.cpp
    SafeStrerror.cpp
    Seconds.cpp
    SegmentedVector.cpp
    SixCharacterHash.cpp
    SmallSet.cpp
    StackBounds.cpp
    StackPointer.cpp
    StackStats.cpp
    StackTrace.cpp
    StringPrintStream.cpp
    SuspendableWorkQueue.cpp
    ThreadGroup.cpp
    ThreadMessage.cpp
    Threading.cpp
    TimeWithDynamicClockType.cpp
    TimingScope.cpp
    URL.cpp
    URLHelpers.cpp
    URLParser.cpp
    UUID.cpp
    UniqueArray.cpp
    Vector.cpp
    WTFAssertions.cpp
    WTFConfig.cpp
    WallTime.cpp
    WordLock.cpp
    WorkQueue.cpp
    WorkerPool.cpp
    dtoa.cpp

    dtoa/bignum-dtoa.cc
    dtoa/bignum.cc
    dtoa/cached-powers.cc
    dtoa/diy-fp.cc
    dtoa/double-conversion.cc
    dtoa/fast-dtoa.cc
    dtoa/fixed-dtoa.cc
    dtoa/strtod.cc

    persistence/PersistentCoders.cpp
    persistence/PersistentDecoder.cpp
    persistence/PersistentEncoder.cpp

    text/ASCIILiteral.cpp
    text/AtomString.cpp
    text/AtomStringImpl.cpp
    text/AtomStringTable.cpp
    text/Base64.cpp
    text/CString.cpp
    text/ExternalStringImpl.cpp
    text/LineBreakIteratorPoolICU.cpp
    text/LineEnding.cpp
    text/StringBuffer.cpp
    text/StringBuilder.cpp
    text/StringBuilderJSON.cpp
    text/StringImpl.cpp
    text/StringView.cpp
    text/SymbolImpl.cpp
    text/SymbolRegistry.cpp
    text/TextBreakIterator.cpp
    text/TextStream.cpp
    text/WTFString.cpp

    text/icu/UTextProvider.cpp
    text/icu/UTextProviderLatin1.cpp
    text/icu/UTextProviderUTF16.cpp

    threads/BinarySemaphore.cpp
    threads/Signals.cpp

    unicode/CollatorDefault.cpp
    unicode/UTF8Conversion.cpp

    unicode/icu/CollatorICU.cpp
    unicode/icu/ICUHelpers.cpp
)

set(WTF_PRIVATE_INCLUDE_DIRECTORIES
    "${CMAKE_BINARY_DIR}"
    "${WTF_DERIVED_SOURCES_DIR}"
    "${WTF_DIR}"
    "${WTF_DIR}/wtf"
    "${WTF_DIR}/wtf/dtoa"
    "${WTF_DIR}/wtf/persistence"
    "${WTF_DIR}/wtf/text"
    "${WTF_DIR}/wtf/text/icu"
    "${WTF_DIR}/wtf/threads"
    "${WTF_DIR}/wtf/unicode"
)

set(WTF_LIBRARIES
    ${CMAKE_DL_LIBS}
    ICU::data
    ICU::i18n
    ICU::uc
)

if (NOT USE_SYSTEM_MALLOC)
    set(WTF_FRAMEWORKS bmalloc)
endif ()

if (ATOMICS_REQUIRE_LIBATOMIC)
    list(APPEND WTF_LIBRARIES atomic)
endif ()

if (COMPILER_IS_GCC_OR_CLANG)
    if (HAVE_STD_EXPERIMENTAL_FILESYSTEM)
        list(APPEND WTF_LIBRARIES stdc++fs)
    endif ()
endif ()

set(WTF_INTERFACE_LIBRARIES WTF)
set(WTF_INTERFACE_INCLUDE_DIRECTORIES ${WTF_FRAMEWORK_HEADERS_DIR})
set(WTF_INTERFACE_DEPENDENCIES WTF_CopyHeaders)

WEBKIT_FRAMEWORK_DECLARE(WTF)
WEBKIT_INCLUDE_CONFIG_FILES_IF_EXISTS()

WEBKIT_COPY_FILES(WTF_CopyHeaders
    DESTINATION ${WTF_FRAMEWORK_HEADERS_DIR}/wtf
    FILES ${WTF_PUBLIC_HEADERS}
)

WEBKIT_FRAMEWORK(WTF)
WEBKIT_FRAMEWORK_TARGET(WTF)

if (MSVC)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
endif ()

if (USE_VERSION_STAMPER)
    add_custom_command(
        TARGET WTF
        PRE_BUILD
        COMMAND ${PERL_EXECUTABLE} ${WEBKIT_LIBRARIES_DIR}/tools/scripts/auto-version.pl ${WTF_DERIVED_SOURCES_DIR}
        VERBATIM)

    add_custom_command(
        TARGET WTF
        POST_BUILD
        COMMAND ${PERL_EXECUTABLE} ${WEBKIT_LIBRARIES_DIR}/tools/scripts/version-stamp.pl ${WTF_DERIVED_SOURCES_DIR} $<TARGET_FILE:WTF>
        VERBATIM)
endif ()

file(MAKE_DIRECTORY ${WTF_SCRIPTS_DIR})

message(STATUS "Copying generate-unified-source-bundles.rb to: ${WTF_SCRIPTS_DIR}")

file(COPY
    ../Scripts/generate-unified-source-bundles.rb
    DESTINATION ${WTF_SCRIPTS_DIR}
)

set(WTF_PREFERENCES
    ../Scripts/GeneratePreferences.rb

    ../Scripts/Preferences/WebPreferences.yaml
    ../Scripts/Preferences/WebPreferencesDebug.yaml
    ../Scripts/Preferences/WebPreferencesExperimental.yaml
    ../Scripts/Preferences/WebPreferencesInternal.yaml
)

WEBKIT_COPY_FILES(WTF_CopyPreferences
    DESTINATION ${WTF_SCRIPTS_DIR}
    FILES ${WTF_PREFERENCES}
)
