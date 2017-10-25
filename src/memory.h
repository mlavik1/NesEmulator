#ifndef NESEMU_MEMORY_H
#define NESEMU_MEMORY_H

#include <stdint.h>

#define NESMEM_TOTAL_MEMORY		0x10000
#define NESMEM_RAM_START		0x0000
#define NESMEM_PPU_START		0x2000
#define NESMEM_APU_START		0x4018
#define NESMEM_ROM_START		0x4020
#define NESMEM_PRG_START		0x8000

namespace nesemu
{
	class Memory
	{
	private:
		uint8_t mData[NESMEM_TOTAL_MEMORY];

	public:
		Memory();

		uint8_t ReadByte(const uint32_t& arg_address);
		uint16_t ReadWord(const uint32_t& arg_address);
		void Read(const uint32_t& arg_address, const size_t& arg_bytes, void* out_dest);
		uint16_t ReadMemoryAddress(const uint16_t& arg_location);

		void Write(const uint32_t& arg_address, void* arg_data, const size_t& arg_bytes);
	};

	extern Memory* GMemory;
}

#endif
