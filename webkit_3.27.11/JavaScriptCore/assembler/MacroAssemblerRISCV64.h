/*
 * Copyright (C) 2021 Igalia S.L.
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

#pragma once

#if ENABLE(ASSEMBLER) && CPU(RISCV64)

#include "AbstractMacroAssembler.h"
#include "RISCV64Assembler.h"

#define MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD(methodName) \
    template<typename... Args> void methodName(Args&&...) { }
#define MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD_WITH_RETURN(methodName, returnType) \
    template<typename... Args> returnType methodName(Args&&...) { return { }; }

namespace JSC {

using Assembler = TARGET_ASSEMBLER;

class MacroAssemblerRISCV64 : public AbstractMacroAssembler<Assembler> {
public:
    static constexpr unsigned numGPRs = 32;
    static constexpr unsigned numFPRs = 32;

    static constexpr RegisterID dataTempRegister = RISCV64Registers::x30;
    static constexpr RegisterID memoryTempRegister = RISCV64Registers::x31;

    static constexpr FPRegisterID fpTempRegister = RISCV64Registers::f30;
    static constexpr FPRegisterID fpTempRegister2 = RISCV64Registers::f31;

    static constexpr RegisterID InvalidGPRReg = RISCV64Registers::InvalidGPRReg;

    RegisterID scratchRegister()
    {
        return dataTempRegister;
    }

    enum TempRegisterType : int8_t {
        Data,
        Memory,
    };

    template<TempRegisterType... RegisterTypes>
    struct TempRegister {
        RegisterID data()
        {
            static_assert(((RegisterTypes == Data) || ...));
            return dataTempRegister;
        }

        RegisterID memory()
        {
            static_assert(((RegisterTypes == Memory) || ...));
            return memoryTempRegister;
        }
    };

    template<TempRegisterType RegisterType>
    struct LazyTempRegister {
        LazyTempRegister(bool allowScratchRegister)
            : m_allowScratchRegister(allowScratchRegister)
        {
            static_assert(RegisterType == Data || RegisterType == Memory);
        }

        operator RegisterID()
        {
            RELEASE_ASSERT(m_allowScratchRegister);
            if constexpr (RegisterType == Data)
                return dataTempRegister;
            if constexpr (RegisterType == Memory)
                return memoryTempRegister;
            return InvalidGPRReg;
        }

        bool m_allowScratchRegister;
    };

    template<TempRegisterType... RegisterTypes>
    auto temps() -> TempRegister<RegisterTypes...>
    {
        RELEASE_ASSERT(m_allowScratchRegister);
        return { };
    }

    template<TempRegisterType RegisterType>
    auto lazyTemp() -> LazyTempRegister<RegisterType>
    {
        return { m_allowScratchRegister };
    }

    static bool supportsFloatingPoint() { return true; }
    static bool supportsFloatingPointTruncate() { return true; }
    static bool supportsFloatingPointSqrt() { return true; }
    static bool supportsFloatingPointAbs() { return true; }
    static bool supportsFloatingPointRounding() { return true; }

    enum RelationalCondition {
        Equal = Assembler::ConditionEQ,
        NotEqual = Assembler::ConditionNE,
        Above = Assembler::ConditionGTU,
        AboveOrEqual = Assembler::ConditionGEU,
        Below = Assembler::ConditionLTU,
        BelowOrEqual = Assembler::ConditionLEU,
        GreaterThan = Assembler::ConditionGT,
        GreaterThanOrEqual = Assembler::ConditionGE,
        LessThan = Assembler::ConditionLT,
        LessThanOrEqual = Assembler::ConditionLE,
    };

    static constexpr RelationalCondition invert(RelationalCondition cond)
    {
        return static_cast<RelationalCondition>(Assembler::invert(static_cast<Assembler::Condition>(cond)));
    }

    enum ResultCondition {
        Overflow,
        Signed,
        PositiveOrZero,
        Zero,
        NonZero,
    };

    enum ZeroCondition {
        IsZero,
        IsNonZero,
    };

    enum DoubleCondition {
        DoubleEqualAndOrdered,
        DoubleNotEqualAndOrdered,
        DoubleGreaterThanAndOrdered,
        DoubleGreaterThanOrEqualAndOrdered,
        DoubleLessThanAndOrdered,
        DoubleLessThanOrEqualAndOrdered,
        DoubleEqualOrUnordered,
        DoubleNotEqualOrUnordered,
        DoubleGreaterThanOrUnordered,
        DoubleGreaterThanOrEqualOrUnordered,
        DoubleLessThanOrUnordered,
        DoubleLessThanOrEqualOrUnordered,
    };

    static constexpr RegisterID stackPointerRegister = RISCV64Registers::sp;
    static constexpr RegisterID framePointerRegister = RISCV64Registers::fp;
    static constexpr RegisterID linkRegister = RISCV64Registers::ra;

    void add32(RegisterID src, RegisterID dest)
    {
        add32(src, dest, dest);
    }

    void add32(RegisterID op1, RegisterID op2, RegisterID dest)
    {
        m_assembler.addwInsn(dest, op1, op2);
        m_assembler.maskRegister<32>(dest);
    }

    void add32(TrustedImm32 imm, RegisterID dest)
    {
        add32(imm, dest, dest);
    }

    void add32(TrustedImm32 imm, RegisterID op2, RegisterID dest)
    {
        if (Imm::isValid<Imm::IType>(imm.m_value)) {
            m_assembler.addiwInsn(dest, op2, Imm::I(imm.m_value));
            m_assembler.maskRegister<32>(dest);
            return;
        }

        auto temp = temps<Data>();
        loadImmediate(imm, temp.data());
        m_assembler.addwInsn(dest, temp.data(), op2);
        m_assembler.maskRegister<32>(dest);
    }

    void add32(TrustedImm32 imm, AbsoluteAddress address)
    {
        auto temp = temps<Data, Memory>();
        loadImmediate(TrustedImmPtr(address.m_ptr), temp.memory());
        if (Imm::isValid<Imm::IType>(imm.m_value)) {
            m_assembler.lwInsn(temp.data(), temp.memory(), Imm::I<0>());
            m_assembler.addiInsn(temp.data(), temp.data(), Imm::I(imm.m_value));
            m_assembler.swInsn(temp.memory(), temp.data(), Imm::S<0>());
            return;
        }

        m_assembler.lwInsn(temp.memory(), temp.memory(), Imm::I<0>());
        loadImmediate(imm, temp.data());
        m_assembler.addInsn(temp.data(), temp.memory(), temp.data());

        loadImmediate(TrustedImmPtr(address.m_ptr), temp.memory());
        m_assembler.swInsn(temp.memory(), temp.data(), Imm::S<0>());
    }

    void add32(TrustedImm32 imm, Address address)
    {
        auto temp = temps<Data, Memory>();
        auto resolution = resolveAddress(address, temp.memory());
        if (Imm::isValid<Imm::IType>(imm.m_value)) {
            m_assembler.lwInsn(temp.data(), resolution.base, Imm::I(resolution.offset));
            m_assembler.addiInsn(temp.data(), temp.data(), Imm::I(imm.m_value));
            m_assembler.swInsn(resolution.base, temp.data(), Imm::S(resolution.offset));
            return;
        }

        m_assembler.lwInsn(temp.memory(), resolution.base, Imm::I(resolution.offset));
        loadImmediate(imm, temp.data());
        m_assembler.addInsn(temp.data(), temp.memory(), temp.data());

        resolution = resolveAddress(address, temp.memory());
        m_assembler.swInsn(resolution.base, temp.data(), Imm::S(resolution.offset));
    }

    void add32(Address address, RegisterID dest)
    {
        auto temp = temps<Data, Memory>();
        auto resolution = resolveAddress(address, temp.memory());
        m_assembler.lwInsn(temp.data(), resolution.base, Imm::I(resolution.offset));
        m_assembler.addwInsn(dest, temp.data(), dest);
        m_assembler.maskRegister<32>(dest);
    }

    void add64(RegisterID src, RegisterID dest)
    {
        add64(src, dest, dest);
    }

    void add64(RegisterID op1, RegisterID op2, RegisterID dest)
    {
        m_assembler.addInsn(dest, op1, op2);
    }

    void add64(TrustedImm32 imm, RegisterID dest)
    {
        add64(imm, dest, dest);
    }

    void add64(TrustedImm32 imm, RegisterID op2, RegisterID dest)
    {
        if (Imm::isValid<Imm::IType>(imm.m_value)) {
            m_assembler.addiInsn(dest, op2, Imm::I(imm.m_value));
            return;
        }

        auto temp = temps<Data>();
        loadImmediate(imm, temp.data());
        m_assembler.addInsn(dest, temp.data(), op2);
    }

    void add64(TrustedImm64 imm, RegisterID dest)
    {
        add64(imm, dest, dest);
    }

    void add64(TrustedImm64 imm, RegisterID op2, RegisterID dest)
    {
        if (Imm::isValid<Imm::IType>(imm.m_value)) {
            m_assembler.addiInsn(dest, op2, Imm::I(imm.m_value));
            return;
        }

        auto temp = temps<Data>();
        loadImmediate(imm, temp.data());
        m_assembler.addInsn(dest, temp.data(), op2);
    }

    void add64(TrustedImm32 imm, AbsoluteAddress address)
    {
        auto temp = temps<Data, Memory>();
        loadImmediate(TrustedImmPtr(address.m_ptr), temp.memory());

        if (Imm::isValid<Imm::IType>(imm.m_value)) {
            m_assembler.ldInsn(temp.data(), temp.memory(), Imm::I<0>());
            m_assembler.addiInsn(temp.data(), temp.data(), Imm::I(imm.m_value));
            m_assembler.sdInsn(temp.memory(), temp.data(), Imm::S<0>());
            return;
        }

        m_assembler.ldInsn(temp.memory(), temp.memory(), Imm::I<0>());
        loadImmediate(imm, temp.data());
        m_assembler.addInsn(temp.data(), temp.data(), temp.memory());

        loadImmediate(TrustedImmPtr(address.m_ptr), temp.memory());
        m_assembler.sdInsn(temp.memory(), temp.data(), Imm::S<0>());
    }

    void add64(TrustedImm32 imm, Address address)
    {
        auto temp = temps<Data, Memory>();
        auto resolution = resolveAddress(address, temp.memory());
        m_assembler.ldInsn(temp.data(), resolution.base, Imm::I(resolution.offset));

        if (Imm::isValid<Imm::IType>(imm.m_value)) {
            m_assembler.addiInsn(temp.data(), temp.data(), Imm::I(imm.m_value));
            m_assembler.sdInsn(resolution.base, temp.data(), Imm::S(resolution.offset));
            return;
        }

        loadImmediate(imm, temp.memory());
        m_assembler.addInsn(temp.data(), temp.memory(), temp.data());

        resolution = resolveAddress(address, temp.memory());
        m_assembler.sdInsn(resolution.base, temp.data(), Imm::S(resolution.offset));
    }

    void add64(AbsoluteAddress address, RegisterID dest)
    {
        auto temp = temps<Memory>();
        loadImmediate(TrustedImmPtr(address.m_ptr), temp.memory());
        m_assembler.ldInsn(temp.memory(), temp.memory(), Imm::I<0>());
        m_assembler.addInsn(dest, temp.memory(), dest);
    }

    void add64(Address address, RegisterID dest)
    {
        auto temp = temps<Data, Memory>();
        auto resolution = resolveAddress(address, temp.memory());
        m_assembler.ldInsn(temp.data(), resolution.base, Imm::I(resolution.offset));
        m_assembler.addInsn(dest, temp.data(), dest);
    }

    void sub32(RegisterID src, RegisterID dest)
    {
        sub32(dest, src, dest);
    }

    void sub32(RegisterID op1, RegisterID op2, RegisterID dest)
    {
        m_assembler.subwInsn(dest, op1, op2);
        m_assembler.maskRegister<32>(dest);
    }

    void sub32(TrustedImm32 imm, RegisterID dest)
    {
        sub32(dest, imm, dest);
    }

    void sub32(RegisterID op1, TrustedImm32 imm, RegisterID dest)
    {
        add32(TrustedImm32(-imm.m_value), op1, dest);
    }

    void sub32(TrustedImm32 imm, AbsoluteAddress address)
    {
        auto temp = temps<Data, Memory>();
        loadImmediate(TrustedImmPtr(address.m_ptr), temp.memory());

        if (Imm::isValid<Imm::IType>(-imm.m_value)) {
            m_assembler.lwInsn(temp.data(), temp.memory(), Imm::I<0>());
            m_assembler.addiwInsn(temp.data(), temp.data(), Imm::I(-imm.m_value));
            m_assembler.swInsn(temp.memory(), temp.data(), Imm::S<0>());
            return;
        }

        m_assembler.lwInsn(temp.memory(), temp.memory(), Imm::I<0>());
        loadImmediate(imm, temp.data());
        m_assembler.subwInsn(temp.data(), temp.memory(), temp.data());

        loadImmediate(TrustedImmPtr(address.m_ptr), temp.memory());
        m_assembler.swInsn(temp.memory(), temp.data(), Imm::S<0>());
    }

    void sub32(TrustedImm32 imm, Address address)
    {
        auto temp = temps<Data, Memory>();
        auto resolution = resolveAddress(address, temp.memory());
        m_assembler.lwInsn(temp.data(), resolution.base, Imm::I(resolution.offset));

        if (Imm::isValid<Imm::IType>(-imm.m_value)) {
            m_assembler.addiwInsn(temp.data(), temp.data(), Imm::I(-imm.m_value));
            m_assembler.swInsn(resolution.base, temp.data(), Imm::S(resolution.offset));
            return;
        }

        loadImmediate(imm, temp.memory());
        m_assembler.subwInsn(temp.data(), temp.data(), temp.memory());

        resolution = resolveAddress(address, temp.memory());
        m_assembler.swInsn(resolution.base, temp.data(), Imm::S(resolution.offset));
    }

    void sub32(Address address, RegisterID dest)
    {
        auto temp = temps<Data, Memory>();
        auto resolution = resolveAddress(address, temp.memory());
        m_assembler.lwInsn(temp.data(), resolution.base, Imm::I(resolution.offset));
        m_assembler.subwInsn(dest, dest, temp.data());
        m_assembler.maskRegister<32>(dest);
    }

    void sub64(RegisterID src, RegisterID dest)
    {
        sub64(dest, src, dest);
    }

    void sub64(RegisterID op1, RegisterID op2, RegisterID dest)
    {
        m_assembler.subInsn(dest, op1, op2);
    }

    void sub64(TrustedImm32 imm, RegisterID dest)
    {
        sub64(dest, imm, dest);
    }

    void sub64(RegisterID op1, TrustedImm32 imm, RegisterID dest)
    {
        add64(TrustedImm32(-imm.m_value), op1, dest);
    }

    void sub64(TrustedImm64 imm, RegisterID dest)
    {
        sub64(dest, imm, dest);
    }

    void sub64(RegisterID op1, TrustedImm64 imm, RegisterID dest)
    {
        add64(TrustedImm64(-imm.m_value), op1, dest);
    }

    void mul32(RegisterID src, RegisterID dest)
    {
        mul32(src, dest, dest);
    }

    void mul32(RegisterID lhs, RegisterID rhs, RegisterID dest)
    {
        m_assembler.mulwInsn(dest, lhs, rhs);
        m_assembler.maskRegister<32>(dest);
    }

    void mul32(TrustedImm32 imm, RegisterID rhs, RegisterID dest)
    {
        auto temp = temps<Data>();
        loadImmediate(imm, temp.data());
        m_assembler.mulwInsn(dest, temp.data(), rhs);
        m_assembler.maskRegister<32>(dest);
    }

    void mul64(RegisterID src, RegisterID dest)
    {
        mul64(src, dest, dest);
    }

    void mul64(RegisterID lhs, RegisterID rhs, RegisterID dest)
    {
        m_assembler.mulInsn(dest, lhs, rhs);
    }

    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD(extractUnsignedBitfield32);
    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD(extractUnsignedBitfield64);
    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD(insertUnsignedBitfieldInZero32);
    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD(insertUnsignedBitfieldInZero64);

    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD(countLeadingZeros32);
    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD(countLeadingZeros64);
    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD(countTrailingZeros32);
    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD(countTrailingZeros64);

    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD(byteSwap16);
    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD(byteSwap32);
    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD(byteSwap64);

    void lshift32(RegisterID shiftAmount, RegisterID dest)
    {
        lshift32(dest, shiftAmount, dest);
    }

    void lshift32(RegisterID src, RegisterID shiftAmount, RegisterID dest)
    {
        m_assembler.sllwInsn(dest, src, shiftAmount);
        m_assembler.maskRegister<32>(dest);
    }

    void lshift32(TrustedImm32 shiftAmount, RegisterID dest)
    {
        lshift32(dest, shiftAmount, dest);
    }

    void lshift32(RegisterID src, TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.slliwInsn(dest, src, uint32_t(imm.m_value & ((1 << 5) - 1)));
        m_assembler.maskRegister<32>(dest);
    }

    void lshift64(RegisterID shiftAmount, RegisterID dest)
    {
        lshift64(dest, shiftAmount, dest);
    }

    void lshift64(RegisterID src, RegisterID shiftAmount, RegisterID dest)
    {
        m_assembler.sllInsn(dest, src, shiftAmount);
    }

    void lshift64(TrustedImm32 shiftAmount, RegisterID dest)
    {
        lshift64(dest, shiftAmount, dest);
    }

    void lshift64(RegisterID src, TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.slliInsn(dest, src, uint32_t(imm.m_value & ((1 << 6) - 1)));
    }

    void rshift32(RegisterID shiftAmount, RegisterID dest)
    {
        rshift32(dest, shiftAmount, dest);
    }

    void rshift32(RegisterID src, RegisterID shiftAmount, RegisterID dest)
    {
        m_assembler.srawInsn(dest, src, shiftAmount);
        m_assembler.maskRegister<32>(dest);
    }

    void rshift32(TrustedImm32 shiftAmount, RegisterID dest)
    {
        rshift32(dest, shiftAmount, dest);
    }

    void rshift32(RegisterID src, TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.sraiwInsn(dest, src, uint32_t(imm.m_value & ((1 << 5) - 1)));
        m_assembler.maskRegister<32>(dest);
    }

    void rshift64(RegisterID shiftAmount, RegisterID dest)
    {
        rshift64(dest, shiftAmount, dest);
    }

    void rshift64(RegisterID src, RegisterID shiftAmount, RegisterID dest)
    {
        m_assembler.sraInsn(dest, src, shiftAmount);
    }

    void rshift64(TrustedImm32 shiftAmount, RegisterID dest)
    {
        rshift64(dest, shiftAmount, dest);
    }

    void rshift64(RegisterID src, TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.sraiInsn(dest, src, uint32_t(imm.m_value & ((1 << 6) - 1)));
    }

    void urshift32(RegisterID shiftAmount, RegisterID dest)
    {
        urshift32(dest, shiftAmount, dest);
    }

    void urshift32(RegisterID src, RegisterID shiftAmount, RegisterID dest)
    {
        m_assembler.srlwInsn(dest, src, shiftAmount);
        m_assembler.maskRegister<32>(dest);
    }

    void urshift32(TrustedImm32 shiftAmount, RegisterID dest)
    {
        urshift32(dest, shiftAmount, dest);
    }

    void urshift32(RegisterID src, TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.srliwInsn(dest, src, uint32_t(imm.m_value & ((1 << 5) - 1)));
        m_assembler.maskRegister<32>(dest);
    }

    void urshift64(RegisterID shiftAmount, RegisterID dest)
    {
        urshift64(dest, shiftAmount, dest);
    }

    void urshift64(RegisterID src, RegisterID shiftAmount, RegisterID dest)
    {
        m_assembler.srlInsn(dest, src, shiftAmount);
    }

    void urshift64(TrustedImm32 shiftAmount, RegisterID dest)
    {
        urshift64(dest, shiftAmount, dest);
    }

    void urshift64(RegisterID src, TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.srliInsn(dest, src, uint32_t(imm.m_value & ((1 << 6) - 1)));
    }

    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD(rotateRight32);
    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD(rotateRight64);

    void load8(Address address, RegisterID dest)
    {
        auto resolution = resolveAddress(address, lazyTemp<Memory>());
        m_assembler.lbuInsn(dest, resolution.base, Imm::I(resolution.offset));
    }

    void load8(BaseIndex address, RegisterID dest)
    {
        auto resolution = resolveAddress(address, lazyTemp<Memory>());
        m_assembler.lbuInsn(dest, resolution.base, Imm::I(resolution.offset));
    }

    void load8(const void* address, RegisterID dest)
    {
        auto temp = temps<Memory>();
        loadImmediate(TrustedImmPtr(address), temp.memory());
        m_assembler.lbuInsn(dest, temp.memory(), Imm::I<0>());
    }

    void load8SignedExtendTo32(Address address, RegisterID dest)
    {
        auto resolution = resolveAddress(address, lazyTemp<Memory>());
        m_assembler.lbInsn(dest, resolution.base, Imm::I(resolution.offset));
        m_assembler.maskRegister<32>(dest);
    }

    void load8SignedExtendTo32(BaseIndex address, RegisterID dest)
    {
        auto resolution = resolveAddress(address, lazyTemp<Memory>());
        m_assembler.lbInsn(dest, resolution.base, Imm::I(resolution.offset));
        m_assembler.maskRegister<32>(dest);
    }

    void load8SignedExtendTo32(const void* address, RegisterID dest)
    {
        auto temp = temps<Memory>();
        loadImmediate(TrustedImmPtr(address), temp.memory());
        m_assembler.lbInsn(dest, temp.memory(), Imm::I<0>());
        m_assembler.maskRegister<32>(dest);
    }

    void load16(Address address, RegisterID dest)
    {
        auto resolution = resolveAddress(address, lazyTemp<Memory>());
        m_assembler.lhuInsn(dest, resolution.base, Imm::I(resolution.offset));
    }

    void load16(BaseIndex address, RegisterID dest)
    {
        auto resolution = resolveAddress(address, lazyTemp<Memory>());
        m_assembler.lhuInsn(dest, resolution.base, Imm::I(resolution.offset));
    }

    void load16(const void* address, RegisterID dest)
    {
        auto temp = temps<Memory>();
        loadImmediate(TrustedImmPtr(address), temp.memory());
        m_assembler.lhuInsn(dest, temp.memory(), Imm::I<0>());
    }

    void load16(ExtendedAddress address, RegisterID dest)
    {
        auto temp = temps<Data, Memory>();
        loadImmediate(TrustedImm64(int64_t(address.offset)), temp.memory());
        m_assembler.slliInsn<1>(temp.data(), address.base);
        m_assembler.addInsn(temp.memory(), temp.memory(), temp.data());
        m_assembler.lhuInsn(dest, temp.memory(), Imm::I<0>());
    }

    void load16Unaligned(Address address, RegisterID dest)
    {
        load16(address, dest);
    }

    void load16Unaligned(BaseIndex address, RegisterID dest)
    {
        load16(address, dest);
    }

    void load16SignedExtendTo32(Address address, RegisterID dest)
    {
        auto resolution = resolveAddress(address, lazyTemp<Memory>());
        m_assembler.lhInsn(dest, resolution.base, Imm::I(resolution.offset));
        m_assembler.maskRegister<32>(dest);
    }

    void load16SignedExtendTo32(BaseIndex address, RegisterID dest)
    {
        auto resolution = resolveAddress(address, lazyTemp<Memory>());
        m_assembler.lhInsn(dest, resolution.base, Imm::I(resolution.offset));
        m_assembler.maskRegister<32>(dest);
    }

    void load16SignedExtendTo32(const void* address, RegisterID dest)
    {
        auto temp = temps<Memory>();
        loadImmediate(TrustedImmPtr(address), temp.memory());
        m_assembler.lhInsn(dest, temp.memory(), Imm::I<0>());
        m_assembler.maskRegister<32>(dest);
    }

    void load32(Address address, RegisterID dest)
    {
        auto resolution = resolveAddress(address, lazyTemp<Memory>());
        m_assembler.lwuInsn(dest, resolution.base, Imm::I(resolution.offset));
    }

    void load32(BaseIndex address, RegisterID dest)
    {
        auto resolution = resolveAddress(address, lazyTemp<Memory>());
        m_assembler.lwuInsn(dest, resolution.base, Imm::I(resolution.offset));
    }

    void load32(const void* address, RegisterID dest)
    {
        auto temp = temps<Memory>();
        loadImmediate(TrustedImmPtr(address), temp.memory());
        m_assembler.lwuInsn(dest, temp.memory(), Imm::I<0>());
    }

    void load32WithUnalignedHalfWords(BaseIndex address, RegisterID dest)
    {
        load32(address, dest);
    }

    void load64(Address address, RegisterID dest)
    {
        auto resolution = resolveAddress(address, lazyTemp<Memory>());
        m_assembler.ldInsn(dest, resolution.base, Imm::I(resolution.offset));
    }

    void load64(BaseIndex address, RegisterID dest)
    {
        auto resolution = resolveAddress(address, lazyTemp<Memory>());
        m_assembler.ldInsn(dest, resolution.base, Imm::I(resolution.offset));
    }

    void load64(const void* address, RegisterID dest)
    {
        auto temp = temps<Memory>();
        loadImmediate(TrustedImmPtr(address), temp.memory());
        m_assembler.ldInsn(dest, temp.memory(), Imm::I<0>());
    }

    void loadPair32(RegisterID src, RegisterID dest1, RegisterID dest2)
    {
        loadPair32(src, TrustedImm32(0), dest1, dest2);
    }

    void loadPair32(RegisterID src, TrustedImm32 offset, RegisterID dest1, RegisterID dest2)
    {
        ASSERT(dest1 != dest2);
        if (src == dest1) {
            load32(Address(src, offset.m_value + 4), dest2);
            load32(Address(src, offset.m_value), dest1);
        } else {
            load32(Address(src, offset.m_value), dest1);
            load32(Address(src, offset.m_value + 4), dest2);
        }
    }

    void store8(RegisterID src, Address address)
    {
        auto resolution = resolveAddress(address, lazyTemp<Memory>());
        m_assembler.sbInsn(resolution.base, src, Imm::S(resolution.offset));
    }

    void store8(RegisterID src, BaseIndex address)
    {
        auto resolution = resolveAddress(address, lazyTemp<Memory>());
        m_assembler.sbInsn(resolution.base, src, Imm::S(resolution.offset));
    }

    void store8(RegisterID src, const void* address)
    {
        auto temp = temps<Memory>();
        loadImmediate(TrustedImmPtr(address), temp.memory());
        m_assembler.sbInsn(temp.memory(), src, Imm::S<0>());
    }

    void store8(TrustedImm32 imm, Address address)
    {
        auto temp = temps<Data, Memory>();
        RegisterID immRegister = RISCV64Registers::zero;
        TrustedImm32 imm8(int8_t(imm.m_value));
        if (!!imm8.m_value) {
            loadImmediate(imm8, temp.data());
            immRegister = temp.data();
        }

        auto resolution = resolveAddress(address, temp.memory());
        m_assembler.sbInsn(resolution.base, immRegister, Imm::S(resolution.offset));
    }

    void store8(TrustedImm32 imm, BaseIndex address)
    {
        auto temp = temps<Data, Memory>();
        RegisterID immRegister = RISCV64Registers::zero;
        TrustedImm32 imm8(int8_t(imm.m_value));
        if (!!imm8.m_value) {
            loadImmediate(imm8, temp.data());
            immRegister = temp.data();
        }

        auto resolution = resolveAddress(address, temp.memory());
        m_assembler.sbInsn(resolution.base, immRegister, Imm::S(resolution.offset));
    }

    void store8(TrustedImm32 imm, const void* address)
    {
        auto temp = temps<Memory, Data>();
        RegisterID immRegister = RISCV64Registers::zero;
        TrustedImm32 imm8(int8_t(imm.m_value));
        if (!!imm8.m_value) {
            loadImmediate(imm8, temp.data());
            immRegister = temp.data();
        }

        loadImmediate(TrustedImmPtr(address), temp.memory());
        m_assembler.sbInsn(temp.memory(), immRegister, Imm::S<0>());
    }

    void store16(RegisterID src, Address address)
    {
        auto resolution = resolveAddress(address, lazyTemp<Memory>());
        m_assembler.shInsn(resolution.base, src, Imm::S(resolution.offset));
    }

    void store16(RegisterID src, BaseIndex address)
    {
        auto resolution = resolveAddress(address, lazyTemp<Memory>());
        m_assembler.shInsn(resolution.base, src, Imm::S(resolution.offset));
    }

    void store16(RegisterID src, const void* address)
    {
        auto temp = temps<Memory>();
        loadImmediate(TrustedImmPtr(address), temp.memory());
        m_assembler.shInsn(temp.memory(), src, Imm::S<0>());
    }

    void store16(TrustedImm32 imm, Address address)
    {
        auto temp = temps<Data, Memory>();
        RegisterID immRegister = RISCV64Registers::zero;
        TrustedImm32 imm16(int16_t(imm.m_value));
        if (!!imm16.m_value) {
            loadImmediate(imm16, temp.data());
            immRegister = temp.data();
        }

        auto resolution = resolveAddress(address, temp.memory());
        m_assembler.shInsn(resolution.base, immRegister, Imm::S(resolution.offset));
    }

    void store16(TrustedImm32 imm, BaseIndex address)
    {
        auto temp = temps<Data, Memory>();
        RegisterID immRegister = RISCV64Registers::zero;
        TrustedImm32 imm16(int16_t(imm.m_value));
        if (!!imm16.m_value) {
            loadImmediate(imm16, temp.data());
            immRegister = temp.data();
        }

        auto resolution = resolveAddress(address, lazyTemp<Memory>());
        m_assembler.shInsn(resolution.base, immRegister, Imm::S(resolution.offset));
    }

    void store16(TrustedImm32 imm, const void* address)
    {
        auto temp = temps<Data, Memory>();
        RegisterID immRegister = RISCV64Registers::zero;
        TrustedImm32 imm16(int16_t(imm.m_value));
        if (!!imm16.m_value) {
            loadImmediate(imm16, temp.data());
            immRegister = temp.data();
        }

        loadImmediate(TrustedImmPtr(address), temp.memory());
        m_assembler.shInsn(temp.memory(), immRegister, Imm::S<0>());
    }

    void store32(RegisterID src, Address address)
    {
        auto resolution = resolveAddress(address, lazyTemp<Memory>());
        m_assembler.swInsn(resolution.base, src, Imm::S(resolution.offset));
    }

    void store32(RegisterID src, BaseIndex address)
    {
        auto resolution = resolveAddress(address, lazyTemp<Memory>());
        m_assembler.swInsn(resolution.base, src, Imm::S(resolution.offset));
    }

    void store32(RegisterID src, const void* address)
    {
        auto temp = temps<Memory>();
        loadImmediate(TrustedImmPtr(address), temp.memory());
        m_assembler.swInsn(temp.memory(), src, Imm::S<0>());
    }

    void store32(TrustedImm32 imm, Address address)
    {
        auto temp = temps<Data, Memory>();
        RegisterID immRegister = RISCV64Registers::zero;
        if (!!imm.m_value) {
            loadImmediate(imm, temp.data());
            immRegister = temp.data();
        }

        auto resolution = resolveAddress(address, temp.memory());
        m_assembler.swInsn(resolution.base, immRegister, Imm::S(resolution.offset));
    }

    void store32(TrustedImm32 imm, BaseIndex address)
    {
        auto temp = temps<Data, Memory>();
        RegisterID immRegister = RISCV64Registers::zero;
        if (!!imm.m_value) {
            loadImmediate(imm, temp.data());
            immRegister = temp.data();
        }

        auto resolution = resolveAddress(address, lazyTemp<Memory>());
        m_assembler.swInsn(resolution.base, immRegister, Imm::S(resolution.offset));
    }

    void store32(TrustedImm32 imm, const void* address)
    {
        auto temp = temps<Data, Memory>();
        RegisterID immRegister = RISCV64Registers::zero;
        if (!!imm.m_value) {
            loadImmediate(imm, temp.data());
            immRegister = temp.data();
        }

        loadImmediate(TrustedImmPtr(address), temp.memory());
        m_assembler.swInsn(temp.memory(), immRegister, Imm::S<0>());
    }

    void store64(RegisterID src, Address address)
    {
        auto resolution = resolveAddress(address, lazyTemp<Memory>());
        m_assembler.sdInsn(resolution.base, src, Imm::S(resolution.offset));
    }

    void store64(RegisterID src, BaseIndex address)
    {
        auto resolution = resolveAddress(address, lazyTemp<Memory>());
        m_assembler.sdInsn(resolution.base, src, Imm::S(resolution.offset));
    }

    void store64(RegisterID src, const void* address)
    {
        auto temp = temps<Memory>();
        loadImmediate(TrustedImmPtr(address), temp.memory());
        m_assembler.sdInsn(temp.memory(), src, Imm::S<0>());
    }

    void store64(TrustedImm32 imm, Address address)
    {
        auto temp = temps<Data, Memory>();
        RegisterID immRegister = RISCV64Registers::zero;
        if (!!imm.m_value) {
            loadImmediate(imm, temp.data());
            m_assembler.maskRegister<32>(temp.data());
            immRegister = temp.data();
        }

        auto resolution = resolveAddress(address, temp.memory());
        m_assembler.sdInsn(resolution.base, immRegister, Imm::S(resolution.offset));
    }

    void store64(TrustedImm64 imm, Address address)
    {
        auto temp = temps<Data, Memory>();
        RegisterID immRegister = RISCV64Registers::zero;
        if (!!imm.m_value) {
            loadImmediate(imm, temp.data());
            immRegister = temp.data();
        }

        auto resolution = resolveAddress(address, temp.memory());
        m_assembler.sdInsn(resolution.base, immRegister, Imm::S(resolution.offset));
    }

    void store64(TrustedImm64 imm, BaseIndex address)
    {
        auto temp = temps<Data, Memory>();
        RegisterID immRegister = RISCV64Registers::zero;
        if (!!imm.m_value) {
            loadImmediate(imm, temp.data());
            immRegister = temp.data();
        }

        auto resolution = resolveAddress(address, temp.memory());
        m_assembler.sdInsn(resolution.base, immRegister, Imm::S(resolution.offset));
    }

    void store64(TrustedImm64 imm, const void* address)
    {
        auto temp = temps<Data, Memory>();
        RegisterID immRegister = RISCV64Registers::zero;
        if (!!imm.m_value) {
            loadImmediate(imm, temp.data());
            immRegister = temp.data();
        }

        loadImmediate(TrustedImmPtr(address), temp.memory());
        m_assembler.sdInsn(temp.memory(), immRegister, Imm::S<0>());
    }

    void storePair32(RegisterID src1, RegisterID src2, RegisterID dest)
    {
        storePair32(src1, src2, dest, TrustedImm32(0));
    }

    void storePair32(RegisterID src1, RegisterID src2, RegisterID dest, TrustedImm32 offset)
    {
        store32(src1, Address(dest, offset.m_value));
        store32(src2, Address(dest, offset.m_value + 4));
    }

    void zeroExtend8To32(RegisterID src, RegisterID dest)
    {
        m_assembler.slliInsn<56>(dest, src);
        m_assembler.srliInsn<56>(dest, dest);
    }

    void zeroExtend16To32(RegisterID src, RegisterID dest)
    {
        m_assembler.slliInsn<48>(dest, src);
        m_assembler.srliInsn<48>(dest, dest);
    }

    void zeroExtend32ToWord(RegisterID src, RegisterID dest)
    {
        m_assembler.slliInsn<32>(dest, src);
        m_assembler.srliInsn<32>(dest, dest);
    }

    void signExtend8To32(RegisterID src, RegisterID dest)
    {
        m_assembler.slliInsn<56>(dest, src);
        m_assembler.sraiInsn<24>(dest, dest);
        m_assembler.srliInsn<32>(dest, dest);
    }

    void signExtend16To32(RegisterID src, RegisterID dest)
    {
        m_assembler.slliInsn<48>(dest, src);
        m_assembler.sraiInsn<16>(dest, dest);
        m_assembler.srliInsn<32>(dest, dest);
    }

    void signExtend32ToPtr(RegisterID src, RegisterID dest)
    {
        m_assembler.addiwInsn(dest, src, Imm::I<0>());
    }

    void signExtend32ToPtr(TrustedImm32 imm, RegisterID dest)
    {
        loadImmediate(imm, dest);
    }

    void and32(RegisterID src, RegisterID dest)
    {
        and32(src, dest, dest);
    }

    void and32(RegisterID op1, RegisterID op2, RegisterID dest)
    {
        m_assembler.andInsn(dest, op1, op2);
        m_assembler.maskRegister<32>(dest);
    }

    void and32(TrustedImm32 imm, RegisterID dest)
    {
        and32(imm, dest, dest);
    }

    void and32(TrustedImm32 imm, RegisterID op2, RegisterID dest)
    {
        if (!Imm::isValid<Imm::IType>(imm.m_value)) {
            auto temp = temps<Data>();
            loadImmediate(imm, temp.data());
            m_assembler.andInsn(dest, temp.data(), op2);
        } else
            m_assembler.andiInsn(dest, op2, Imm::I(imm.m_value));
        m_assembler.maskRegister<32>(dest);
    }

    void and32(Address address, RegisterID dest)
    {
        auto temp = temps<Data, Memory>();
        auto resolution = resolveAddress(address, temp.memory());
        m_assembler.lwInsn(temp.data(), resolution.base, Imm::I(resolution.offset));
        m_assembler.andInsn(dest, temp.data(), dest);
        m_assembler.maskRegister<32>(dest);
    }

    void and64(RegisterID src, RegisterID dest)
    {
        and64(src, dest, dest);
    }

    void and64(RegisterID op1, RegisterID op2, RegisterID dest)
    {
        m_assembler.andInsn(dest, op1, op2);
    }

    void and64(TrustedImm32 imm, RegisterID dest)
    {
        and64(imm, dest, dest);
    }

    void and64(TrustedImm32 imm, RegisterID op2, RegisterID dest)
    {
        if (Imm::isValid<Imm::IType>(imm.m_value)) {
            m_assembler.andiInsn(dest, op2, Imm::I(imm.m_value));
            return;
        }

        auto temp = temps<Data>();
        loadImmediate(imm, temp.data());
        m_assembler.andInsn(dest, temp.data(), op2);
    }

    void and64(TrustedImm64 imm, RegisterID dest)
    {
        and64(imm, dest, dest);
    }

    void and64(TrustedImm64 imm, RegisterID op2, RegisterID dest)
    {
        if (Imm::isValid<Imm::IType>(imm.m_value)) {
            m_assembler.andiInsn(dest, op2, Imm::I(imm.m_value));
            return;
        }

        auto temp = temps<Data>();
        loadImmediate(imm, temp.data());
        m_assembler.andInsn(dest, temp.data(), op2);
    }

    void and64(TrustedImmPtr imm, RegisterID dest)
    {
        intptr_t value = imm.asIntptr();
        if constexpr (sizeof(intptr_t) == sizeof(int64_t))
            and64(TrustedImm64(int64_t(value)), dest);
        else
            and64(TrustedImm32(int32_t(value)), dest);
    }

    void or8(RegisterID src, AbsoluteAddress address)
    {
        auto temp = temps<Data, Memory>();
        loadImmediate(TrustedImmPtr(address.m_ptr), temp.memory());
        m_assembler.lbInsn(temp.data(), temp.memory(), Imm::I<0>());
        m_assembler.orInsn(temp.data(), src, temp.data());
        m_assembler.sbInsn(temp.memory(), temp.data(), Imm::S<0>());
    }

    void or8(TrustedImm32 imm, AbsoluteAddress address)
    {
        auto temp = temps<Data, Memory>();
        loadImmediate(TrustedImmPtr(address.m_ptr), temp.memory());
        m_assembler.lbInsn(temp.data(), temp.memory(), Imm::I<0>());

        if (Imm::isValid<Imm::IType>(imm.m_value)) {
            m_assembler.oriInsn(temp.data(), temp.data(), Imm::I(imm.m_value));
            m_assembler.sbInsn(temp.memory(), temp.data(), Imm::S<0>());
        } else {
            loadImmediate(imm, temp.memory());
            m_assembler.orInsn(temp.data(), temp.data(), temp.memory());
            loadImmediate(TrustedImmPtr(address.m_ptr), temp.memory());
            m_assembler.sbInsn(temp.memory(), temp.data(), Imm::S<0>());
        }
    }

    void or16(RegisterID src, AbsoluteAddress address)
    {
        auto temp = temps<Data, Memory>();
        loadImmediate(TrustedImmPtr(address.m_ptr), temp.memory());
        m_assembler.lhInsn(temp.data(), temp.memory(), Imm::I<0>());
        m_assembler.orInsn(temp.data(), src, temp.data());
        m_assembler.shInsn(temp.memory(), temp.data(), Imm::S<0>());
    }

    void or16(TrustedImm32 imm, AbsoluteAddress address)
    {
        auto temp = temps<Data, Memory>();
        loadImmediate(TrustedImmPtr(address.m_ptr), temp.memory());
        m_assembler.lhInsn(temp.data(), temp.memory(), Imm::I<0>());

        if (Imm::isValid<Imm::IType>(imm.m_value)) {
            m_assembler.oriInsn(temp.data(), temp.data(), Imm::I(imm.m_value));
            m_assembler.shInsn(temp.memory(), temp.data(), Imm::S<0>());
        } else {
            loadImmediate(imm, temp.memory());
            m_assembler.orInsn(temp.data(), temp.data(), temp.memory());
            loadImmediate(TrustedImmPtr(address.m_ptr), temp.memory());
            m_assembler.shInsn(temp.memory(), temp.data(), Imm::S<0>());
        }
    }

    void or32(RegisterID src, RegisterID dest)
    {
        or32(src, dest, dest);
    }

    void or32(RegisterID op1, RegisterID op2, RegisterID dest)
    {
        m_assembler.orInsn(dest, op1, op2);
        m_assembler.maskRegister<32>(dest);
    }

    void or32(TrustedImm32 imm, RegisterID dest)
    {
        or32(imm, dest, dest);
    }

    void or32(TrustedImm32 imm, RegisterID op2, RegisterID dest)
    {
        if (Imm::isValid<Imm::IType>(imm.m_value)) {
            m_assembler.oriInsn(dest, op2, Imm::I(imm.m_value));
            m_assembler.maskRegister<32>(dest);
            return;
        }

        auto temp = temps<Data>();
        loadImmediate(imm, temp.data());
        m_assembler.orInsn(dest, temp.data(), op2);
        m_assembler.maskRegister<32>(dest);
    }

    void or32(RegisterID src, AbsoluteAddress address)
    {
        auto temp = temps<Data, Memory>();
        loadImmediate(TrustedImmPtr(address.m_ptr), temp.memory());
        m_assembler.lwInsn(temp.data(), temp.memory(), Imm::I<0>());
        m_assembler.orInsn(temp.data(), src, temp.data());
        m_assembler.swInsn(temp.memory(), temp.data(), Imm::S<0>());
    }

    void or32(TrustedImm32 imm, AbsoluteAddress address)
    {
        auto temp = temps<Data, Memory>();
        loadImmediate(TrustedImmPtr(address.m_ptr), temp.memory());
        m_assembler.lwInsn(temp.data(), temp.memory(), Imm::I<0>());

        if (Imm::isValid<Imm::IType>(imm.m_value)) {
            m_assembler.oriInsn(temp.data(), temp.data(), Imm::I(imm.m_value));
            m_assembler.swInsn(temp.memory(), temp.data(), Imm::S<0>());
        } else {
            loadImmediate(imm, temp.memory());
            m_assembler.orInsn(temp.data(), temp.data(), temp.memory());
            loadImmediate(TrustedImmPtr(address.m_ptr), temp.memory());
            m_assembler.swInsn(temp.memory(), temp.data(), Imm::S<0>());
        }
    }

    void or32(TrustedImm32 imm, Address address)
    {
        auto temp = temps<Data, Memory>();
        auto resolution = resolveAddress(address, temp.memory());
        m_assembler.lwInsn(temp.data(), resolution.base, Imm::I(resolution.offset));

        if (Imm::isValid<Imm::IType>(imm.m_value)) {
            m_assembler.oriInsn(temp.data(), temp.memory(), Imm::I(imm.m_value));
            m_assembler.swInsn(resolution.base, temp.data(), Imm::S(resolution.offset));
        } else {
            loadImmediate(imm, temp.memory());
            m_assembler.orInsn(temp.data(), temp.data(), temp.memory());
            resolution = resolveAddress(address, temp.memory());
            m_assembler.swInsn(resolution.base, temp.data(), Imm::S(resolution.offset));
        }
    }

    void or64(RegisterID src, RegisterID dest)
    {
        or64(src, dest, dest);
    }

    void or64(RegisterID op1, RegisterID op2, RegisterID dest)
    {
        m_assembler.orInsn(dest, op1, op2);
    }

    void or64(TrustedImm32 imm, RegisterID dest)
    {
        or64(imm, dest, dest);
    }

    void or64(TrustedImm32 imm, RegisterID op2, RegisterID dest)
    {
        if (Imm::isValid<Imm::IType>(imm.m_value)) {
            m_assembler.oriInsn(dest, op2, Imm::I(imm.m_value));
            return;
        }

        auto temp = temps<Data>();
        loadImmediate(imm, temp.data());
        m_assembler.orInsn(dest, temp.data(), op2);
    }

    void or64(TrustedImm64 imm, RegisterID dest)
    {
        or64(imm, dest, dest);
    }

    void or64(TrustedImm64 imm, RegisterID op2, RegisterID dest)
    {
        if (Imm::isValid<Imm::IType>(imm.m_value)) {
            m_assembler.oriInsn(dest, op2, Imm::I(imm.m_value));
            return;
        }

        auto temp = temps<Data>();
        loadImmediate(imm, temp.data());
        m_assembler.orInsn(dest, temp.data(), op2);
    }


    void xor32(RegisterID src, RegisterID dest)
    {
        xor32(src, dest, dest);
    }

    void xor32(RegisterID op1, RegisterID op2, RegisterID dest)
    {
        m_assembler.xorInsn(dest, op1, op2);
        m_assembler.maskRegister<32>(dest);
    }

    void xor32(TrustedImm32 imm, RegisterID dest)
    {
        xor32(imm, dest, dest);
    }

    void xor32(TrustedImm32 imm, RegisterID op2, RegisterID dest)
    {
        if (Imm::isValid<Imm::IType>(imm.m_value)) {
            m_assembler.xoriInsn(dest, op2, Imm::I(imm.m_value));
            m_assembler.maskRegister<32>(dest);
            return;
        }

        auto temp = temps<Data>();
        loadImmediate(imm, temp.data());
        m_assembler.xorInsn(dest, temp.data(), op2);
        m_assembler.maskRegister<32>(dest);
    }

    void xor32(Address address, RegisterID dest)
    {
        auto temp = temps<Data, Memory>();
        auto resolution = resolveAddress(address, temp.memory());
        m_assembler.lwInsn(temp.data(), resolution.base, Imm::I(resolution.offset));
        m_assembler.xorInsn(dest, temp.data(), dest);
        m_assembler.maskRegister<32>(dest);
    }

    void xor64(RegisterID src, RegisterID dest)
    {
        xor64(src, dest, dest);
    }

    void xor64(RegisterID op1, RegisterID op2, RegisterID dest)
    {
        m_assembler.xorInsn(dest, op1, op2);
    }

    void xor64(TrustedImm32 imm, RegisterID dest)
    {
        xor64(imm, dest, dest);
    }

    void xor64(TrustedImm32 imm, RegisterID op2, RegisterID dest)
    {
        if (Imm::isValid<Imm::IType>(imm.m_value)) {
            m_assembler.xoriInsn(dest, op2, Imm::I(imm.m_value));
            return;
        }

        auto temp = temps<Data>();
        loadImmediate(imm, temp.data());
        m_assembler.xorInsn(dest, temp.data(), op2);
    }

    void xor64(TrustedImm64 imm, RegisterID dest)
    {
        xor64(imm, dest, dest);
    }

    void xor64(TrustedImm64 imm, RegisterID op2, RegisterID dest)
    {
        if (Imm::isValid<Imm::IType>(imm.m_value)) {
            m_assembler.xoriInsn(dest, op2, Imm::I(imm.m_value));
            return;
        }

        auto temp = temps<Data>();
        loadImmediate(imm, temp.data());
        m_assembler.xorInsn(dest, temp.data(), op2);
    }

    void xor64(Address address, RegisterID dest)
    {
        auto temp = temps<Data, Memory>();
        auto resolution = resolveAddress(address, temp.memory());
        m_assembler.ldInsn(temp.data(), resolution.base, Imm::I(resolution.offset));
        m_assembler.xorInsn(dest, temp.data(), dest);
    }

    void xor64(RegisterID src, Address address)
    {
        auto temp = temps<Data, Memory>();
        auto resolution = resolveAddress(address, temp.memory());
        m_assembler.ldInsn(temp.data(), resolution.base, Imm::I(resolution.offset));
        m_assembler.xorInsn(temp.data(), src, temp.data());
        m_assembler.sdInsn(resolution.base, temp.data(), Imm::S(resolution.offset));
    }

    void not32(RegisterID dest)
    {
        not32(dest, dest);
    }

    void not32(RegisterID src, RegisterID dest)
    {
        m_assembler.xoriInsn(dest, src, Imm::I<-1>());
        m_assembler.maskRegister<32>(dest);
    }

    void not64(RegisterID dest)
    {
        not64(dest, dest);
    }

    void not64(RegisterID src, RegisterID dest)
    {
        m_assembler.xoriInsn(dest, src, Imm::I<-1>());
    }

    void neg32(RegisterID dest)
    {
        neg32(dest, dest);
    }

    void neg32(RegisterID src, RegisterID dest)
    {
        m_assembler.subwInsn(dest, RISCV64Registers::zero, src);
        m_assembler.maskRegister<32>(dest);
    }

    void neg64(RegisterID dest)
    {
        neg64(dest, dest);
    }

    void neg64(RegisterID src, RegisterID dest)
    {
        m_assembler.subInsn(dest, RISCV64Registers::zero, src);
    }

    void move(RegisterID src, RegisterID dest)
    {
        m_assembler.addiInsn(dest, src, Imm::I<0>());
    }

    void move(TrustedImm32 imm, RegisterID dest)
    {
        loadImmediate(imm, dest);
        m_assembler.maskRegister<32>(dest);
    }

    void move(TrustedImm64 imm, RegisterID dest)
    {
        loadImmediate(imm, dest);
    }

    void move(TrustedImmPtr imm, RegisterID dest)
    {
        loadImmediate(imm, dest);
    }

    void swap(RegisterID reg1, RegisterID reg2)
    {
        auto temp = temps<Data>();
        move(reg1, temp.data());
        move(reg2, reg1);
        move(temp.data(), reg2);
    }

    void swap(FPRegisterID reg1, FPRegisterID reg2)
    {
        moveDouble(reg1, fpTempRegister);
        moveDouble(reg2, reg1);
        moveDouble(fpTempRegister, reg2);
    }

    void moveZeroToFloat(FPRegisterID dest)
    {
        m_assembler.fcvtInsn<RISCV64Assembler::FCVTType::S, RISCV64Assembler::FCVTType::W>(dest, RISCV64Registers::zero);
    }

    void moveZeroToDouble(FPRegisterID dest)
    {
        m_assembler.fcvtInsn<RISCV64Assembler::FCVTType::D, RISCV64Assembler::FCVTType::L>(dest, RISCV64Registers::zero);
    }

    void moveFloat(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.fsgnjInsn<32>(dest, src, src);
    }

    void moveFloatTo32(FPRegisterID src, RegisterID dest)
    {
        m_assembler.fmvInsn<RISCV64Assembler::FMVType::X, RISCV64Assembler::FMVType::W>(dest, src);
    }

    void move32ToFloat(RegisterID src, FPRegisterID dest)
    {
        m_assembler.fmvInsn<RISCV64Assembler::FMVType::W, RISCV64Assembler::FMVType::X>(dest, src);
    }

    void moveDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.fsgnjInsn<64>(dest, src, src);
    }

    void moveDoubleTo64(FPRegisterID src, RegisterID dest)
    {
        m_assembler.fmvInsn<RISCV64Assembler::FMVType::X, RISCV64Assembler::FMVType::D>(dest, src);
    }

    void move64ToDouble(RegisterID src, FPRegisterID dest)
    {
        m_assembler.fmvInsn<RISCV64Assembler::FMVType::D, RISCV64Assembler::FMVType::X>(dest, src);
    }

    static bool supportsCountPopulation() { return false; }
    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD(countPopulation32);
    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD(countPopulation64);

    template<PtrTag resultTag, PtrTag locationTag>
    static FunctionPtr<resultTag> readCallTarget(CodeLocationCall<locationTag> call)
    {
        return FunctionPtr<resultTag>(MacroAssemblerCodePtr<resultTag>(Assembler::readCallTarget(call.dataLocation())));
    }

    template<PtrTag tag>
    static void replaceWithVMHalt(CodeLocationLabel<tag> instructionStart)
    {
        Assembler::replaceWithVMHalt(instructionStart.dataLocation());
    }

    template<PtrTag startTag, PtrTag destTag>
    static void replaceWithJump(CodeLocationLabel<startTag> instructionStart, CodeLocationLabel<destTag> destination)
    {
        Assembler::replaceWithJump(instructionStart.dataLocation(), destination.dataLocation());
    }

    static ptrdiff_t maxJumpReplacementSize()
    {
        return Assembler::maxJumpReplacementSize();
    }

    static ptrdiff_t patchableJumpSize()
    {
        return Assembler::patchableJumpSize();
    }

    template<PtrTag tag>
    static CodeLocationLabel<tag> startOfBranchPtrWithPatchOnRegister(CodeLocationDataLabelPtr<tag> label)
    {
        return label.labelAtOffset(0);
    }

    template<PtrTag tag>
    static void revertJumpReplacementToBranchPtrWithPatch(CodeLocationLabel<tag> jump, RegisterID, void* initialValue)
    {
        Assembler::revertJumpReplacementToPatch(jump.dataLocation(), initialValue);
    }

    template<PtrTag tag>
    static void linkCall(void* code, Call call, FunctionPtr<tag> function)
    {
        if (!call.isFlagSet(Call::Near))
            Assembler::linkPointer(code, call.m_label, function.executableAddress());
        else
            Assembler::linkCall(code, call.m_label, function.template retaggedExecutableAddress<NoPtrTag>());
    }

    template<PtrTag callTag, PtrTag destTag>
    static void repatchCall(CodeLocationCall<callTag> call, CodeLocationLabel<destTag> destination)
    {
        Assembler::repatchPointer(call.dataLocation(), destination.executableAddress());
    }

    template<PtrTag callTag, PtrTag destTag>
    static void repatchCall(CodeLocationCall<callTag> call, FunctionPtr<destTag> destination)
    {
        Assembler::repatchPointer(call.dataLocation(), destination.executableAddress());
    }

    Jump jump()
    {
        auto label = m_assembler.label();
        m_assembler.jumpPlaceholder(
            [&] {
                m_assembler.jalInsn(RISCV64Registers::zero, Imm::J<0>());
            });
        return Jump(label);
    }

    void farJump(RegisterID target, PtrTag)
    {
        m_assembler.jalrInsn(RISCV64Registers::zero, target, Imm::I<0>());
    }

    void farJump(AbsoluteAddress address, PtrTag)
    {
        auto temp = temps<Data>();
        loadImmediate(TrustedImmPtr(address.m_ptr), temp.data());
        m_assembler.ldInsn(temp.data(), temp.data(), Imm::I<0>());
        m_assembler.jalrInsn(RISCV64Registers::zero, temp.data(), Imm::I<0>());
    }

    void farJump(Address address, PtrTag)
    {
        auto temp = temps<Data, Memory>();
        auto resolution = resolveAddress(address, temp.memory());
        m_assembler.ldInsn(temp.data(), resolution.base, Imm::I(resolution.offset));
        m_assembler.jalrInsn(RISCV64Registers::zero, temp.data(), Imm::I<0>());
    }

    void farJump(BaseIndex address, PtrTag)
    {
        auto temp = temps<Data, Memory>();
        auto resolution = resolveAddress(address, temp.memory());
        m_assembler.ldInsn(temp.data(), resolution.base, Imm::I(resolution.offset));
        m_assembler.jalrInsn(RISCV64Registers::zero, temp.data(), Imm::I<0>());
    }

    void farJump(TrustedImmPtr imm, PtrTag)
    {
        auto temp = temps<Data>();
        loadImmediate(imm, temp.data());
        m_assembler.jalrInsn(RISCV64Registers::zero, temp.data(), Imm::I<0>());
    }

    void farJump(RegisterID target, RegisterID jumpTag)
    {
        UNUSED_PARAM(jumpTag);
        farJump(target, NoPtrTag);
    }

    void farJump(AbsoluteAddress address, RegisterID jumpTag)
    {
        UNUSED_PARAM(jumpTag);
        farJump(address, NoPtrTag);
    }

    void farJump(Address address, RegisterID jumpTag)
    {
        UNUSED_PARAM(jumpTag);
        farJump(address, NoPtrTag);
    }

    void farJump(BaseIndex address, RegisterID jumpTag)
    {
        UNUSED_PARAM(jumpTag);
        farJump(address, NoPtrTag);
    }

    Call nearCall()
    {
        auto label = m_assembler.label();
        m_assembler.nearCallPlaceholder(
            [&] {
                m_assembler.jalInsn(RISCV64Registers::x1, Imm::J<0>());
            });
        return Call(label, Call::LinkableNear);
    }

    Call nearTailCall()
    {
        auto label = m_assembler.label();
        m_assembler.nearCallPlaceholder(
            [&] {
                m_assembler.jalInsn(RISCV64Registers::zero, Imm::J<0>());
            });
        return Call(label, Call::LinkableNearTail);
    }

    Call threadSafePatchableNearCall()
    {
        auto label = m_assembler.label();
        m_assembler.nearCallPlaceholder(
            [&] {
                m_assembler.jalInsn(RISCV64Registers::x1, Imm::J<0>());
            });
        return Call(label, Call::LinkableNear);
    }

    void ret()
    {
        m_assembler.jalrInsn(RISCV64Registers::zero, RISCV64Registers::x1, Imm::I<0>());
    }

    void compare8(RelationalCondition cond, Address address, TrustedImm32 imm, RegisterID dest)
    {
        auto temp = temps<Data, Memory>();
        auto resolution = resolveAddress(address, temp.memory());
        m_assembler.lbInsn(temp.memory(), resolution.base, Imm::I(resolution.offset));
        loadImmediate(imm, temp.data());
        compareFinalize(cond, temp.memory(), temp.data(), dest);
    }

    void compare32(RelationalCondition cond, RegisterID lhs, RegisterID rhs, RegisterID dest)
    {
        auto temp = temps<Data, Memory>();
        m_assembler.signExtend<32>(temp.memory(), lhs);
        m_assembler.signExtend<32>(temp.data(), rhs);
        compareFinalize(cond, temp.memory(), temp.data(), dest);
    }

    void compare32(RelationalCondition cond, RegisterID lhs, TrustedImm32 imm, RegisterID dest)
    {
        auto temp = temps<Data, Memory>();
        m_assembler.signExtend<32>(temp.memory(), lhs);
        loadImmediate(imm, temp.data());
        compareFinalize(cond, temp.memory(), temp.data(), dest);
    }

    void compare32(RelationalCondition cond, Address address, RegisterID rhs, RegisterID dest)
    {
        auto temp = temps<Data, Memory>();
        auto resolution = resolveAddress(address, temp.memory());
        m_assembler.lwInsn(temp.memory(), resolution.base, Imm::I(resolution.offset));
        m_assembler.signExtend<32>(temp.data(), rhs);
        compareFinalize(cond, temp.memory(), temp.data(), dest);
    }

    void compare64(RelationalCondition cond, RegisterID lhs, RegisterID rhs, RegisterID dest)
    {
        compareFinalize(cond, lhs, rhs, dest);
    }

    void compare64(RelationalCondition cond, RegisterID lhs, TrustedImm32 imm, RegisterID dest)
    {
        auto temp = temps<Data>();
        loadImmediate(imm, temp.data());
        compareFinalize(cond, lhs, temp.data(), dest);
    }

    void test8(ResultCondition cond, Address address, TrustedImm32 imm, RegisterID dest)
    {
        auto temp = temps<Data, Memory>();
        auto resolution = resolveAddress(address, temp.memory());
        m_assembler.lbuInsn(temp.data(), resolution.base, Imm::I(resolution.offset));
        m_assembler.andiInsn(temp.data(), temp.data(), Imm::I(imm.m_value & 0xff));
        testFinalize(cond, temp.data(), dest);
    }

    void test32(ResultCondition cond, RegisterID lhs, RegisterID rhs, RegisterID dest)
    {
        auto temp = temps<Data>();
        m_assembler.andInsn(temp.data(), lhs, rhs);
        m_assembler.maskRegister<32>(temp.data());
        testFinalize(cond, temp.data(), dest);
    }

    void test32(ResultCondition cond, RegisterID lhs, TrustedImm32 imm, RegisterID dest)
    {
        auto temp = temps<Data>();
        if (!Imm::isValid<Imm::IType>(imm.m_value)) {
            loadImmediate(imm, temp.data());
            m_assembler.andInsn(temp.data(), lhs, temp.data());
        } else
            m_assembler.andiInsn(temp.data(), lhs, Imm::I(imm.m_value));
        m_assembler.maskRegister<32>(temp.data());
        testFinalize(cond, temp.data(), dest);
    }

    void test32(ResultCondition cond, Address address, TrustedImm32 imm, RegisterID dest)
    {
        auto temp = temps<Data, Memory>();
        auto resolution = resolveAddress(address, temp.memory());
        m_assembler.lwuInsn(temp.memory(), resolution.base, Imm::I(resolution.offset));

        if (!Imm::isValid<Imm::IType>(imm.m_value)) {
            loadImmediate(imm, temp.data());
            m_assembler.andInsn(temp.data(), temp.memory(), temp.data());
        } else
            m_assembler.andiInsn(temp.data(), temp.memory(), Imm::I(imm.m_value));
        testFinalize(cond, temp.data(), dest);
    }

    void test64(ResultCondition cond, RegisterID lhs, RegisterID rhs, RegisterID dest)
    {
        m_assembler.andInsn(dest, lhs, rhs);
        testFinalize(cond, dest, dest);
    }

    void test64(ResultCondition cond, RegisterID lhs, TrustedImm32 imm, RegisterID dest)
    {
        auto temp = temps<Data>();
        if (!Imm::isValid<Imm::IType>(imm.m_value)) {
            loadImmediate(imm, temp.data());
            m_assembler.andInsn(dest, lhs, temp.data());
        } else
            m_assembler.andiInsn(dest, lhs, Imm::I(imm.m_value));
        testFinalize(cond, dest, dest);
    }

    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD(setCarry);

    Jump branch8(RelationalCondition cond, Address address, TrustedImm32 imm)
    {
        auto temp = temps<Data, Memory>();
        auto resolution = resolveAddress(address, temp.memory());
        m_assembler.lbInsn(temp.memory(), resolution.base, Imm::I(resolution.offset));
        loadImmediate(imm, temp.data());
        return makeBranch(cond, temp.memory(), temp.data());
    }

    Jump branch8(RelationalCondition cond, AbsoluteAddress address, TrustedImm32 imm)
    {
        auto temp = temps<Data, Memory>();
        loadImmediate(TrustedImmPtr(address.m_ptr), temp.memory());
        m_assembler.lbInsn(temp.memory(), temp.memory(), Imm::I<0>());
        loadImmediate(imm, temp.data());
        return makeBranch(cond, temp.memory(), temp.data());
    }

    Jump branch32(RelationalCondition cond, RegisterID lhs, RegisterID rhs)
    {
        auto temp = temps<Data, Memory>();
        m_assembler.signExtend<32>(temp.data(), lhs);
        m_assembler.signExtend<32>(temp.memory(), rhs);
        return makeBranch(cond, temp.data(), temp.memory());
    }

    Jump branch32(RelationalCondition cond, RegisterID lhs, TrustedImm32 imm)
    {
        auto temp = temps<Data, Memory>();
        m_assembler.signExtend<32>(temp.data(), lhs);
        loadImmediate(imm, temp.memory());
        return makeBranch(cond, temp.data(), temp.memory());
    }

    Jump branch32(RelationalCondition cond, RegisterID lhs, Address address)
    {
        auto temp = temps<Data, Memory>();
        auto resolution = resolveAddress(address, temp.memory());
        m_assembler.lwInsn(temp.memory(), resolution.base, Imm::I(resolution.offset));
        m_assembler.signExtend<32>(temp.data(), lhs);
        return makeBranch(cond, temp.data(), temp.memory());
    }

    Jump branch32(RelationalCondition cond, Address address, RegisterID rhs)
    {
        auto temp = temps<Data, Memory>();
        auto resolution = resolveAddress(address, temp.memory());
        m_assembler.lwInsn(temp.memory(), resolution.base, Imm::I(resolution.offset));
        m_assembler.signExtend<32>(temp.data(), rhs);
        return makeBranch(cond, temp.memory(), temp.data());
    }

    Jump branch32(RelationalCondition cond, Address address, TrustedImm32 imm)
    {
        auto temp = temps<Data, Memory>();
        auto resolution = resolveAddress(address, temp.memory());
        m_assembler.lwInsn(temp.memory(), resolution.base, Imm::I(resolution.offset));
        loadImmediate(imm, temp.data());
        return makeBranch(cond, temp.memory(), temp.data());
    }

    Jump branch32(RelationalCondition cond, AbsoluteAddress address, RegisterID rhs)
    {
        auto temp = temps<Data, Memory>();
        loadImmediate(TrustedImmPtr(address.m_ptr), temp.memory());
        m_assembler.lwInsn(temp.memory(), temp.memory(), Imm::I<0>());
        m_assembler.signExtend<32>(temp.data(), rhs);
        return makeBranch(cond, temp.memory(), temp.data());
    }

    Jump branch32(RelationalCondition cond, BaseIndex address, TrustedImm32 imm)
    {
        auto temp = temps<Data, Memory>();
        auto resolution = resolveAddress(address, temp.memory());
        m_assembler.lwInsn(temp.memory(), resolution.base, Imm::I(resolution.offset));
        loadImmediate(imm, temp.data());
        return makeBranch(cond, temp.memory(), temp.data());
    }

    Jump branch64(RelationalCondition cond, RegisterID lhs, RegisterID rhs)
    {
        return makeBranch(cond, lhs, rhs);
    }

    Jump branch64(RelationalCondition cond, RegisterID lhs, TrustedImm32 imm)
    {
        auto temp = temps<Data>();
        loadImmediate(imm, temp.data());
        return makeBranch(cond, lhs, temp.data());
    }

    Jump branch64(RelationalCondition cond, RegisterID lhs, TrustedImm64 imm)
    {
        auto temp = temps<Data>();
        loadImmediate(imm, temp.data());
        return makeBranch(cond, lhs, temp.data());
    }

    Jump branch64(RelationalCondition cond, RegisterID left, Imm64 right)
    {
        return branch64(cond, left, right.asTrustedImm64());
    }

    Jump branch64(RelationalCondition cond, RegisterID lhs, Address address)
    {
        auto temp = temps<Data, Memory>();
        auto resolution = resolveAddress(address, temp.memory());
        m_assembler.ldInsn(temp.data(), resolution.base, Imm::I(resolution.offset));
        return makeBranch(cond, lhs, temp.data());
    }

    Jump branch64(RelationalCondition cond, RegisterID lhs, AbsoluteAddress address)
    {
        auto temp = temps<Data, Memory>();
        loadImmediate(TrustedImmPtr(address.m_ptr), temp.memory());
        m_assembler.ldInsn(temp.data(), temp.memory(), Imm::I<0>());
        return makeBranch(cond, lhs, temp.data());
    }

    Jump branch64(RelationalCondition cond, Address address, RegisterID rhs)
    {
        auto temp = temps<Data, Memory>();
        auto resolution = resolveAddress(address, temp.memory());
        m_assembler.ldInsn(temp.data(), resolution.base, Imm::I(resolution.offset));
        return makeBranch(cond, temp.data(), rhs);
    }

    Jump branch64(RelationalCondition cond, Address address, TrustedImm32 imm)
    {
        auto temp = temps<Data, Memory>();
        auto resolution = resolveAddress(address, temp.memory());
        m_assembler.ldInsn(temp.memory(), resolution.base, Imm::I(resolution.offset));
        loadImmediate(imm, temp.data());
        return makeBranch(cond, temp.memory(), temp.data());
    }

    Jump branch64(RelationalCondition cond, Address address, TrustedImm64 imm)
    {
        auto temp = temps<Data, Memory>();
        auto resolution = resolveAddress(address, temp.memory());
        m_assembler.ldInsn(temp.memory(), resolution.base, Imm::I(resolution.offset));
        loadImmediate(imm, temp.data());
        return makeBranch(cond, temp.memory(), temp.data());
    }

    Jump branch64(RelationalCondition cond, AbsoluteAddress address, RegisterID rhs)
    {
        auto temp = temps<Data, Memory>();
        loadImmediate(TrustedImmPtr(address.m_ptr), temp.memory());
        m_assembler.ldInsn(temp.data(), temp.memory(), Imm::I<0>());
        return makeBranch(cond, temp.data(), rhs);
    }

    Jump branch64(RelationalCondition cond, AbsoluteAddress address, TrustedImm32 imm)
    {
        auto temp = temps<Data, Memory>();
        loadImmediate(TrustedImmPtr(address.m_ptr), temp.memory());
        m_assembler.ldInsn(temp.memory(), temp.memory(), Imm::I<0>());
        loadImmediate(imm, temp.data());
        return makeBranch(cond, temp.memory(), temp.data());
    }

    Jump branch64(RelationalCondition cond, AbsoluteAddress address, TrustedImm64 imm)
    {
        auto temp = temps<Data, Memory>();
        loadImmediate(TrustedImmPtr(address.m_ptr), temp.memory());
        m_assembler.ldInsn(temp.memory(), temp.memory(), Imm::I<0>());
        loadImmediate(imm, temp.data());
        return makeBranch(cond, temp.memory(), temp.data());
    }

    Jump branch64(RelationalCondition cond, BaseIndex address, RegisterID rhs)
    {
        auto temp = temps<Data, Memory>();
        auto resolution = resolveAddress(address, temp.memory());
        m_assembler.ldInsn(temp.data(), resolution.base, Imm::I(resolution.offset));
        return makeBranch(cond, temp.data(), rhs);
    }

    Jump branch32WithUnalignedHalfWords(RelationalCondition cond, BaseIndex address, TrustedImm32 imm)
    {
        return branch32(cond, address, imm);
    }

    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD_WITH_RETURN(branchAdd32, Jump);
    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD_WITH_RETURN(branchAdd64, Jump);

    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD_WITH_RETURN(branchSub32, Jump);
    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD_WITH_RETURN(branchSub64, Jump);

    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD_WITH_RETURN(branchMul32, Jump);
    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD_WITH_RETURN(branchMul64, Jump);

    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD_WITH_RETURN(branchNeg32, Jump);
    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD_WITH_RETURN(branchNeg64, Jump);

    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD_WITH_RETURN(branchTest8, Jump);
    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD_WITH_RETURN(branchTest16, Jump);
    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD_WITH_RETURN(branchTest32, Jump);
    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD_WITH_RETURN(branchTest64, Jump);

    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD_WITH_RETURN(branchPtr, Jump);

    DataLabel32 moveWithPatch(TrustedImm32 imm, RegisterID dest)
    {
        RISCV64Assembler::ImmediateLoader imml(RISCV64Assembler::ImmediateLoader::Placeholder, imm.m_value);

        DataLabel32 label(this);
        imml.moveInto(m_assembler, dest);
        return label;
    }

    DataLabelPtr moveWithPatch(TrustedImmPtr imm, RegisterID dest)
    {
        RISCV64Assembler::ImmediateLoader imml(RISCV64Assembler::ImmediateLoader::Placeholder, int64_t(imm.asIntptr()));

        DataLabelPtr label(this);
        imml.moveInto(m_assembler, dest);
        return label;
    }

    DataLabelPtr storePtrWithPatch(TrustedImmPtr initialValue, Address address)
    {
        auto temp = temps<Data, Memory>();
        RISCV64Assembler::ImmediateLoader imml(RISCV64Assembler::ImmediateLoader::Placeholder, int64_t(initialValue.asIntptr()));
        DataLabelPtr label(this);
        imml.moveInto(m_assembler, temp.data());

        auto resolution = resolveAddress(address, temp.memory());
        m_assembler.sdInsn(resolution.base, temp.data(), Imm::S(resolution.offset));
        return label;
    }

    DataLabelPtr storePtrWithPatch(Address address)
    {
        return storePtrWithPatch(TrustedImmPtr(nullptr), address);
    }

    Jump branch32WithPatch(RelationalCondition cond, Address address, DataLabel32& dataLabel, TrustedImm32 initialRightValue = TrustedImm32(0))
    {
        auto temp = temps<Data, Memory>();
        auto resolution = resolveAddress(address, temp.memory());
        m_assembler.lwInsn(temp.memory(), resolution.base, Imm::I(resolution.offset));

        dataLabel = moveWithPatch(initialRightValue, temp.data());
        return makeBranch(cond, temp.memory(), temp.data());
    }

    Jump branchPtrWithPatch(RelationalCondition cond, Address address, DataLabelPtr& dataLabel, TrustedImmPtr initialRightValue = TrustedImmPtr(nullptr))
    {
        auto temp = temps<Data, Memory>();
        auto resolution = resolveAddress(address, temp.memory());
        m_assembler.ldInsn(temp.memory(), resolution.base, Imm::I(resolution.offset));

        dataLabel = moveWithPatch(initialRightValue, temp.data());
        return makeBranch(cond, temp.memory(), temp.data());
    }

    Jump branchPtrWithPatch(RelationalCondition cond, RegisterID lhs, DataLabelPtr& dataLabel, TrustedImmPtr initialRightValue = TrustedImmPtr(nullptr))
    {
        auto temp = temps<Data>();
        dataLabel = moveWithPatch(initialRightValue, temp.data());
        return makeBranch(cond, lhs, temp.data());
    }

    PatchableJump patchableBranch64(RelationalCondition cond, RegisterID left, RegisterID right)
    {
        return PatchableJump(branch64(cond, left, right));
    }

    Jump branchFloat(DoubleCondition cond, FPRegisterID lhs, FPRegisterID rhs)
    {
        return branchFP<32>(cond, lhs, rhs);
    }

    Jump branchDouble(DoubleCondition cond, FPRegisterID lhs, FPRegisterID rhs)
    {
        return branchFP<64>(cond, lhs, rhs);
    }

    Jump branchDoubleNonZero(FPRegisterID reg, FPRegisterID)
    {
        m_assembler.fcvtInsn<RISCV64Assembler::FCVTType::D, RISCV64Assembler::FCVTType::L>(fpTempRegister, RISCV64Registers::zero);
        return branchFP<64>(DoubleNotEqualAndOrdered, reg, fpTempRegister);
    }

    Jump branchDoubleZeroOrNaN(FPRegisterID reg, FPRegisterID)
    {
        m_assembler.fcvtInsn<RISCV64Assembler::FCVTType::D, RISCV64Assembler::FCVTType::L>(fpTempRegister, RISCV64Registers::zero);
        return branchFP<64>(DoubleEqualOrUnordered, reg, fpTempRegister);
    }

    enum BranchTruncateType { BranchIfTruncateFailed, BranchIfTruncateSuccessful };
    Jump branchTruncateDoubleToInt32(FPRegisterID src, RegisterID dest, BranchTruncateType branchType = BranchIfTruncateFailed)
    {
        auto temp = temps<Data>();
        m_assembler.fcvtInsn<RISCV64Assembler::FPRoundingMode::RTZ, RISCV64Assembler::FCVTType::L, RISCV64Assembler::FCVTType::D>(dest, src);
        m_assembler.signExtend<32>(temp.data(), dest);
        m_assembler.xorInsn(temp.data(), dest, temp.data());
        if (branchType == BranchIfTruncateFailed)
            m_assembler.sltuInsn(temp.data(), RISCV64Registers::zero, temp.data());
        else
            m_assembler.sltiuInsn(temp.data(), temp.data(), Imm::I<1>());

        m_assembler.maskRegister<32>(dest);
        return makeBranch(NotEqual, temp.data(), RISCV64Registers::zero);
    }

    void branchConvertDoubleToInt32(FPRegisterID src, RegisterID dest, JumpList& failureCases, FPRegisterID, bool negZeroCheck = true)
    {
        auto temp = temps<Data>();
        m_assembler.fcvtInsn<RISCV64Assembler::FCVTType::W, RISCV64Assembler::FCVTType::D>(temp.data(),  src);
        m_assembler.fcvtInsn<RISCV64Assembler::FCVTType::D, RISCV64Assembler::FCVTType::W>(fpTempRegister, temp.data());
        m_assembler.maskRegister<32>(dest, temp.data());
        failureCases.append(branchFP<64>(DoubleNotEqualOrUnordered, src, fpTempRegister));

        if (negZeroCheck) {
            Jump resultIsNonZero = makeBranch(NotEqual, temp.data(), RISCV64Registers::zero);
            m_assembler.fmvInsn<RISCV64Assembler::FMVType::X, RISCV64Assembler::FMVType::D>(temp.data(), src);
            failureCases.append(makeBranch(LessThan, temp.data(), RISCV64Registers::zero));
            resultIsNonZero.link(this);
        }
    }

    Call call(PtrTag)
    {
        auto label = m_assembler.label();
        m_assembler.pointerCallPlaceholder(
            [&] {
                auto temp = temps<Data>();
                m_assembler.addiInsn(temp.data(), RISCV64Registers::zero, Imm::I<0>());
                m_assembler.jalrInsn(RISCV64Registers::x1, temp.data(), Imm::I<0>());
            });
        return Call(label, Call::Linkable);
    }

    Call call(RegisterID target, PtrTag)
    {
        m_assembler.jalrInsn(RISCV64Registers::x1, target, Imm::I<0>());
        return Call(m_assembler.label(), Call::None);
    }

    Call call(Address address, PtrTag)
    {
        auto temp = temps<Data, Memory>();
        auto resolution = resolveAddress(address, temp.memory());
        m_assembler.ldInsn(temp.data(), resolution.base, Imm::I(resolution.offset));
        m_assembler.jalrInsn(RISCV64Registers::x1, temp.data(), Imm::I<0>());
        return Call(m_assembler.label(), Call::None);
    }

    Call call(RegisterID callTag) { UNUSED_PARAM(callTag); return call(NoPtrTag); }
    Call call(RegisterID target, RegisterID callTag) { UNUSED_PARAM(callTag); return call(target, NoPtrTag); }
    Call call(Address address, RegisterID callTag) { UNUSED_PARAM(callTag); return call(address, NoPtrTag); }

    void callOperation(const FunctionPtr<OperationPtrTag> operation)
    {
        auto temp = temps<Data>();
        loadImmediate(TrustedImmPtr(operation.executableAddress()), temp.data());
        m_assembler.jalrInsn(RISCV64Registers::x1, temp.data(), Imm::I<0>());
    }

    void getEffectiveAddress(BaseIndex address, RegisterID dest)
    {
        auto resolution = resolveAddress(address, lazyTemp<Memory>());
        m_assembler.addiInsn(dest, resolution.base, Imm::I(resolution.offset));
    }

    void loadFloat(Address address, FPRegisterID dest)
    {
        auto resolution = resolveAddress(address, lazyTemp<Memory>());
        m_assembler.flwInsn(dest, resolution.base, Imm::I(resolution.offset));
    }

    void loadFloat(BaseIndex address, FPRegisterID dest)
    {
        auto resolution = resolveAddress(address, lazyTemp<Memory>());
        m_assembler.flwInsn(dest, resolution.base, Imm::I(resolution.offset));
    }

    void loadFloat(TrustedImmPtr address, FPRegisterID dest)
    {
        auto temp = temps<Memory>();
        loadImmediate(address, temp.memory());
        m_assembler.flwInsn(dest, temp.memory(), Imm::I<0>());
    }

    void loadDouble(Address address, FPRegisterID dest)
    {
        auto resolution = resolveAddress(address, lazyTemp<Memory>());
        m_assembler.fldInsn(dest, resolution.base, Imm::I(resolution.offset));
    }

    void loadDouble(BaseIndex address, FPRegisterID dest)
    {
        auto resolution = resolveAddress(address, lazyTemp<Memory>());
        m_assembler.fldInsn(dest, resolution.base, Imm::I(resolution.offset));
    }

    void loadDouble(TrustedImmPtr address, FPRegisterID dest)
    {
        auto temp = temps<Memory>();
        loadImmediate(address, temp.memory());
        m_assembler.fldInsn(dest, temp.memory(), Imm::I<0>());
    }

    void storeFloat(FPRegisterID src, Address address)
    {
        auto resolution = resolveAddress(address, lazyTemp<Memory>());
        m_assembler.fswInsn(resolution.base, src, Imm::S(resolution.offset));
    }

    void storeFloat(FPRegisterID src, BaseIndex address)
    {
        auto resolution = resolveAddress(address, lazyTemp<Memory>());
        m_assembler.fswInsn(resolution.base, src, Imm::S(resolution.offset));
    }

    void storeFloat(FPRegisterID src, TrustedImmPtr address)
    {
        auto temp = temps<Memory>();
        loadImmediate(address, temp.memory());
        m_assembler.fswInsn(temp.memory(), src, Imm::S<0>());
    }

    void storeDouble(FPRegisterID src, Address address)
    {
        auto resolution = resolveAddress(address, lazyTemp<Memory>());
        m_assembler.fsdInsn(resolution.base, src, Imm::S(resolution.offset));
    }

    void storeDouble(FPRegisterID src, BaseIndex address)
    {
        auto resolution = resolveAddress(address, lazyTemp<Memory>());
        m_assembler.fsdInsn(resolution.base, src, Imm::S(resolution.offset));
    }

    void storeDouble(FPRegisterID src, TrustedImmPtr address)
    {
        auto temp = temps<Memory>();
        loadImmediate(address, temp.memory());
        m_assembler.fsdInsn(temp.memory(), src, Imm::S<0>());
    }

    void addFloat(FPRegisterID op1, FPRegisterID op2, FPRegisterID dest)
    {
        m_assembler.faddInsn<32>(dest, op1, op2);
    }

    void addDouble(FPRegisterID src, FPRegisterID dest)
    {
        addDouble(src, dest, dest);
    }

    void addDouble(FPRegisterID op1, FPRegisterID op2, FPRegisterID dest)
    {
        m_assembler.faddInsn<64>(dest, op1, op2);
    }

    void addDouble(AbsoluteAddress address, FPRegisterID dest)
    {
        loadDouble(TrustedImmPtr(address.m_ptr), fpTempRegister);
        m_assembler.faddInsn<64>(dest, fpTempRegister, dest);
    }

    void subFloat(FPRegisterID op1, FPRegisterID op2, FPRegisterID dest)
    {
        m_assembler.fsubInsn<32>(dest, op1, op2);
    }

    void subDouble(FPRegisterID src, FPRegisterID dest)
    {
        subDouble(dest, src, dest);
    }

    void subDouble(FPRegisterID op1, FPRegisterID op2, FPRegisterID dest)
    {
        m_assembler.fsubInsn<64>(dest, op1, op2);
    }

    void mulFloat(FPRegisterID src, FPRegisterID dest)
    {
        mulFloat(src, dest, dest);
    }

    void mulFloat(FPRegisterID op1, FPRegisterID op2, FPRegisterID dest)
    {
        m_assembler.fmulInsn<32>(dest, op1, op2);
    }

    void mulDouble(FPRegisterID src, FPRegisterID dest)
    {
        mulDouble(src, dest, dest);
    }

    void mulDouble(FPRegisterID op1, FPRegisterID op2, FPRegisterID dest)
    {
        m_assembler.fmulInsn<64>(dest, op1, op2);
    }

    void mulDouble(Address address, FPRegisterID dest)
    {
        loadDouble(address, fpTempRegister);
        mulDouble(fpTempRegister, dest, dest);
    }

    void divFloat(FPRegisterID src, FPRegisterID dest)
    {
        divFloat(dest, src, dest);
    }

    void divFloat(FPRegisterID op1, FPRegisterID op2, FPRegisterID dest)
    {
        m_assembler.fdivInsn<32>(dest, op1, op2);
    }

    void divDouble(FPRegisterID src, FPRegisterID dest)
    {
        divDouble(dest, src, dest);
    }

    void divDouble(FPRegisterID op1, FPRegisterID op2, FPRegisterID dest)
    {
        m_assembler.fdivInsn<64>(dest, op1, op2);
    }

    void sqrtFloat(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.fsqrtInsn<32>(dest, src);
    }

    void sqrtDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.fsqrtInsn<64>(dest, src);
    }

    void absFloat(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.fsgnjxInsn<32>(dest, src, src);
    }

    void absDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.fsgnjxInsn<64>(dest, src, src);
    }

    void ceilFloat(FPRegisterID src, FPRegisterID dest)
    {
        roundFP<32, RISCV64Assembler::FPRoundingMode::RUP>(src, dest);
    }

    void ceilDouble(FPRegisterID src, FPRegisterID dest)
    {
        roundFP<64, RISCV64Assembler::FPRoundingMode::RUP>(src, dest);
    }

    void floorFloat(FPRegisterID src, FPRegisterID dest)
    {
        roundFP<32, RISCV64Assembler::FPRoundingMode::RDN>(src, dest);
    }

    void floorDouble(FPRegisterID src, FPRegisterID dest)
    {
        roundFP<64, RISCV64Assembler::FPRoundingMode::RDN>(src, dest);
    }

    void roundTowardNearestIntFloat(FPRegisterID src, FPRegisterID dest)
    {
        roundFP<32, RISCV64Assembler::FPRoundingMode::RNE>(src, dest);
    }

    void roundTowardNearestIntDouble(FPRegisterID src, FPRegisterID dest)
    {
        roundFP<64, RISCV64Assembler::FPRoundingMode::RNE>(src, dest);
    }

    void roundTowardZeroFloat(FPRegisterID src, FPRegisterID dest)
    {
        roundFP<32, RISCV64Assembler::FPRoundingMode::RTZ>(src, dest);
    }

    void roundTowardZeroDouble(FPRegisterID src, FPRegisterID dest)
    {
        roundFP<64, RISCV64Assembler::FPRoundingMode::RTZ>(src, dest);
    }

    void andFloat(FPRegisterID op1, FPRegisterID op2, FPRegisterID dest)
    {
        auto temp = temps<Data, Memory>();
        m_assembler.fmvInsn<RISCV64Assembler::FMVType::X, RISCV64Assembler::FMVType::W>(temp.data(), op1);
        m_assembler.fmvInsn<RISCV64Assembler::FMVType::X, RISCV64Assembler::FMVType::W>(temp.memory(), op2);
        m_assembler.andInsn(temp.data(), temp.data(), temp.memory());
        m_assembler.fmvInsn<RISCV64Assembler::FMVType::W, RISCV64Assembler::FMVType::X>(dest, temp.data());
    }

    void andDouble(FPRegisterID op1, FPRegisterID op2, FPRegisterID dest)
    {
        auto temp = temps<Data, Memory>();
        m_assembler.fmvInsn<RISCV64Assembler::FMVType::X, RISCV64Assembler::FMVType::D>(temp.data(), op1);
        m_assembler.fmvInsn<RISCV64Assembler::FMVType::X, RISCV64Assembler::FMVType::D>(temp.memory(), op2);
        m_assembler.andInsn(temp.data(), temp.data(), temp.memory());
        m_assembler.fmvInsn<RISCV64Assembler::FMVType::D, RISCV64Assembler::FMVType::X>(dest, temp.data());
    }

    void orFloat(FPRegisterID op1, FPRegisterID op2, FPRegisterID dest)
    {
        auto temp = temps<Data, Memory>();
        m_assembler.fmvInsn<RISCV64Assembler::FMVType::X, RISCV64Assembler::FMVType::W>(temp.data(), op1);
        m_assembler.fmvInsn<RISCV64Assembler::FMVType::X, RISCV64Assembler::FMVType::W>(temp.memory(), op2);
        m_assembler.orInsn(temp.data(), temp.data(), temp.memory());
        m_assembler.fmvInsn<RISCV64Assembler::FMVType::W, RISCV64Assembler::FMVType::X>(dest, temp.data());
    }

    void orDouble(FPRegisterID op1, FPRegisterID op2, FPRegisterID dest)
    {
        auto temp = temps<Data, Memory>();
        m_assembler.fmvInsn<RISCV64Assembler::FMVType::X, RISCV64Assembler::FMVType::D>(temp.data(), op1);
        m_assembler.fmvInsn<RISCV64Assembler::FMVType::X, RISCV64Assembler::FMVType::D>(temp.memory(), op2);
        m_assembler.orInsn(temp.data(), temp.data(), temp.memory());
        m_assembler.fmvInsn<RISCV64Assembler::FMVType::D, RISCV64Assembler::FMVType::X>(dest, temp.data());
    }

    void negateFloat(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.fsgnjnInsn<32>(dest, src, src);
    }

    void negateDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.fsgnjnInsn<64>(dest, src, src);
    }

    void compareFloat(DoubleCondition cond, FPRegisterID lhs, FPRegisterID rhs, RegisterID dest)
    {
        compareFP<32>(cond, lhs, rhs, dest);
    }

    void compareDouble(DoubleCondition cond, FPRegisterID lhs, FPRegisterID rhs, RegisterID dest)
    {
        compareFP<64>(cond, lhs, rhs, dest);
    }

    void convertInt32ToFloat(RegisterID src, FPRegisterID dest)
    {
        m_assembler.fcvtInsn<RISCV64Assembler::FCVTType::S, RISCV64Assembler::FCVTType::W>(dest, src);
    }

    void convertInt32ToDouble(RegisterID src, FPRegisterID dest)
    {
        m_assembler.fcvtInsn<RISCV64Assembler::FCVTType::D, RISCV64Assembler::FCVTType::W>(dest, src);
    }

    void convertInt32ToDouble(TrustedImm32 imm, FPRegisterID dest)
    {
        auto temp = temps<Data>();
        loadImmediate(imm, temp.data());
        convertInt32ToDouble(temp.data(), dest);
    }

    void convertInt64ToFloat(RegisterID src, FPRegisterID dest)
    {
        m_assembler.fcvtInsn<RISCV64Assembler::FCVTType::S, RISCV64Assembler::FCVTType::L>(dest, src);
    }

    void convertInt64ToDouble(RegisterID src, FPRegisterID dest)
    {
        m_assembler.fcvtInsn<RISCV64Assembler::FCVTType::D, RISCV64Assembler::FCVTType::L>(dest, src);
    }

    void convertUInt64ToFloat(RegisterID src, FPRegisterID dest)
    {
        m_assembler.fcvtInsn<RISCV64Assembler::FCVTType::S, RISCV64Assembler::FCVTType::LU>(dest, src);
    }

    void convertUInt64ToDouble(RegisterID src, FPRegisterID dest)
    {
        m_assembler.fcvtInsn<RISCV64Assembler::FCVTType::D, RISCV64Assembler::FCVTType::LU>(dest, src);
    }

    void convertFloatToDouble(FPRegisterID src, FPRegisterID dest)
    {
        auto temp = temps<Data>();
        m_assembler.fmvInsn<RISCV64Assembler::FMVType::X, RISCV64Assembler::FMVType::W>(temp.data(), src);
        m_assembler.fmvInsn<RISCV64Assembler::FMVType::W, RISCV64Assembler::FMVType::X>(dest, temp.data());
        m_assembler.fcvtInsn<RISCV64Assembler::FCVTType::D, RISCV64Assembler::FCVTType::S>(dest, dest);
    }

    void convertDoubleToFloat(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.fcvtInsn<RISCV64Assembler::FCVTType::S, RISCV64Assembler::FCVTType::D>(dest, src);
    }

    void truncateFloatToInt32(FPRegisterID src, RegisterID dest)
    {
        m_assembler.fcvtInsn<RISCV64Assembler::FPRoundingMode::RTZ, RISCV64Assembler::FCVTType::W, RISCV64Assembler::FCVTType::S>(dest, src);
        m_assembler.maskRegister<32>(dest);
    }

    void truncateFloatToUint32(FPRegisterID src, RegisterID dest)
    {
        m_assembler.fcvtInsn<RISCV64Assembler::FPRoundingMode::RTZ, RISCV64Assembler::FCVTType::WU, RISCV64Assembler::FCVTType::S>(dest, src);
        m_assembler.maskRegister<32>(dest);
    }

    void truncateFloatToInt64(FPRegisterID src, RegisterID dest)
    {
        m_assembler.fcvtInsn<RISCV64Assembler::FPRoundingMode::RTZ, RISCV64Assembler::FCVTType::L, RISCV64Assembler::FCVTType::S>(dest, src);
    }

    void truncateFloatToUint64(FPRegisterID src, RegisterID dest)
    {
        m_assembler.fcvtInsn<RISCV64Assembler::FPRoundingMode::RTZ, RISCV64Assembler::FCVTType::LU, RISCV64Assembler::FCVTType::S>(dest, src);
    }

    void truncateFloatToUint64(FPRegisterID src, RegisterID dest, FPRegisterID, FPRegisterID)
    {
        truncateFloatToUint64(src, dest);
    }

    void truncateDoubleToInt32(FPRegisterID src, RegisterID dest)
    {
        m_assembler.fcvtInsn<RISCV64Assembler::FPRoundingMode::RTZ, RISCV64Assembler::FCVTType::W, RISCV64Assembler::FCVTType::D>(dest, src);
        m_assembler.maskRegister<32>(dest);
    }

    void truncateDoubleToUint32(FPRegisterID src, RegisterID dest)
    {
        m_assembler.fcvtInsn<RISCV64Assembler::FPRoundingMode::RTZ, RISCV64Assembler::FCVTType::WU, RISCV64Assembler::FCVTType::D>(dest, src);
        m_assembler.maskRegister<32>(dest);
    }

    void truncateDoubleToInt64(FPRegisterID src, RegisterID dest)
    {
        m_assembler.fcvtInsn<RISCV64Assembler::FPRoundingMode::RTZ, RISCV64Assembler::FCVTType::L, RISCV64Assembler::FCVTType::D>(dest, src);
    }

    void truncateDoubleToUint64(FPRegisterID src, RegisterID dest)
    {
        m_assembler.fcvtInsn<RISCV64Assembler::FPRoundingMode::RTZ, RISCV64Assembler::FCVTType::LU, RISCV64Assembler::FCVTType::D>(dest, src);
    }

    void truncateDoubleToUint64(FPRegisterID src, RegisterID dest, FPRegisterID, FPRegisterID)
    {
        truncateDoubleToUint64(src, dest);
    }

    void push(RegisterID src)
    {
        m_assembler.addiInsn(RISCV64Registers::sp, RISCV64Registers::sp, Imm::I<-8>());
        m_assembler.sdInsn(RISCV64Registers::sp, src, Imm::S<0>());
    }

    void push(TrustedImm32 imm)
    {
        auto temp = temps<Data>();
        loadImmediate(imm, temp.data());
        m_assembler.addiInsn(RISCV64Registers::sp, RISCV64Registers::sp, Imm::I<-8>());
        m_assembler.sdInsn(RISCV64Registers::sp, temp.data(), Imm::S<0>());
    }

    void pushPair(RegisterID src1, RegisterID src2)
    {
        m_assembler.addiInsn(RISCV64Registers::sp, RISCV64Registers::sp, Imm::I<-16>());
        m_assembler.sdInsn(RISCV64Registers::sp, src1, Imm::S<0>());
        m_assembler.sdInsn(RISCV64Registers::sp, src2, Imm::S<8>());
    }

    void pop(RegisterID dest)
    {
        m_assembler.ldInsn(dest, RISCV64Registers::sp, Imm::I<0>());
        m_assembler.addiInsn(RISCV64Registers::sp, RISCV64Registers::sp, Imm::I<8>());
    }

    void popPair(RegisterID dest1, RegisterID dest2)
    {
        m_assembler.ldInsn(dest1, RISCV64Registers::sp, Imm::I<0>());
        m_assembler.ldInsn(dest2, RISCV64Registers::sp, Imm::I<8>());
        m_assembler.addiInsn(RISCV64Registers::sp, RISCV64Registers::sp, Imm::I<16>());
    }

    void abortWithReason(AbortReason reason)
    {
        auto temp = temps<Data>();
        loadImmediate(TrustedImm32(reason), temp.data());
        m_assembler.ebreakInsn();
    }

    void abortWithReason(AbortReason reason, intptr_t misc)
    {
        auto temp = temps<Data, Memory>();
        loadImmediate(TrustedImm32(reason), temp.data());
        loadImmediate(TrustedImm64(misc), temp.memory());
        m_assembler.ebreakInsn();
    }

    void breakpoint(uint16_t = 0xc471)
    {
        m_assembler.ebreakInsn();
    }

    void nop()
    {
        m_assembler.addiInsn(RISCV64Registers::zero, RISCV64Registers::zero, Imm::I<0>());
    }

    void memoryFence() { }
    void storeFence() { }
    void loadFence() { }

    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD_WITH_RETURN(branchAtomicWeakCAS8, JumpList);
    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD_WITH_RETURN(branchAtomicWeakCAS16, JumpList);
    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD_WITH_RETURN(branchAtomicWeakCAS32, JumpList);
    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD_WITH_RETURN(branchAtomicWeakCAS64, JumpList);

    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD(moveConditionally32);
    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD(moveConditionally64);
    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD(moveConditionallyFloat);
    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD(moveConditionallyDouble);
    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD(moveConditionallyTest32);
    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD(moveConditionallyTest64);
    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD(moveDoubleConditionally32);
    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD(moveDoubleConditionally64);
    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD(moveDoubleConditionallyFloat);
    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD(moveDoubleConditionallyDouble);
    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD(moveDoubleConditionallyTest32);
    MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD(moveDoubleConditionallyTest64);

private:
    struct Imm {
        template<typename T>
        using EnableIfInteger = std::enable_if_t<(std::is_same_v<T, int32_t> || std::is_same_v<T, int64_t>)>;

        template<typename ImmediateType, typename T, typename = EnableIfInteger<T>>
        static bool isValid(T value) { return ImmediateType::isValid(value); }

        using IType = RISCV64Assembler::IImmediate;
        template<int32_t value>
        static IType I() { return IType::v<IType, value>(); }
        template<typename T, typename = EnableIfInteger<T>>
        static IType I(T value) { return IType::v<IType>(value); }
        static IType I(uint32_t value) { return IType(value); }

        using SType = RISCV64Assembler::SImmediate;
        template<int32_t value>
        static SType S() { return SType::v<SType, value>(); }
        template<typename T, typename = EnableIfInteger<T>>
        static SType S(T value) { return SType::v<SType>(value); }

        using BType = RISCV64Assembler::BImmediate;
        template<int32_t value>
        static BType B() { return BType::v<BType, value>(); }
        template<typename T, typename = EnableIfInteger<T>>
        static BType B(T value) { return BType::v<BType>(value); }
        static BType B(uint32_t value) { return BType(value); }

        using UType = RISCV64Assembler::UImmediate;
        static UType U(uint32_t value) { return UType(value); }

        using JType = RISCV64Assembler::JImmediate;
        template<int32_t value>
        static JType J() { return JType::v<JType, value>(); }
    };

    void loadImmediate(TrustedImm32 imm, RegisterID dest)
    {
        RISCV64Assembler::ImmediateLoader(imm.m_value).moveInto(m_assembler, dest);
    }

    void loadImmediate(TrustedImm64 imm, RegisterID dest)
    {
        RISCV64Assembler::ImmediateLoader(imm.m_value).moveInto(m_assembler, dest);
    }

    void loadImmediate(TrustedImmPtr imm, RegisterID dest)
    {
        intptr_t value = imm.asIntptr();
        if constexpr (sizeof(intptr_t) == sizeof(int64_t))
            loadImmediate(TrustedImm64(int64_t(value)), dest);
        else
            loadImmediate(TrustedImm32(int32_t(value)), dest);
    }

    struct AddressResolution {
        RegisterID base;
        int32_t offset;
    };

    template<typename RegisterType>
    AddressResolution resolveAddress(BaseIndex address, RegisterType destination)
    {
        if (!!address.offset) {
            if (RISCV64Assembler::ImmediateBase<12>::isValid(address.offset)) {
                if (address.scale != TimesOne) {
                    m_assembler.slliInsn(destination, address.index, uint32_t(address.scale));
                    m_assembler.addInsn(destination, address.base, destination);
                } else
                    m_assembler.addInsn(destination, address.base, address.index);
                return { destination, address.offset };
            }

            if (address.scale != TimesOne) {
                uint32_t scale = address.scale;
                int32_t upperOffset = address.offset >> scale;
                int32_t lowerOffset = address.offset & ((1 << scale) - 1);

                if (!RISCV64Assembler::ImmediateBase<12>::isValid(upperOffset)) {
                    RISCV64Assembler::ImmediateLoader imml(upperOffset);
                    imml.moveInto(m_assembler, destination);
                    m_assembler.addInsn(destination, address.index, destination);
                } else
                    m_assembler.addiInsn(destination, address.index, Imm::I(upperOffset));
                m_assembler.slliInsn(destination, destination, scale);
                m_assembler.oriInsn(destination, destination, Imm::I(lowerOffset));
            } else {
                RISCV64Assembler::ImmediateLoader imml(address.offset);
                imml.moveInto(m_assembler, destination);
                m_assembler.addInsn(destination, destination, address.index);
            }
            m_assembler.addInsn(destination, address.base, destination);
            return { destination, 0 };
        }

        if (address.scale != TimesOne) {
            m_assembler.slliInsn(destination, address.index, address.scale);
            m_assembler.addInsn(destination, address.base, destination);
        } else
            m_assembler.addInsn(destination, address.base, address.index);
        return { destination, 0 };
    }

    template<typename RegisterType>
    AddressResolution resolveAddress(Address address, RegisterType destination)
    {
        if (RISCV64Assembler::ImmediateBase<12>::isValid(address.offset))
            return { address.base, address.offset };

        uint32_t value = *reinterpret_cast<uint32_t*>(&address.offset);
        if (value & (1 << 11))
            value += (1 << 12);

        m_assembler.luiInsn(destination, Imm::U(value));
        m_assembler.addiInsn(destination, destination, Imm::I(value & ((1 << 12) - 1)));
        m_assembler.addInsn(destination, address.base, destination);
        return { destination, 0 };
    }

    template<typename RegisterType>
    AddressResolution resolveAddress(ExtendedAddress address, RegisterType destination)
    {
        if (RISCV64Assembler::ImmediateBase<12>::isValid(address.offset))
            return { address.base, int32_t(address.offset) };

        RISCV64Assembler::ImmediateLoader imml(int64_t(address.offset));
        imml.moveInto(m_assembler, destination);
        m_assembler.addInsn(destination, address.base, destination);
        return { destination, 0 };
    }

    Jump makeBranch(RelationalCondition condition, RegisterID lhs, RegisterID rhs)
    {
        auto label = m_assembler.label();
        m_assembler.branchPlaceholder(
            [&] {
                switch (condition) {
                case Equal:
                    return m_assembler.beqInsn(lhs, rhs, Imm::B<0>());
                case NotEqual:
                    return m_assembler.bneInsn(lhs, rhs, Imm::B<0>());
                case Above:
                    return m_assembler.bltuInsn(rhs, lhs, Imm::B<0>());
                case AboveOrEqual:
                    return m_assembler.bgeuInsn(lhs, rhs, Imm::B<0>());
                case Below:
                    return m_assembler.bltuInsn(lhs, rhs, Imm::B<0>());
                case BelowOrEqual:
                    return m_assembler.bgeuInsn(rhs, lhs, Imm::B<0>());
                case GreaterThan:
                    return m_assembler.bltInsn(rhs, lhs, Imm::B<0>());
                case GreaterThanOrEqual:
                    return m_assembler.bgeInsn(lhs, rhs, Imm::B<0>());
                case LessThan:
                    return m_assembler.bltInsn(lhs, rhs, Imm::B<0>());
                case LessThanOrEqual:
                    return m_assembler.bgeInsn(rhs, lhs, Imm::B<0>());
                }
            });
        return Jump(label);
    }

    void compareFinalize(RelationalCondition cond, RegisterID lhs, RegisterID rhs, RegisterID dest)
    {
        switch (cond) {
        case Equal:
            m_assembler.xorInsn(dest, lhs, rhs);
            m_assembler.sltiuInsn(dest, dest, Imm::I<1>());
            break;
        case NotEqual:
            m_assembler.xorInsn(dest, lhs, rhs);
            m_assembler.sltuInsn(dest, RISCV64Registers::zero, dest);
            break;
        case Above:
            m_assembler.sltuInsn(dest, rhs, lhs);
            break;
        case AboveOrEqual:
            m_assembler.sltuInsn(dest, lhs, rhs);
            m_assembler.xoriInsn(dest, dest, Imm::I<1>());
            break;
        case Below:
            m_assembler.sltuInsn(dest, lhs, rhs);
            break;
        case BelowOrEqual:
            m_assembler.sltuInsn(dest, rhs, lhs);
            m_assembler.xoriInsn(dest, dest, Imm::I<1>());
            break;
        case GreaterThan:
            m_assembler.sltInsn(dest, rhs, lhs);
            break;
        case GreaterThanOrEqual:
            m_assembler.sltInsn(dest, lhs, rhs);
            m_assembler.xoriInsn(dest, dest, Imm::I<1>());
            break;
        case LessThan:
            m_assembler.sltInsn(dest, lhs, rhs);
            break;
        case LessThanOrEqual:
            m_assembler.sltInsn(dest, rhs, lhs);
            m_assembler.xoriInsn(dest, dest, Imm::I<1>());
            break;
        }
    }

    void testFinalize(ResultCondition cond, RegisterID src, RegisterID dest)
    {
        switch (cond) {
        case Overflow:
        case Signed:
        case PositiveOrZero:
            // None of the above should be used for testing operations.
            RELEASE_ASSERT_NOT_REACHED();
            break;
        case Zero:
            m_assembler.sltiuInsn(dest, src, Imm::I<1>());
            break;
        case NonZero:
            m_assembler.sltuInsn(dest, RISCV64Registers::zero, src);
            break;
        }
    }

    template<unsigned fpSize, bool invert = false>
    Jump branchFP(DoubleCondition cond, FPRegisterID lhs, FPRegisterID rhs)
    {
        auto temp = temps<Data>();
        JumpList unorderedJump;

        m_assembler.fclassInsn<fpSize>(temp.data(), lhs);
        m_assembler.andiInsn(temp.data(), temp.data(), Imm::I<0b1100000000>());
        unorderedJump.append(makeBranch(NotEqual, temp.data(), RISCV64Registers::zero));

        m_assembler.fclassInsn<fpSize>(temp.data(), rhs);
        m_assembler.andiInsn(temp.data(), temp.data(), Imm::I<0b1100000000>());
        unorderedJump.append(makeBranch(NotEqual, temp.data(), RISCV64Registers::zero));

        switch (cond) {
        case DoubleEqualAndOrdered:
        case DoubleEqualOrUnordered:
            m_assembler.feqInsn<fpSize>(temp.data(), lhs, rhs);
            break;
        case DoubleNotEqualAndOrdered:
        case DoubleNotEqualOrUnordered:
            m_assembler.feqInsn<fpSize>(temp.data(), lhs, rhs);
            m_assembler.xoriInsn(temp.data(), temp.data(), Imm::I<1>());
            break;
        case DoubleGreaterThanAndOrdered:
        case DoubleGreaterThanOrUnordered:
            m_assembler.fltInsn<fpSize>(temp.data(), rhs, lhs);
            break;
        case DoubleGreaterThanOrEqualAndOrdered:
        case DoubleGreaterThanOrEqualOrUnordered:
            m_assembler.fleInsn<fpSize>(temp.data(), rhs, lhs);
            break;
        case DoubleLessThanAndOrdered:
        case DoubleLessThanOrUnordered:
            m_assembler.fltInsn<fpSize>(temp.data(), lhs, rhs);
            break;
        case DoubleLessThanOrEqualAndOrdered:
        case DoubleLessThanOrEqualOrUnordered:
            m_assembler.fleInsn<fpSize>(temp.data(), lhs, rhs);
            break;
        }

        Jump end = jump();
        unorderedJump.link(this);

        switch (cond) {
        case DoubleEqualAndOrdered:
        case DoubleNotEqualAndOrdered:
        case DoubleGreaterThanAndOrdered:
        case DoubleGreaterThanOrEqualAndOrdered:
        case DoubleLessThanAndOrdered:
        case DoubleLessThanOrEqualAndOrdered:
            m_assembler.addiInsn(temp.data(), RISCV64Registers::zero, Imm::I<0>());
            break;
        case DoubleEqualOrUnordered:
        case DoubleNotEqualOrUnordered:
        case DoubleGreaterThanOrUnordered:
        case DoubleGreaterThanOrEqualOrUnordered:
        case DoubleLessThanOrUnordered:
        case DoubleLessThanOrEqualOrUnordered:
            m_assembler.addiInsn(temp.data(), RISCV64Registers::zero, Imm::I<1>());
            break;
        }

        end.link(this);
        return makeBranch(invert ? Equal : NotEqual, temp.data(), RISCV64Registers::zero);
    }

    template<unsigned fpSize, RISCV64Assembler::FPRoundingMode RM>
    void roundFP(FPRegisterID src, FPRegisterID dest)
    {
        static_assert(fpSize == 32 || fpSize == 64);
        auto temp = temps<Data>();

        JumpList end;

        // Test the given source register for NaN condition. If detected, it should be
        // propagated to the destination register.
        m_assembler.fclassInsn<fpSize>(temp.data(), src);
        m_assembler.andiInsn(temp.data(), temp.data(), Imm::I<0b1100000000>());
        Jump notNaN = makeBranch(Equal, temp.data(), RISCV64Registers::zero);

        m_assembler.faddInsn<fpSize>(dest, src, src);
        end.append(jump());

        notNaN.link(this);
        m_assembler.fsgnjxInsn<fpSize>(fpTempRegister, src, src);

        // Compare the absolute source value with the maximum representable integer value.
        // Rounding is only possible if the absolute source value is smaller.
        if constexpr (fpSize == 32) {
            m_assembler.addiInsn(temp.data(), RISCV64Registers::zero, Imm::I<0b10010111>());
            m_assembler.slliInsn<23>(temp.data(), temp.data());
            m_assembler.fmvInsn<RISCV64Assembler::FMVType::W, RISCV64Assembler::FMVType::X>(fpTempRegister2, temp.data());
        } else {
            m_assembler.addiInsn(temp.data(), RISCV64Registers::zero, Imm::I<0b10000110100>());
            m_assembler.slliInsn<52>(temp.data(), temp.data());
            m_assembler.fmvInsn<RISCV64Assembler::FMVType::D, RISCV64Assembler::FMVType::X>(fpTempRegister2, temp.data());
        }

        m_assembler.fltInsn<fpSize>(temp.data(), fpTempRegister, fpTempRegister2);
        Jump notRoundable = makeBranch(Equal, temp.data(), RISCV64Registers::zero);

        FPRegisterID dealiasedSrc = src;
        if (src == dest) {
            m_assembler.fsgnjInsn<fpSize>(fpTempRegister, src, src);
            dealiasedSrc = fpTempRegister;
        }

        // Rounding can now be done by roundtripping through a general-purpose register
        // with the desired rounding mode applied.
        if constexpr (fpSize == 32) {
            m_assembler.fcvtInsn<RM, RISCV64Assembler::FCVTType::W, RISCV64Assembler::FCVTType::S>(temp.data(), dealiasedSrc);
            m_assembler.fcvtInsn<RM, RISCV64Assembler::FCVTType::S, RISCV64Assembler::FCVTType::W>(dest, temp.data());
        } else {
            m_assembler.fcvtInsn<RM, RISCV64Assembler::FCVTType::L, RISCV64Assembler::FCVTType::D>(temp.data(), dealiasedSrc);
            m_assembler.fcvtInsn<RM, RISCV64Assembler::FCVTType::D, RISCV64Assembler::FCVTType::L>(dest, temp.data());
        }
        m_assembler.fsgnjInsn<fpSize>(dest, dest, dealiasedSrc);
        end.append(jump());

        notRoundable.link(this);
        // If not roundable, the value should still be moved over into the destination register.
        if (src != dest)
            m_assembler.fsgnjInsn<fpSize>(dest, src, src);

        end.link(this);
    }

    template<unsigned fpSize>
    void compareFP(DoubleCondition cond, FPRegisterID lhs, FPRegisterID rhs, RegisterID dest)
    {
        static_assert(fpSize == 32 || fpSize == 64);
        auto temp = temps<Data>();

        JumpList unorderedJump;

        // Detect any NaN values that could still yield a positive comparison, depending on the condition.
        m_assembler.fclassInsn<fpSize>(temp.data(), lhs);
        m_assembler.andiInsn(temp.data(), temp.data(), Imm::I<0b1100000000>());
        unorderedJump.append(makeBranch(NotEqual, temp.data(), RISCV64Registers::zero));

        m_assembler.fclassInsn<fpSize>(temp.data(), rhs);
        m_assembler.andiInsn(temp.data(), temp.data(), Imm::I<0b1100000000>());
        unorderedJump.append(makeBranch(NotEqual, temp.data(), RISCV64Registers::zero));

        switch (cond) {
        case DoubleEqualAndOrdered:
        case DoubleEqualOrUnordered:
            m_assembler.feqInsn<fpSize>(dest, lhs, rhs);
            break;
        case DoubleNotEqualAndOrdered:
        case DoubleNotEqualOrUnordered:
            m_assembler.feqInsn<fpSize>(dest, lhs, rhs);
            m_assembler.xoriInsn(dest, dest, Imm::I<1>());
            break;
        case DoubleGreaterThanAndOrdered:
        case DoubleGreaterThanOrUnordered:
            m_assembler.fltInsn<fpSize>(dest, rhs, lhs);
            break;
        case DoubleGreaterThanOrEqualAndOrdered:
        case DoubleGreaterThanOrEqualOrUnordered:
            m_assembler.fleInsn<fpSize>(dest, rhs, lhs);
            break;
        case DoubleLessThanAndOrdered:
        case DoubleLessThanOrUnordered:
            m_assembler.fltInsn<fpSize>(dest, lhs, rhs);
            break;
        case DoubleLessThanOrEqualAndOrdered:
        case DoubleLessThanOrEqualOrUnordered:
            m_assembler.fleInsn<fpSize>(dest, lhs, rhs);
            break;
        }

        Jump end = jump();
        unorderedJump.link(this);

        switch (cond) {
        case DoubleEqualAndOrdered:
        case DoubleNotEqualAndOrdered:
        case DoubleGreaterThanAndOrdered:
        case DoubleGreaterThanOrEqualAndOrdered:
        case DoubleLessThanAndOrdered:
        case DoubleLessThanOrEqualAndOrdered:
            m_assembler.addiInsn(dest, RISCV64Registers::zero, Imm::I<0>());
            break;
        case DoubleEqualOrUnordered:
        case DoubleNotEqualOrUnordered:
        case DoubleGreaterThanOrUnordered:
        case DoubleGreaterThanOrEqualOrUnordered:
        case DoubleLessThanOrUnordered:
        case DoubleLessThanOrEqualOrUnordered:
            m_assembler.addiInsn(dest, RISCV64Registers::zero, Imm::I<1>());
            break;
        }

        end.link(this);
    }
};

} // namespace JSC

#undef MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD
#undef MACRO_ASSEMBLER_RISCV64_TEMPLATED_NOOP_METHOD_WITH_RETURN

#endif // ENABLE(ASSEMBLER) && CPU(RISCV64)
