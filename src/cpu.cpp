#include "cpu.h"

#include "memory.h"
#include <iostream>

namespace nesemu
{
	CPU::CPU()
	{
		mRegA = 0;
		mRegX = 0;
		mRegY = 0;
		mStatusRegister = 0;
		mProgramCounter = 0;
		mStackPointer = 0;
		mNMILabel = 0;
		mIRQLabel = 0;
		mResetLabel = 0;
	}

	void CPU::Initialise()
	{
		RegisterOpcodes();

		mNMILabel = GMemory->ReadMemoryAddress(0xFFFA);
		mResetLabel = GMemory->ReadMemoryAddress(0xFFFC);
		mIRQLabel = GMemory->ReadMemoryAddress(0xFFFE);

		std::cout << "NMI: " << std::hex << mNMILabel << std::endl;
		std::cout << "reset: " << std::hex << mResetLabel << std::endl;
		std::cout << "IRQ: " << std::hex << mIRQLabel << std::endl;

		Reset();
	}

	void CPU::Tick()
	{
		mCurrentCycles = 0;

		uint8_t op = GMemory->ReadByte(mProgramCounter);
		Opcode* opcode = DecodeOpcode(op);
		mCurrentOpcode = opcode;
		if (opcode != nullptr)
		{
			mCurrentCycles += opcode->mCycles;
			// TODO: Add extra cycle for indexed addressing mode if page boundary was crossed
		
			int operandLength = 0;
			if (opcode->mAddressingMode == AddressingMode::Absolute || opcode->mAddressingMode == AddressingMode::AbsoluteX || opcode->mAddressingMode == AddressingMode::AbsoluteY)
			{
				operandLength = 2;
			}
			else if (opcode->mAddressingMode != AddressingMode::Implied)
			{
				operandLength = 1;
			}

			// instruction length + operand length
			mNextOperationAddress = mProgramCounter + operandLength + 1;

			auto callback = opcode->mCallback;
			if (callback != nullptr)
			{
				// Get operand address
				const uint16_t address = DecodeOperandAddress(mProgramCounter + 1, opcode->mAddressingMode);
				
				mCurrentOperandAddress = address;

				// Execute the instruction
				(this->*callback)();
			}

#ifdef NESEMU_DEBUG
			const char valSymbol = (mCurrentOpcode->mAddressingMode == AddressingMode::Immediate ? '#' : '$');
			std::cout << opcode->mName << " (" << std::hex << (int)GMemory->ReadByte(mProgramCounter) << ")   ";
			if (operandLength == 2)
				std::cout << valSymbol << std::hex << (int)GMemory->ReadWord(mProgramCounter + 1);
			else if(operandLength == 1)
				std::cout << valSymbol << std::hex << (int)GMemory->ReadByte(mProgramCounter + 1);
			std::cout << std::endl;
#endif

			// Increase the program counter
			mProgramCounter = mNextOperationAddress;
		}
	}

	void CPU::QueueNMI()
	{
		uint8_t r = mProgramCounter;
		uint8_t l = mProgramCounter >> 8;
		StackPush(l);
		StackPush(r); // TODO: Is the order right??
		mNMI = true;
	}

	void CPU::StackPush(uint8_t arg_value)
	{
		GMemory->Write(mStackPointer, &arg_value, sizeof(arg_value));
		mStackPointer--;
	}

	uint8_t CPU::StackPop()
	{
		mStackPointer++;
		uint8_t retVal = GMemory->ReadByte(mStackPointer);
		return retVal;
	}

	void CPU::ClearFlags(statusflag_t flags)
	{
		mStatusRegister &= ~flags;
	}

	void CPU::SetFlags(statusflag_t flags)
	{
		mStatusRegister |= flags;
	}

	void CPU::SetFlags(statusflag_t flags, bool arg_set)
	{
		if (arg_set)
			SetFlags(flags);
		else
			ClearFlags(flags);
	}

