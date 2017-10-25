#include "rom.h"
#include <fstream>
#include <iostream>

namespace nesemu
{
	bool ROM::Load(const char* arg_file)
	{
		std::cout << "Loading cartridge " << arg_file << "..." << std::endl;
		std::basic_ifstream<uint8_t> file;
		file.open(arg_file, std::ios::in | std::ios::binary);
		if (!file.is_open())
		{
			std::cout << "ERROR: Failed to read ROM file" << std::endl;
			return false;
		}

		// Read Header
		file.read(mHeaderBuffer, ROM_HEADER_SIZE);

		// Validate ROM: first 3 bytes should be "NES"
		if (mHeaderBuffer[0] != 'N' || mHeaderBuffer[1] != 'E' || mHeaderBuffer[2] != 'S')
		{
			std::cout << "ERROR: Invalid ROM file" << std::endl;
			return false;
		}
		std::cout << "NES" << std::endl;

		mPrgCount = mHeaderBuffer[4];
		mChrCount = mHeaderBuffer[5];;
		mPrgSize = mPrgCount * 0x4000;

		// Read PRG
		file.read(mPRGBuffer, mPrgSize);

		// Read CHR
		file.read(mCHRBuffer, 0x2000 * mChrCount);

		std::cout << mHeaderBuffer[0];
		std::cout << mHeaderBuffer[1];
		std::cout << mHeaderBuffer[2];

		std::cout << "PrgCount: " << mPrgCount << std::endl;
		std::cout << "ChrCount: " << mChrCount << std::endl;

		std::cout << "Done reading ROM" << std::endl;

		return true;
	}

	void ROM::CopyToMemory()
	{
		if (mPrgCount == 1)
		{
			GMemory->Write(0x8000, &mPRGBuffer, 0x4000);
			GMemory->Write(0xC000, &mPRGBuffer, 0x4000);
		}
		else
		{
			GMemory->Write(0x8000, &mPRGBuffer, 0x8000);
		}

		if (mChrCount > 0)
		{
			GMemory->Write(0x0000, &mPRGBuffer, 0x2000); // ???
		}
	}
}
