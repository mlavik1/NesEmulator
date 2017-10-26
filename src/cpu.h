#ifndef NESEMU_CPU_H
#define NESEMU_CPU_H

#include <string>
#include <stdint.h>

typedef unsigned int statusflag_t;
#define STATUSFLAG_NEGATIVE		128
#define STATUSFLAG_OVERFLOW		64
#define STATUSFLAG_DECIMAL		8
#define STATUSFLAG_INTERRUPT	4
#define STATUSFLAG_ZERO			2
#define STATUSFLAG_CARRY		1

namespace nesemu
{
	enum AddressingMode
	{
		Immediate,	// uses value directly (no memory lookup)
		ZeroPage,	// first 256 bytes
		ZeroPageX,
		ZeroPageY,
		Absolute,	// full 16 bit address - LSB MSB
		AbsoluteX,
		AbsoluteY,
		Indirect,
		IndirectX,
		IndirectY,
		Implied
	};

	class CPU; // fwd.decl.

	struct Opcode
	{
		std::string mName;
		void(CPU::*mCallback)();
		AddressingMode mAddressingMode;
		int mCycles;
	};

	class CPU
	{
	private:
		uint8_t mRegA;
		uint8_t mRegX;
		uint8_t mRegY;
		uint8_t mStatusRegister;
		uint16_t mProgramCounter;
		uint8_t mStackPointer;

		Opcode* mCurrentOpcode;
		uint16_t mCurrentOperandAddress;
		int mCurrentCycles = 0;

		uint16_t mNMILabel;
		uint16_t mIRQLabel;
		uint16_t mResetLabel;

		Opcode* mOpcodeTable[256];

		void ClearFlags(statusflag_t flags);
		void SetFlags(statusflag_t flags);
		void SetFlags(statusflag_t flags, bool arg_set);
		void SetZNFlags(uint16_t arg_value);
		bool GetFlags(statusflag_t flags);

		Opcode* DecodeOpcode(uint8_t arg_op);

		void Reset();
		void RegisterOpcodes();

		/**
		* Decodes the address of a specified opcode.
		* @return Actual memory address to use.
		**/
		uint16_t DecodeOperandAddress(const uint16_t arg_addr, AddressingMode arg_addrmode);

		// ***** OPCODES *****
		// file:///C:/Users/DeepThought/Desktop/NES%20DOCS/Opcodes/6502%20Opcodes.html#TOC

		void opcode_notimplemented();
		void opcode_jmp();

		// Processor status instructions
		void opcode_sei();
		void opcode_cli();
		void opcode_cld();
		void opcode_clc();
		void opcode_sec();

		// Register manipulation instructions
		void opcode_lda();
		void opcode_ldx();
		void opcode_ldy();
		void opcode_sta();
		void opcode_stx();
		void opcode_sty();
		void opcode_inx();

		// Register instructions
		void opcode_dey();

		// Branch
		void opcode_bne();
		void opcode_bpl();
		void opcode_bit();

		// Stack instructions
		void opcode_txs();
		void opcode_tsx();

		void opcode_ora();
		void opcode_asl();
		void opcode_cmp();


	public:
		CPU();
		void Initialise();
		void Tick();
	};
}

#endif