	void CPU::SetZNFlags(uint16_t arg_value)
	{
		ClearFlags(STATUSFLAG_NEGATIVE | STATUSFLAG_ZERO);
		if (arg_value & 0x80)
		{
			SetFlags(STATUSFLAG_NEGATIVE);
		}
		else if (arg_value == 0)
			SetFlags(STATUSFLAG_ZERO);
	}

	bool CPU::GetFlags(statusflag_t flags)
	{
		return mStatusRegister & flags;
	}

	Opcode* CPU::DecodeOpcode(uint8_t arg_op)
	{
		return mOpcodeTable[arg_op];
	}

	void CPU::Reset()
	{
		mProgramCounter = mResetLabel;
	}


	// get memory address of value used by opcode
	uint16_t CPU::DecodeOperandAddress(const uint16_t arg_addr, AddressingMode arg_addrmode)
	{
		uint16_t outAddress = 0;
		switch (arg_addrmode)
		{
		case AddressingMode::Immediate:
			outAddress = arg_addr;
			break;
		case AddressingMode::Absolute:
			outAddress = GMemory->ReadMemoryAddress(arg_addr);
			break;
		case AddressingMode::AbsoluteX:
			outAddress = GMemory->ReadMemoryAddress(arg_addr) + mRegX;
			break;
		case AddressingMode::AbsoluteY:
			return GMemory->ReadMemoryAddress(arg_addr) + mRegY;
			break;
		case AddressingMode::ZeroPage:
			outAddress = ((uint16_t)0) | (arg_addr >> 8);
			break;
		case AddressingMode::ZeroPageX:
			outAddress = ((uint16_t)0) | (arg_addr >> 8) + mRegX;
			break;
		case AddressingMode::ZeroPageY:
			outAddress = ((uint16_t)0) | (arg_addr >> 8) + mRegY;
			break;
		case AddressingMode::Indirect:
		{
			const int addr = GMemory->ReadMemoryAddress(arg_addr);
			outAddress = GMemory->ReadMemoryAddress(addr);
			break;
		}
		case AddressingMode::IndirectX:
		{
			const int addr = ((uint16_t)0) | (arg_addr >> 8) + mRegY;
			outAddress = GMemory->ReadMemoryAddress(addr + mRegX);
			break;
		}
		case AddressingMode::IndirectY:
		{
			const int addr = ((uint16_t)0) | (arg_addr >> 8) + mRegY;
			outAddress = GMemory->ReadMemoryAddress(addr + mRegY);
			break;
		}
		case AddressingMode::Implied:
			break;
		default:
			std::cout << "Unhandled AddressingMode" << std::endl;
			break;
		}

		return outAddress;
	}

	void CPU::Branch(const uint8_t& arg_offset)
	{
		// TODO: Make "Relative" memory addressing mode
		const bool negative = (arg_offset & (1 << 7));
		const uint8_t offset = negative ? ~arg_offset + 1 : arg_offset; // 2's Complement
		if (negative)
			mNextOperationAddress -= offset;
		else
			mNextOperationAddress += offset;
	}

	void CPU::opcode_notimplemented()
	{
		std::cout << "Not implemented: " << mCurrentOpcode->mName << ", at: " << std::hex << mProgramCounter  << std::endl;
	}

	void CPU::opcode_jmp()
	{
		mNextOperationAddress = GMemory->ReadMemoryAddress(mCurrentOperandAddress);
	}

	void CPU::opcode_jsr()
	{
		const uint16_t addr = mNextOperationAddress - 1;
		uint8_t r = addr;
		uint8_t l = addr >> 8;
		StackPush(l);
		StackPush(r); // TODO: Is the order right??

		//const uint16_t memVal = GMemory->ReadMemoryAddress(mCurrentOperandAddress);
		mNextOperationAddress = mCurrentOperandAddress; // TODO: does this really count as "absolute" addressing mode? Or maybe immediate?
	}

	void CPU::opcode_rts()
	{
		uint8_t r = StackPop();
		uint8_t l = StackPop();
		mNextOperationAddress = (r | (((uint16_t)l) << 8)) + 1;
	}

	void CPU::opcode_sei()
	{
		mStatusRegister |= STATUSFLAG_INTERRUPT;
	}

