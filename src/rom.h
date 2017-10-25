#ifndef NESEMU_ROM_H
#define NESEMU_ROM_H

#include <stdint.h>
#include "memory.h"

#define ROM_HEADER_SIZE		0x10
#define ROM_PRG_SIZE_MAX	0x8000

namespace nesemu
{
	// https://wiki.nesdev.com/w/index.php/INES
	class ROM
	{
	private:
		uint8_t mHeaderBuffer[ROM_HEADER_SIZE];
		uint8_t mPRGBuffer[ROM_PRG_SIZE_MAX];
		uint8_t mCHRBuffer[ROM_PRG_SIZE_MAX];

		int mPrgCount;
		int mChrCount;
		int mPrgSize;

	public:
		bool Load(const char* arg_file);
		void CopyToMemory();
	};
}

#endif
