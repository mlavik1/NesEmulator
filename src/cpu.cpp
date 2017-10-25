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

		mNMILabel = GMemory->ReadWord(0xFFFA);
		mResetLabel = GMemory->ReadWord(0xFFFC);
		mIRQLabel = GMemory->ReadWord(0xFFFE);

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
		if (opcode != nullptr)
		{
			mCurrentCycles += opcode->mCycles;
			// TODO: Add extra cycle for indexed addressing mode if page boundary was crossed
			
			auto callback = opcode->mCallback;
			if (callback != nullptr)
			{
				// Get operand address
				const uint16_t address = DecodeOperandAddress(mProgramCounter + 1, opcode->mAddressingMode);
				
				mCurrentOpcodeValue = address;

				// Execute the instruction
				(this->*callback)();
			}

			// instruction length
			mProgramCounter += 1;

			// operand length
			if (opcode->mAddressingMode == AddressingMode::Absolute || opcode->mAddressingMode == AddressingMode::AbsoluteX || opcode->mAddressingMode == AddressingMode::AbsoluteY)
			{
				mProgramCounter += 2;
			}
			else if(opcode->mAddressingMode != AddressingMode::Implied)
			{
				mProgramCounter += 1;
			}
			
		}
	}

	void CPU::ClearFlags(statusflag_t flags)
	{
		mStatusRegister &= ~flags;
	}

	void CPU::SetFlags(statusflag_t flags)
	{
		mStatusRegister |= flags;
	}

	void CPU::SetZNFlagsOf(uint8_t arg_value)
	{
		if ((int16_t)arg_value <0)
		{
			SetFlags(STATUSFLAG_NEGATIVE);
		}
		else if ((int16_t)arg_value == 0)
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
			outAddress = GMemory->ReadMemoryAddress(arg_addr + mRegX);
			break;
		case AddressingMode::AbsoluteY:
			return GMemory->ReadMemoryAddress(arg_addr + mRegY);
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
		default:
			std::cout << "Unhandled AddressingMode" << std::endl;
			break;
		}

		return outAddress;
	}

	void CPU::opcode_notimplemented()
	{

	}

	void CPU::opcode_jmp()
	{
		mProgramCounter = GMemory->ReadMemoryAddress(mCurrentOpcodeValue);
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
		mStatusRegister &= STATUSFLAG_DECIMAL;
	}

	void CPU::opcode_ldx()
	{
		ClearFlags(STATUSFLAG_NEGATIVE | STATUSFLAG_ZERO);
		SetZNFlagsOf(mRegX);
		mRegX = GMemory->ReadByte(mCurrentOpcodeValue);
	}

	void CPU::opcode_ldy()
	{
		ClearFlags(STATUSFLAG_NEGATIVE | STATUSFLAG_ZERO);
		SetZNFlagsOf(mRegY);
		mRegY = GMemory->ReadByte(mCurrentOpcodeValue);
	}

	void CPU::opcode_lda()
	{
		ClearFlags(STATUSFLAG_NEGATIVE | STATUSFLAG_ZERO);
		SetZNFlagsOf(mRegA);
		mRegA = GMemory->ReadByte(mCurrentOpcodeValue);
	}

	void CPU::opcode_sta()
	{
		GMemory->Write(mCurrentOpcodeValue, &mRegA, sizeof(mRegA));
	}

	void CPU::opcode_inx()
	{
		ClearFlags(STATUSFLAG_NEGATIVE | STATUSFLAG_ZERO);
		mRegX += 1;
		SetZNFlagsOf(mRegX);
	}

	void CPU::opcode_dey()
	{
		ClearFlags(STATUSFLAG_NEGATIVE | STATUSFLAG_ZERO);
		mRegY -= 1;
		SetZNFlagsOf(mRegY);
	}

	void CPU::opcode_bne()
	{
		if (!GetFlags(STATUSFLAG_ZERO))
		{
			mProgramCounter += GMemory->ReadByte(mCurrentOpcodeValue);
		}
	}

	void CPU::opcode_bpl()
	{
		if (!GetFlags(STATUSFLAG_NEGATIVE))
		{
			mProgramCounter += GMemory->ReadByte(mCurrentOpcodeValue);
		}
	}

	void CPU::opcode_bit()
	{
		std::cout << "UNHANDLED opcode_bit" << std::endl;
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
		SET_OPCODE(0x01, "ORA", &CPU::opcode_notimplemented, AddressingMode::IndirectX, 6);
		SET_OPCODE(0x02, "", &CPU::opcode_notimplemented, AddressingMode::Implied, 0);
		SET_OPCODE(0x03, "", &CPU::opcode_notimplemented, AddressingMode::Implied, 0);
		SET_OPCODE(0x04, "", &CPU::opcode_notimplemented, AddressingMode::Implied, 0);
		SET_OPCODE(0x05, "ORA", &CPU::opcode_notimplemented, AddressingMode::ZeroPage, 2);
		SET_OPCODE(0x06, "ASL", &CPU::opcode_notimplemented, AddressingMode::ZeroPage, 5);
		SET_OPCODE(0x07, "", &CPU::opcode_notimplemented, AddressingMode::Immediate, 0);
		SET_OPCODE(0x08, "PHP", &CPU::opcode_notimplemented, AddressingMode::Implied, 3);
		SET_OPCODE(0x09, "ORA", &CPU::opcode_notimplemented, AddressingMode::Immediate, 2);
		SET_OPCODE(0x0A, "ASL", &CPU::opcode_notimplemented, AddressingMode::Implied, 2); // TODO: Accumulator
		SET_OPCODE(0x0B, "", &CPU::opcode_notimplemented, AddressingMode::Implied, 0);
		SET_OPCODE(0x0C, "", &CPU::opcode_notimplemented, AddressingMode::Implied, 0);
		SET_OPCODE(0x0D, "ORA", &CPU::opcode_notimplemented, AddressingMode::Absolute, 4);
		SET_OPCODE(0x0E, "ASL", &CPU::opcode_notimplemented, AddressingMode::Absolute, 6);
		SET_OPCODE(0x0F, "", &CPU::opcode_notimplemented, AddressingMode::Implied, 0);

		SET_OPCODE(0x10, "BPL", &CPU::opcode_bpl, AddressingMode::Immediate, 2); // TODO: add cycles if branch is taken
		SET_OPCODE(0x11, "ORA", &CPU::opcode_notimplemented, AddressingMode::IndirectY, 5);
		SET_OPCODE(0x12, "", &CPU::opcode_notimplemented, AddressingMode::Implied, 0);
		SET_OPCODE(0x13, "", &CPU::opcode_notimplemented, AddressingMode::Implied, 0);
		SET_OPCODE(0x14, "", &CPU::opcode_notimplemented, AddressingMode::Implied, 0);
		SET_OPCODE(0x15, "ORA", &CPU::opcode_notimplemented, AddressingMode::ZeroPageX, 3);
		SET_OPCODE(0x16, "ASL", &CPU::opcode_notimplemented, AddressingMode::ZeroPageX, 5);
		SET_OPCODE(0x17, "", &CPU::opcode_notimplemented, AddressingMode::Implied, 0);
		SET_OPCODE(0x18, "CLC", &CPU::opcode_notimplemented, AddressingMode::Implied, 2);
		SET_OPCODE(0x19, "ORA", &CPU::opcode_notimplemented, AddressingMode::AbsoluteX, 4);
		SET_OPCODE(0x1A, "", &CPU::opcode_notimplemented, AddressingMode::Implied, 0);
		SET_OPCODE(0x1B, "", &CPU::opcode_notimplemented, AddressingMode::Implied, 0);
		SET_OPCODE(0x1C, "", &CPU::opcode_notimplemented, AddressingMode::Implied, 0);
		SET_OPCODE(0x1D, "ORA", &CPU::opcode_notimplemented, AddressingMode::AbsoluteX, 4);
		SET_OPCODE(0x1E, "ASL", &CPU::opcode_notimplemented, AddressingMode::AbsoluteX, 7);
		SET_OPCODE(0x1F, "", &CPU::opcode_notimplemented, AddressingMode::Implied, 0);

		SET_OPCODE(0x20, "JSR", &CPU::opcode_notimplemented, AddressingMode::Absolute, 6);
		SET_OPCODE(0x21, "AND", &CPU::opcode_notimplemented, AddressingMode::IndirectX, 6);
		SET_OPCODE(0x22, "", &CPU::opcode_notimplemented, AddressingMode::Implied, 0);
		SET_OPCODE(0x23, "", &CPU::opcode_notimplemented, AddressingMode::Implied, 0);
		SET_OPCODE(0x24, "BIT", &CPU::opcode_bit, AddressingMode::ZeroPage, 3);
		SET_OPCODE(0x25, "AND", &CPU::opcode_notimplemented, AddressingMode::ZeroPage, 2);
		SET_OPCODE(0x26, "ROL", &CPU::opcode_notimplemented, AddressingMode::ZeroPage, 5);
		SET_OPCODE(0x27, "", &CPU::opcode_notimplemented, AddressingMode::Implied, 0);
		SET_OPCODE(0x28, "PLP", &CPU::opcode_notimplemented, AddressingMode::Implied, 4);
		SET_OPCODE(0x29, "AND", &CPU::opcode_notimplemented, AddressingMode::Immediate, 2);
		SET_OPCODE(0x2A, "ROL", &CPU::opcode_notimplemented, AddressingMode::Implied, 2); // TODO: accumulator
		SET_OPCODE(0x2B, "", &CPU::opcode_notimplemented, AddressingMode::Implied, 0);
		SET_OPCODE(0x2C, "BIT", &CPU::opcode_bit, AddressingMode::Absolute, 4);
		SET_OPCODE(0x2D, "AND", &CPU::opcode_notimplemented, AddressingMode::Absolute, 4);
		SET_OPCODE(0x2E, "ROL", &CPU::opcode_notimplemented, AddressingMode::Absolute, 6);
		SET_OPCODE(0x2F, "", &CPU::opcode_notimplemented, AddressingMode::Implied, 0);

		SET_OPCODE(0x30, "BMI", &CPU::opcode_notimplemented, AddressingMode::Immediate, 2); // TODO: add cycles if branch is taken
		SET_OPCODE(0x31, "AND", &CPU::opcode_notimplemented, AddressingMode::IndirectY, 5);
		SET_OPCODE(0x32, "", &CPU::opcode_notimplemented, AddressingMode::Implied, 0);
		SET_OPCODE(0x33, "", &CPU::opcode_notimplemented, AddressingMode::Implied, 0);
		SET_OPCODE(0x34, "", &CPU::opcode_notimplemented, AddressingMode::Implied, 0);
		SET_OPCODE(0x35, "AND", &CPU::opcode_notimplemented, AddressingMode::ZeroPageX, 3);
		SET_OPCODE(0x36, "ROL", &CPU::opcode_notimplemented, AddressingMode::ZeroPageX, 6);
		SET_OPCODE(0x37, "", &CPU::opcode_notimplemented, AddressingMode::Implied, 0);
		SET_OPCODE(0x38, "SEC", &CPU::opcode_notimplemented, AddressingMode::Implied, 2);
		SET_OPCODE(0x39, "AND", &CPU::opcode_notimplemented, AddressingMode::AbsoluteX, 4);
		SET_OPCODE(0x3A, "", &CPU::opcode_notimplemented, AddressingMode::Implied, 0);
		SET_OPCODE(0x3B, "", &CPU::opcode_notimplemented, AddressingMode::Implied, 0);
		SET_OPCODE(0x3C, "", &CPU::opcode_notimplemented, AddressingMode::Implied, 0);
		SET_OPCODE(0x3D, "AND", &CPU::opcode_notimplemented, AddressingMode::AbsoluteX, 4);
		SET_OPCODE(0x3E, "ROL", &CPU::opcode_notimplemented, AddressingMode::AbsoluteX, 7);
		SET_OPCODE(0x3F, "", &CPU::opcode_notimplemented, AddressingMode::Implied, 0);

		SET_OPCODE(0x40, "RTI", &CPU::opcode_notimplemented, AddressingMode::Implied, 6);
		SET_OPCODE(0x41, "EOR", &CPU::opcode_notimplemented, AddressingMode::IndirectX, 6);
		SET_OPCODE(0x42, "", &CPU::opcode_notimplemented, AddressingMode::Implied, 0);
		SET_OPCODE(0x43, "", &CPU::opcode_notimplemented, AddressingMode::Implied, 0);
		SET_OPCODE(0x44, "", &CPU::opcode_notimplemented, AddressingMode::Implied, 0);
		SET_OPCODE(0x45, "EOR", &CPU::opcode_notimplemented, AddressingMode::ZeroPage, 3);
		SET_OPCODE(0x46, "LSR", &CPU::opcode_notimplemented, AddressingMode::ZeroPage, 5);
		SET_OPCODE(0x47, "", &CPU::opcode_notimplemented, AddressingMode::Implied, 0);
		SET_OPCODE(0x48, "PHA", &CPU::opcode_notimplemented, AddressingMode::Implied, 3);
		SET_OPCODE(0x49, "EOR", &CPU::opcode_notimplemented, AddressingMode::Immediate, 2);
		SET_OPCODE(0x4A, "LSR", &CPU::opcode_notimplemented, AddressingMode::Implied, 2); // TODO: Accumulator
		SET_OPCODE(0x4B, "", &CPU::opcode_notimplemented, AddressingMode::Implied, 0);
		SET_OPCODE(0x4C, "JMP", &CPU::opcode_jmp, AddressingMode::Absolute, 3);
		SET_OPCODE(0x4D, "EOR", &CPU::opcode_notimplemented, AddressingMode::Absolute, 4);
		SET_OPCODE(0x4E, "LSR", &CPU::opcode_notimplemented, AddressingMode::Absolute, 6);
		SET_OPCODE(0x4F, "", &CPU::opcode_notimplemented, AddressingMode::Implied, 0);

		SET_OPCODE(0x50, "BVC",		&CPU::opcode_notimplemented,	AddressingMode::Immediate, 2); // TODO: add cycles if branch is taken
		SET_OPCODE(0x51, "EOR",		&CPU::opcode_notimplemented,	AddressingMode::IndirectY, 5);
		SET_OPCODE(0x52, "",		&CPU::opcode_notimplemented,	AddressingMode::Implied, 0);
		SET_OPCODE(0x53, "",		&CPU::opcode_notimplemented,	AddressingMode::Implied, 0);
		SET_OPCODE(0x54, "",		&CPU::opcode_notimplemented,	AddressingMode::Implied, 0);
		SET_OPCODE(0x55, "EOR",		&CPU::opcode_notimplemented,	AddressingMode::ZeroPageX, 4);
		SET_OPCODE(0x56, "LSR",		&CPU::opcode_notimplemented,	AddressingMode::ZeroPageX, 6);
		SET_OPCODE(0x57, "",		&CPU::opcode_notimplemented,	AddressingMode::Implied, 0);
		SET_OPCODE(0x58, "CLI",		&CPU::opcode_cli,				AddressingMode::Implied, 2);
		SET_OPCODE(0x59, "EOR",		&CPU::opcode_notimplemented,	AddressingMode::AbsoluteY, 4);
		SET_OPCODE(0x5A, "",		&CPU::opcode_notimplemented,	AddressingMode::Implied, 0);
		SET_OPCODE(0x5B, "",		&CPU::opcode_notimplemented,	AddressingMode::Implied, 0);
		SET_OPCODE(0x5C, "",		&CPU::opcode_notimplemented,	AddressingMode::Implied, 0);
		SET_OPCODE(0x5D, "EOR",		&CPU::opcode_notimplemented,	AddressingMode::AbsoluteX, 4);
		SET_OPCODE(0x5E, "LSR",		&CPU::opcode_notimplemented,	AddressingMode::AbsoluteX, 7);
		SET_OPCODE(0x5F, "",		&CPU::opcode_notimplemented,	AddressingMode::Implied, 0);

		SET_OPCODE(0x60, "RTS",		&CPU::opcode_notimplemented,	AddressingMode::Implied, 6);
		SET_OPCODE(0x61, "ADC",		&CPU::opcode_notimplemented,	AddressingMode::IndirectX, 6);
		SET_OPCODE(0x62, "",		&CPU::opcode_notimplemented,	AddressingMode::Implied, 0);
		SET_OPCODE(0x63, "",		&CPU::opcode_notimplemented,	AddressingMode::Implied, 0);
		SET_OPCODE(0x64, "",		&CPU::opcode_notimplemented,	AddressingMode::Implied, 0);
		SET_OPCODE(0x65, "ADC",		&CPU::opcode_notimplemented,	AddressingMode::ZeroPage, 3);
		SET_OPCODE(0x66, "ROR",		&CPU::opcode_notimplemented,	AddressingMode::ZeroPage, 5);
		SET_OPCODE(0x67, "",		&CPU::opcode_notimplemented,	AddressingMode::Implied, 0);
		SET_OPCODE(0x68, "PLA",		&CPU::opcode_notimplemented,	AddressingMode::Implied, 4);
		SET_OPCODE(0x69, "ADC",		&CPU::opcode_notimplemented,	AddressingMode::Immediate, 2);
		SET_OPCODE(0x6A, "ROR",		&CPU::opcode_notimplemented,	AddressingMode::Implied, 2); // TODO: accumulator
		SET_OPCODE(0x6B, "",		&CPU::opcode_notimplemented,	AddressingMode::Implied, 0);
		SET_OPCODE(0x6C, "JMP",		&CPU::opcode_notimplemented,	AddressingMode::Indirect, 5);
		SET_OPCODE(0x6D, "ADC",		&CPU::opcode_notimplemented,	AddressingMode::Absolute, 4);
		SET_OPCODE(0x6E, "ROR",		&CPU::opcode_notimplemented,	AddressingMode::Absolute, 6);
		SET_OPCODE(0x6F, "",		&CPU::opcode_notimplemented,	AddressingMode::Implied, 0);

		SET_OPCODE(0x70, "BVS",		&CPU::opcode_notimplemented,	AddressingMode::Immediate, 2); // TODO: add cycles if branch is taken
		SET_OPCODE(0x71, "ADC",		&CPU::opcode_notimplemented,	AddressingMode::IndirectY, 5);
		SET_OPCODE(0x72, "",		&CPU::opcode_notimplemented,	AddressingMode::Implied, 0);
		SET_OPCODE(0x73, "",		&CPU::opcode_notimplemented,	AddressingMode::Implied, 0);
		SET_OPCODE(0x74, "",		&CPU::opcode_notimplemented,	AddressingMode::Implied, 0);
		SET_OPCODE(0x75, "ADC",		&CPU::opcode_notimplemented,	AddressingMode::ZeroPageX, 4);
		SET_OPCODE(0x76, "ROR",		&CPU::opcode_notimplemented,	AddressingMode::ZeroPageX, 6);
		SET_OPCODE(0x77, "",		&CPU::opcode_notimplemented,	AddressingMode::Implied, 0);
		SET_OPCODE(0x78, "SEI",		&CPU::opcode_sei,				AddressingMode::Implied, 2);
		SET_OPCODE(0x79, "ADC",		&CPU::opcode_notimplemented,	AddressingMode::AbsoluteY, 4);
		SET_OPCODE(0x7A, "",		&CPU::opcode_notimplemented,	AddressingMode::Implied, 0);
		SET_OPCODE(0x7B, "",		&CPU::opcode_notimplemented,	AddressingMode::Implied, 0);
		SET_OPCODE(0x7C, "",		&CPU::opcode_notimplemented,	AddressingMode::Implied, 0);
		SET_OPCODE(0x7D, "ADC",		&CPU::opcode_notimplemented,	AddressingMode::AbsoluteX, 4);
		SET_OPCODE(0x7E, "ROR",		&CPU::opcode_notimplemented,	AddressingMode::AbsoluteX, 7);
		SET_OPCODE(0x7F, "",		&CPU::opcode_notimplemented,	AddressingMode::Implied, 0);

		SET_OPCODE(0x80, "",		&CPU::opcode_notimplemented,	AddressingMode::Implied, 0);
		SET_OPCODE(0x81, "STA",		&CPU::opcode_sta,				AddressingMode::IndirectX, 6);
		SET_OPCODE(0x82, "",		&CPU::opcode_notimplemented,	AddressingMode::Implied, 0);
		SET_OPCODE(0x83, "",		&CPU::opcode_notimplemented,	AddressingMode::Implied, 0);
		SET_OPCODE(0x84, "STY",		&CPU::opcode_notimplemented,	AddressingMode::ZeroPage, 3);
		SET_OPCODE(0x85, "STA",		&CPU::opcode_sta,				AddressingMode::ZeroPage, 3);
		SET_OPCODE(0x86, "STX",		&CPU::opcode_notimplemented,	AddressingMode::ZeroPage, 3);
		SET_OPCODE(0x87, "",		&CPU::opcode_notimplemented,	AddressingMode::Implied, 0);
		SET_OPCODE(0x88, "DEY",		&CPU::opcode_dey,				AddressingMode::Implied, 2);
		SET_OPCODE(0x89, "",		&CPU::opcode_notimplemented,	AddressingMode::Implied, 0);
		SET_OPCODE(0x8A, "TXA",		&CPU::opcode_notimplemented,	AddressingMode::Implied, 2);
		SET_OPCODE(0x8B, "",		&CPU::opcode_notimplemented,	AddressingMode::Implied, 0);
		SET_OPCODE(0x8C, "STY",		&CPU::opcode_notimplemented,	AddressingMode::Absolute, 4);
		SET_OPCODE(0x8D, "STA",		&CPU::opcode_sta,				AddressingMode::Absolute, 4);
		SET_OPCODE(0x8E, "STX",		&CPU::opcode_notimplemented,	AddressingMode::Absolute, 4);
		SET_OPCODE(0x8F, "",		&CPU::opcode_notimplemented,	AddressingMode::Implied, 0);
		/*
		SET_OPCODE(0x90, "BCC",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0x91, "STA",		&CPU::opcode_sta,				memaddr_indY);
		SET_OPCODE(0x92, "",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0x93, "",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0x94, "STY",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0x95, "STA",		&CPU::opcode_sta,				AddressingMode::ZeroPageX);
		SET_OPCODE(0x96, "STX",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0x97, "",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0x98, "TYA",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0x99, "STA",		&CPU::opcode_sta,				AddressingMode::AbsoluteY);
		SET_OPCODE(0x9A, "TXS",		&CPU::opcode_notimplemented,	memaddr_none);
		SET_OPCODE(0x9B, "",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0x9C, "",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0x9D, "STA",		&CPU::opcode_sta,				AddressingMode::AbsoluteX);
		SET_OPCODE(0x9E, "",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0x9F, "",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);

		SET_OPCODE(0xA0, "LDY",		&CPU::opcode_ldy,				AddressingMode::Immediate);
		SET_OPCODE(0xA1, "LDA",		&CPU::opcode_lda,				memaddr_indX);
		SET_OPCODE(0xA2, "LDX",		&CPU::opcode_ldx,				AddressingMode::Immediate);
		SET_OPCODE(0xA3, "",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xA4, "LDY",		&CPU::opcode_ldy,				AddressingMode::ZeroPage);
		SET_OPCODE(0xA5, "LDA",		&CPU::opcode_lda,				AddressingMode::ZeroPage);
		SET_OPCODE(0xA6, "LDX",		&CPU::opcode_ldx,				AddressingMode::ZeroPage);
		SET_OPCODE(0xA7, "",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xA8, "TAY",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xA9, "LDA",		&CPU::opcode_lda,				AddressingMode::Immediate);
		SET_OPCODE(0xAA, "TAX",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xAB, "",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xAC, "LDY",		&CPU::opcode_ldy,				AddressingMode::Absolute);
		SET_OPCODE(0xAD, "LDA",		&CPU::opcode_lda,				AddressingMode::Absolute);
		SET_OPCODE(0xAE, "LDX",		&CPU::opcode_ldx,				AddressingMode::Absolute);
		SET_OPCODE(0xAF, "",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);

		SET_OPCODE(0xB0, "BCS",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xB1, "LDA",		&CPU::opcode_lda,				memaddr_indY);
		SET_OPCODE(0xB2, "",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xB3, "",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xB4, "LDY",		&CPU::opcode_ldy,				AddressingMode::ZeroPageX);
		SET_OPCODE(0xB5, "LDA",		&CPU::opcode_lda,				AddressingMode::ZeroPageX);
		SET_OPCODE(0xB6, "LDX",		&CPU::opcode_ldx,				AddressingMode::ZeroPageY);
		SET_OPCODE(0xB7, "",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xB8, "CLV",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xB9, "LDA",		&CPU::opcode_lda,				AddressingMode::AbsoluteY);
		SET_OPCODE(0xBA, "TSX",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xBB, "",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xBC, "LDY",		&CPU::opcode_ldy,				AddressingMode::AbsoluteX);
		SET_OPCODE(0xBD, "LDA",		&CPU::opcode_lda,				AddressingMode::AbsoluteX);
		SET_OPCODE(0xBE, "LDX",		&CPU::opcode_ldx,				AddressingMode::Absolute);
		SET_OPCODE(0xBF, "",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);

		SET_OPCODE(0xC0, "CPY",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xC1, "CMP",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xC2, "",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xC3, "",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xC4, "CPY",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xC5, "CMP",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xC6, "DEC",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xC7, "",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xC8, "INY",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xC9, "CMP",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xCA, "DEX",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xCB, "",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xCC, "CPY",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xCD, "CMP",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xCE, "DEC",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xCF, "",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);

		SET_OPCODE(0xD0, "BNE",		&CPU::opcode_bne,				AddressingMode::Immediate);
		SET_OPCODE(0xD1, "CMP",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xD2, "",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xD3, "",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xD4, "",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xD5, "CMP",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xD6, "DEC",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xD7, "",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xD8, "CLD",		&CPU::opcode_cld,				AddressingMode::Absolute);
		SET_OPCODE(0xD9, "CMP",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xDA, "",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xDB, "",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xDC, "",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xDD, "CMP",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xDE, "DEC",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xDF, "",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);

		SET_OPCODE(0xE0, "CPX",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xE1, "SBC",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xE2, "",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xE3, "",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xE4, "CPX",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xE5, "SBC",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xE6, "INC",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xE7, "",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xE8, "INX",		&CPU::opcode_inx,				memaddr_none);
		SET_OPCODE(0xE9, "SBC",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xEA, "NOP",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xEB, "",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xEC, "CPX",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xED, "SBC",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xEE, "INC",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xEF, "",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);

		SET_OPCODE(0xF0, "BEQ",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xF1, "SBC",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xF2, "",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xF3, "",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xF4, "",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xF5, "SBC",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xF6, "INC",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xF7, "",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xF8, "SED",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xF9, "SBC",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xFA, "",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xFB, "",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xFC, "",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xFD, "SBC",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xFE, "INC",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		SET_OPCODE(0xFF, "",		&CPU::opcode_notimplemented,	AddressingMode::Absolute);
		*/

}