	void CPU::opcode_cli()
	{
		mStatusRegister &= ~STATUSFLAG_INTERRUPT;
	}

	void CPU::opcode_cld()
	{
		mStatusRegister &= ~STATUSFLAG_DECIMAL;
	}

	void CPU::opcode_clc()
	{
		mStatusRegister &= ~STATUSFLAG_CARRY;
	}

	void CPU::opcode_sec()
	{
		mStatusRegister |= STATUSFLAG_CARRY;
	}

	void CPU::opcode_lda()
	{
		ClearFlags(STATUSFLAG_NEGATIVE | STATUSFLAG_ZERO);
		SetZNFlags(mRegA);
		mRegA = GMemory->ReadByte(mCurrentOperandAddress);
	}

	void CPU::opcode_ldx()
	{
		ClearFlags(STATUSFLAG_NEGATIVE | STATUSFLAG_ZERO);
		SetZNFlags(mRegX);
		mRegX = GMemory->ReadByte(mCurrentOperandAddress);
	}

	void CPU::opcode_ldy()
	{
		ClearFlags(STATUSFLAG_NEGATIVE | STATUSFLAG_ZERO);
		SetZNFlags(mRegY);
		mRegY = GMemory->ReadByte(mCurrentOperandAddress);
	}

	void CPU::opcode_sta()
	{
		GMemory->Write(mCurrentOperandAddress, &mRegA, sizeof(mRegA));
	}

	void CPU::opcode_stx()
	{
		GMemory->Write(mCurrentOperandAddress, &mRegX, sizeof(mRegX));
	}

	void CPU::opcode_sty()
	{
		GMemory->Write(mCurrentOperandAddress, &mRegY, sizeof(mRegY));
	}

	void CPU::opcode_inx()
	{
		ClearFlags(STATUSFLAG_NEGATIVE | STATUSFLAG_ZERO);
		mRegX += 1;
		SetZNFlags(mRegX);
	}

	void CPU::opcode_iny()
	{
		ClearFlags(STATUSFLAG_NEGATIVE | STATUSFLAG_ZERO);
		mRegY += 1;
		SetZNFlags(mRegY);
	}

	void CPU::opcode_dey()
	{
		ClearFlags(STATUSFLAG_NEGATIVE | STATUSFLAG_ZERO);
		mRegY -= 1;
		SetZNFlags(mRegY);
	}

	void CPU::opcode_tax()
	{
		ClearFlags(STATUSFLAG_NEGATIVE | STATUSFLAG_ZERO);
		mRegX = mRegA;
		SetZNFlags(mRegX);
	}

	void CPU::opcode_bne()
	{
		if (!GetFlags(STATUSFLAG_ZERO))
		{
			const uint8_t memVal = GMemory->ReadByte(mCurrentOperandAddress);
			Branch(memVal);
		}
	}

	void CPU::opcode_bpl()
	{
		if (!GetFlags(STATUSFLAG_NEGATIVE))
		{
			const uint8_t memVal = GMemory->ReadByte(mCurrentOperandAddress);
			Branch(memVal);
		}
	}

	void CPU::opcode_bit()
	{
		uint8_t val = GMemory->ReadByte(mCurrentOperandAddress);
		SetFlags(STATUSFLAG_ZERO, mRegA & val);

		uint8_t v = val & (1 << 6);
		uint8_t n = val & (1 << 7);

		SetFlags(STATUSFLAG_OVERFLOW, v);
		SetFlags(STATUSFLAG_NEGATIVE, n);
	}

	void CPU::opcode_txs()
	{
		mStackPointer = mRegX;
	}

	void CPU::opcode_tsx()
	{
		mRegX = mStackPointer;
	}

	void CPU::opcode_ora()
	{
		uint8_t val = GMemory->ReadByte(mCurrentOperandAddress);
		mRegA |= val;
		SetZNFlags(mRegA);
	}

	void CPU::opcode_asl()
	{
		const uint8_t val = GMemory->ReadByte(mCurrentOperandAddress);
		uint8_t c = ((1<<7) & val);

		if (c)
			SetFlags(STATUSFLAG_CARRY);
		else
			ClearFlags(STATUSFLAG_CARRY);
	}

	void CPU::opcode_cmp()
	{
		ClearFlags(STATUSFLAG_CARRY | STATUSFLAG_ZERO | STATUSFLAG_NEGATIVE);
		const uint8_t val = GMemory->ReadByte(mCurrentOperandAddress);
		const uint16_t diff = mRegA - val;
		SetZNFlags(diff);
		if (!(diff & 0x100))
			SetFlags(STATUSFLAG_CARRY);
	}



#define SET_OPCODE(index,name,func,addrmode,cycles)\
{\
	Opcode* op = new Opcode();\
	op->mName = name;\
	op->mCallback = func;\
	op->mAddressingMode = addrmode;\
	op->mCycles = cycles;\
	mOpcodeTable[index] = op;\
}

	void CPU::RegisterOpcodes()
	{
		for (int i = 0; i < 256; i++)
		{
			mOpcodeTable[i] = nullptr;
		}

		SET_OPCODE(0x00, "BRK", &CPU::opcode_notimplemented, AddressingMode::Implied, 7);
		SET_OPCODE(0x01, "ORA", &CPU::opcode_ora, AddressingMode::IndirectX, 6);
		SET_OPCODE(0x05, "ORA", &CPU::opcode_ora, AddressingMode::ZeroPage, 2);
		SET_OPCODE(0x06, "ASL", &CPU::opcode_asl, AddressingMode::ZeroPage, 5);
		SET_OPCODE(0x08, "PHP", &CPU::opcode_notimplemented, AddressingMode::Implied, 3);
		SET_OPCODE(0x09, "ORA", &CPU::opcode_ora, AddressingMode::Immediate, 2);
		SET_OPCODE(0x0A, "ASL", &CPU::opcode_asl, AddressingMode::Implied, 2); // TODO: Accumulator
		SET_OPCODE(0x0D, "ORA", &CPU::opcode_ora, AddressingMode::Absolute, 4);
		SET_OPCODE(0x0E, "ASL", &CPU::opcode_asl, AddressingMode::Absolute, 6);

		SET_OPCODE(0x10, "BPL", &CPU::opcode_bpl, AddressingMode::Immediate, 2); // TODO: add cycles if branch is taken
		SET_OPCODE(0x11, "ORA", &CPU::opcode_ora, AddressingMode::IndirectY, 5);
		SET_OPCODE(0x15, "ORA", &CPU::opcode_ora, AddressingMode::ZeroPageX, 3);
		SET_OPCODE(0x16, "ASL", &CPU::opcode_asl, AddressingMode::ZeroPageX, 5);
		SET_OPCODE(0x18, "CLC", &CPU::opcode_clc, AddressingMode::Implied, 2);
		SET_OPCODE(0x19, "ORA", &CPU::opcode_ora, AddressingMode::AbsoluteX, 4);
		SET_OPCODE(0x1D, "ORA", &CPU::opcode_ora, AddressingMode::AbsoluteX, 4);
		SET_OPCODE(0x1E, "ASL", &CPU::opcode_asl, AddressingMode::AbsoluteX, 7);

		SET_OPCODE(0x20, "JSR", &CPU::opcode_jsr, AddressingMode::Absolute, 6);
		SET_OPCODE(0x21, "AND", &CPU::opcode_notimplemented, AddressingMode::IndirectX, 6);
		SET_OPCODE(0x24, "BIT", &CPU::opcode_bit, AddressingMode::ZeroPage, 3);
		SET_OPCODE(0x25, "AND", &CPU::opcode_notimplemented, AddressingMode::ZeroPage, 2);
		SET_OPCODE(0x26, "ROL", &CPU::opcode_notimplemented, AddressingMode::ZeroPage, 5);
		SET_OPCODE(0x28, "PLP", &CPU::opcode_notimplemented, AddressingMode::Implied, 4);
		SET_OPCODE(0x29, "AND", &CPU::opcode_notimplemented, AddressingMode::Immediate, 2);
		SET_OPCODE(0x2A, "ROL", &CPU::opcode_notimplemented, AddressingMode::Implied, 2); // TODO: accumulator
		SET_OPCODE(0x2C, "BIT", &CPU::opcode_bit, AddressingMode::Absolute, 4);
		SET_OPCODE(0x2D, "AND", &CPU::opcode_notimplemented, AddressingMode::Absolute, 4);
		SET_OPCODE(0x2E, "ROL", &CPU::opcode_notimplemented, AddressingMode::Absolute, 6);

		SET_OPCODE(0x30, "BMI", &CPU::opcode_notimplemented, AddressingMode::Immediate, 2); // TODO: add cycles if branch is taken
		SET_OPCODE(0x31, "AND", &CPU::opcode_notimplemented, AddressingMode::IndirectY, 5);
		SET_OPCODE(0x35, "AND", &CPU::opcode_notimplemented, AddressingMode::ZeroPageX, 3);
		SET_OPCODE(0x36, "ROL", &CPU::opcode_notimplemented, AddressingMode::ZeroPageX, 6);
		SET_OPCODE(0x38, "SEC", &CPU::opcode_sec, AddressingMode::Implied, 2);
		SET_OPCODE(0x39, "AND", &CPU::opcode_notimplemented, AddressingMode::AbsoluteX, 4);
		SET_OPCODE(0x3D, "AND", &CPU::opcode_notimplemented, AddressingMode::AbsoluteX, 4);
		SET_OPCODE(0x3E, "ROL", &CPU::opcode_notimplemented, AddressingMode::AbsoluteX, 7);

		SET_OPCODE(0x40, "RTI", &CPU::opcode_notimplemented, AddressingMode::Implied, 6);
		SET_OPCODE(0x41, "EOR", &CPU::opcode_notimplemented, AddressingMode::IndirectX, 6);
		SET_OPCODE(0x45, "EOR", &CPU::opcode_notimplemented, AddressingMode::ZeroPage, 3);
		SET_OPCODE(0x46, "LSR", &CPU::opcode_notimplemented, AddressingMode::ZeroPage, 5);
		SET_OPCODE(0x48, "PHA", &CPU::opcode_notimplemented, AddressingMode::Implied, 3);
		SET_OPCODE(0x49, "EOR", &CPU::opcode_notimplemented, AddressingMode::Immediate, 2);
		SET_OPCODE(0x4A, "LSR", &CPU::opcode_notimplemented, AddressingMode::Implied, 2); // TODO: Accumulator
		SET_OPCODE(0x4C, "JMP", &CPU::opcode_jmp, AddressingMode::Absolute, 3);
		SET_OPCODE(0x4D, "EOR", &CPU::opcode_notimplemented, AddressingMode::Absolute, 4);
		SET_OPCODE(0x4E, "LSR", &CPU::opcode_notimplemented, AddressingMode::Absolute, 6);

		SET_OPCODE(0x50, "BVC", &CPU::opcode_notimplemented, AddressingMode::Immediate, 2); // TODO: add cycles if branch is taken
		SET_OPCODE(0x51, "EOR", &CPU::opcode_notimplemented, AddressingMode::IndirectY, 5);
		SET_OPCODE(0x55, "EOR", &CPU::opcode_notimplemented, AddressingMode::ZeroPageX, 4);
		SET_OPCODE(0x56, "LSR", &CPU::opcode_notimplemented, AddressingMode::ZeroPageX, 6);
		SET_OPCODE(0x58, "CLI", &CPU::opcode_cli, AddressingMode::Implied, 2);
		SET_OPCODE(0x59, "EOR", &CPU::opcode_notimplemented, AddressingMode::AbsoluteY, 4);
		SET_OPCODE(0x5D, "EOR", &CPU::opcode_notimplemented, AddressingMode::AbsoluteX, 4);
		SET_OPCODE(0x5E, "LSR", &CPU::opcode_notimplemented, AddressingMode::AbsoluteX, 7);

		SET_OPCODE(0x60, "RTS", &CPU::opcode_rts, AddressingMode::Implied, 6);
		SET_OPCODE(0x61, "ADC", &CPU::opcode_notimplemented, AddressingMode::IndirectX, 6);
		SET_OPCODE(0x65, "ADC", &CPU::opcode_notimplemented, AddressingMode::ZeroPage, 3);
		SET_OPCODE(0x66, "ROR", &CPU::opcode_notimplemented, AddressingMode::ZeroPage, 5);
		SET_OPCODE(0x68, "PLA", &CPU::opcode_notimplemented, AddressingMode::Implied, 4);
		SET_OPCODE(0x69, "ADC", &CPU::opcode_notimplemented, AddressingMode::Immediate, 2);
		SET_OPCODE(0x6A, "ROR", &CPU::opcode_notimplemented, AddressingMode::Implied, 2); // TODO: accumulator
		SET_OPCODE(0x6C, "JMP", &CPU::opcode_notimplemented, AddressingMode::Indirect, 5);
		SET_OPCODE(0x6D, "ADC", &CPU::opcode_notimplemented, AddressingMode::Absolute, 4);
		SET_OPCODE(0x6E, "ROR", &CPU::opcode_notimplemented, AddressingMode::Absolute, 6);

		SET_OPCODE(0x70, "BVS", &CPU::opcode_notimplemented, AddressingMode::Immediate, 2); // TODO: add cycles if branch is taken
		SET_OPCODE(0x71, "ADC", &CPU::opcode_notimplemented, AddressingMode::IndirectY, 5);
		SET_OPCODE(0x75, "ADC", &CPU::opcode_notimplemented, AddressingMode::ZeroPageX, 4);
		SET_OPCODE(0x76, "ROR", &CPU::opcode_notimplemented, AddressingMode::ZeroPageX, 6);
		SET_OPCODE(0x78, "SEI", &CPU::opcode_sei, AddressingMode::Implied, 2);
		SET_OPCODE(0x79, "ADC", &CPU::opcode_notimplemented, AddressingMode::AbsoluteY, 4);
		SET_OPCODE(0x7D, "ADC", &CPU::opcode_notimplemented, AddressingMode::AbsoluteX, 4);
		SET_OPCODE(0x7E, "ROR", &CPU::opcode_notimplemented, AddressingMode::AbsoluteX, 7);

		SET_OPCODE(0x81, "STA", &CPU::opcode_sta, AddressingMode::IndirectX, 6);
		SET_OPCODE(0x84, "STY", &CPU::opcode_sty, AddressingMode::ZeroPage, 3);
		SET_OPCODE(0x85, "STA", &CPU::opcode_sta, AddressingMode::ZeroPage, 3);
		SET_OPCODE(0x86, "STX", &CPU::opcode_stx, AddressingMode::ZeroPage, 3);
		SET_OPCODE(0x88, "DEY", &CPU::opcode_dey, AddressingMode::Implied, 2);
		SET_OPCODE(0x8A, "TXA", &CPU::opcode_notimplemented, AddressingMode::Implied, 2);
		SET_OPCODE(0x8C, "STY", &CPU::opcode_sty, AddressingMode::Absolute, 4);
		SET_OPCODE(0x8D, "STA", &CPU::opcode_sta, AddressingMode::Absolute, 4);
		SET_OPCODE(0x8E, "STX", &CPU::opcode_stx, AddressingMode::Absolute, 4);

		SET_OPCODE(0x90, "BCC", &CPU::opcode_notimplemented, AddressingMode::Immediate, 2); // TODO: add cycles if branch is taken
		SET_OPCODE(0x91, "STA", &CPU::opcode_sta, AddressingMode::IndirectY, 6);
		SET_OPCODE(0x94, "STY", &CPU::opcode_sty, AddressingMode::ZeroPageX, 4);
		SET_OPCODE(0x95, "STA", &CPU::opcode_sta, AddressingMode::ZeroPageX, 4);
		SET_OPCODE(0x96, "STX", &CPU::opcode_stx, AddressingMode::ZeroPageY, 4);
		SET_OPCODE(0x98, "TYA", &CPU::opcode_notimplemented, AddressingMode::Implied, 2);
		SET_OPCODE(0x99, "STA", &CPU::opcode_sta, AddressingMode::AbsoluteY, 5);
		SET_OPCODE(0x9A, "TXS", &CPU::opcode_txs, AddressingMode::Implied, 2);
		SET_OPCODE(0x9D, "STA", &CPU::opcode_sta, AddressingMode::AbsoluteX, 5);

		SET_OPCODE(0xA0, "LDY", &CPU::opcode_ldy, AddressingMode::Immediate, 2);
		SET_OPCODE(0xA1, "LDA", &CPU::opcode_lda, AddressingMode::IndirectX, 6);
		SET_OPCODE(0xA2, "LDX", &CPU::opcode_ldx, AddressingMode::Immediate, 2);
		SET_OPCODE(0xA4, "LDY", &CPU::opcode_ldy, AddressingMode::ZeroPage, 3);
		SET_OPCODE(0xA5, "LDA", &CPU::opcode_lda, AddressingMode::ZeroPage, 3);
		SET_OPCODE(0xA6, "LDX", &CPU::opcode_ldx, AddressingMode::ZeroPage, 3);
		SET_OPCODE(0xA8, "TAY", &CPU::opcode_notimplemented, AddressingMode::Implied, 2);
		SET_OPCODE(0xA9, "LDA", &CPU::opcode_lda, AddressingMode::Immediate, 2);
		SET_OPCODE(0xAA, "TAX", &CPU::opcode_notimplemented, AddressingMode::Implied, 2);
		SET_OPCODE(0xAC, "LDY", &CPU::opcode_ldy, AddressingMode::Absolute, 4);
		SET_OPCODE(0xAD, "LDA", &CPU::opcode_lda, AddressingMode::Absolute, 4);
		SET_OPCODE(0xAE, "LDX", &CPU::opcode_ldx, AddressingMode::Absolute, 4);

		SET_OPCODE(0xB0, "BCS", &CPU::opcode_notimplemented, AddressingMode::Immediate, 2); // TODO: add cycles if branch is taken
		SET_OPCODE(0xB1, "LDA", &CPU::opcode_lda, AddressingMode::IndirectY, 5);
		SET_OPCODE(0xB4, "LDY", &CPU::opcode_ldy, AddressingMode::ZeroPageX, 4);
		SET_OPCODE(0xB5, "LDA", &CPU::opcode_lda, AddressingMode::ZeroPageX, 4);
		SET_OPCODE(0xB6, "LDX", &CPU::opcode_ldx, AddressingMode::ZeroPageY, 4);
		SET_OPCODE(0xB8, "CLV", &CPU::opcode_notimplemented, AddressingMode::Implied, 2);
		SET_OPCODE(0xB9, "LDA", &CPU::opcode_lda, AddressingMode::AbsoluteY, 4);
		SET_OPCODE(0xBA, "TSX", &CPU::opcode_tsx, AddressingMode::Implied, 2);
		SET_OPCODE(0xBC, "LDY", &CPU::opcode_ldy, AddressingMode::AbsoluteX, 4);
		SET_OPCODE(0xBD, "LDA", &CPU::opcode_lda, AddressingMode::AbsoluteX, 4);
		SET_OPCODE(0xBE, "LDX", &CPU::opcode_ldx, AddressingMode::AbsoluteY, 4);

		SET_OPCODE(0xC0, "CPY", &CPU::opcode_notimplemented, AddressingMode::Immediate, 2);
		SET_OPCODE(0xC1, "CMP", &CPU::opcode_cmp, AddressingMode::IndirectX, 6);
		SET_OPCODE(0xC4, "CPY", &CPU::opcode_notimplemented, AddressingMode::ZeroPage, 3);
		SET_OPCODE(0xC5, "CMP", &CPU::opcode_cmp, AddressingMode::ZeroPage, 3);
		SET_OPCODE(0xC6, "DEC", &CPU::opcode_notimplemented, AddressingMode::ZeroPage, 5);
		SET_OPCODE(0xC8, "INY", &CPU::opcode_iny, AddressingMode::Implied, 2);
		SET_OPCODE(0xC9, "CMP", &CPU::opcode_cmp, AddressingMode::Immediate, 2);
		SET_OPCODE(0xCA, "DEX", &CPU::opcode_notimplemented, AddressingMode::Implied, 2);
		SET_OPCODE(0xCC, "CPY", &CPU::opcode_notimplemented, AddressingMode::Absolute, 4);
		SET_OPCODE(0xCD, "CMP", &CPU::opcode_cmp, AddressingMode::Absolute, 4);
		SET_OPCODE(0xCE, "DEC", &CPU::opcode_notimplemented, AddressingMode::Absolute, 6);

		SET_OPCODE(0xD0, "BNE", &CPU::opcode_bne, AddressingMode::Immediate, 2); // TODO: add cycles if branch is taken
		SET_OPCODE(0xD1, "CMP", &CPU::opcode_cmp, AddressingMode::IndirectY, 5);
		SET_OPCODE(0xD5, "CMP", &CPU::opcode_cmp, AddressingMode::ZeroPageX, 4);
		SET_OPCODE(0xD6, "DEC", &CPU::opcode_notimplemented, AddressingMode::ZeroPageX, 6);
		SET_OPCODE(0xD8, "CLD", &CPU::opcode_cld, AddressingMode::Implied, 2);
		SET_OPCODE(0xD9, "CMP", &CPU::opcode_cmp, AddressingMode::AbsoluteY, 4);
		SET_OPCODE(0xDD, "CMP", &CPU::opcode_cmp, AddressingMode::AbsoluteX, 4);
		SET_OPCODE(0xDE, "DEC", &CPU::opcode_notimplemented, AddressingMode::AbsoluteX, 7);

		SET_OPCODE(0xE0, "CPX", &CPU::opcode_notimplemented, AddressingMode::Immediate, 2);
		SET_OPCODE(0xE1, "SBC", &CPU::opcode_notimplemented, AddressingMode::IndirectX, 6);
		SET_OPCODE(0xE4, "CPX", &CPU::opcode_notimplemented, AddressingMode::ZeroPage, 3);
		SET_OPCODE(0xE5, "SBC", &CPU::opcode_notimplemented, AddressingMode::ZeroPage, 3);
		SET_OPCODE(0xE6, "INC", &CPU::opcode_notimplemented, AddressingMode::ZeroPage, 5);
		SET_OPCODE(0xE8, "INX", &CPU::opcode_inx, AddressingMode::Implied, 2);
		SET_OPCODE(0xE9, "SBC", &CPU::opcode_notimplemented, AddressingMode::Immediate, 2);
		SET_OPCODE(0xEA, "NOP", &CPU::opcode_notimplemented, AddressingMode::Implied, 2);
		SET_OPCODE(0xEC, "CPX", &CPU::opcode_notimplemented, AddressingMode::Absolute, 4);
		SET_OPCODE(0xED, "SBC", &CPU::opcode_notimplemented, AddressingMode::Absolute, 4);
		SET_OPCODE(0xEE, "INC", &CPU::opcode_notimplemented, AddressingMode::Absolute, 6);

		SET_OPCODE(0xF0, "BEQ", &CPU::opcode_notimplemented, AddressingMode::Immediate, 2); // TODO: add cycles if branch is taken
		SET_OPCODE(0xF1, "SBC", &CPU::opcode_notimplemented, AddressingMode::IndirectY, 5);
		SET_OPCODE(0xF5, "SBC", &CPU::opcode_notimplemented, AddressingMode::ZeroPageX, 4);
		SET_OPCODE(0xF6, "INC", &CPU::opcode_notimplemented, AddressingMode::ZeroPageX, 6);
		SET_OPCODE(0xF8, "SED", &CPU::opcode_notimplemented, AddressingMode::Implied, 2);
		SET_OPCODE(0xF9, "SBC", &CPU::opcode_notimplemented, AddressingMode::AbsoluteY, 4);
		SET_OPCODE(0xFD, "SBC", &CPU::opcode_notimplemented, AddressingMode::AbsoluteX, 4);
		SET_OPCODE(0xFE, "INC", &CPU::opcode_notimplemented, AddressingMode::AbsoluteX, 7);

	}
}
