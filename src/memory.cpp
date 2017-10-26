#include "memory.h"

#include <memory>

namespace nesemu
{
	Memory* GMemory = nullptr;

	Memory::Memory()
	{
	}

	uint8_t Memory::ReadByte(const uint32_t& arg_address)
	{
		return mData[arg_address];
	}

	uint16_t Memory::ReadWord(const uint32_t& arg_address)
	{
		return *(uint16_t*)&mData[arg_address];
	}

	void Memory::Read(const uint32_t& arg_address, const size_t& arg_bytes, void* out_dest)
	{
		memcpy(out_dest, &mData[arg_address], arg_bytes);
	}

	uint16_t Memory::ReadMemoryAddress(const uint16_t& arg_location)
	{
		uint16_t retVal;
		uint16_t l = ReadByte(arg_location);
		uint16_t r = ReadByte(arg_location + 1);
		retVal = l | (r << 8); // swap order of the two bytes
		return retVal;
	}

	void Memory::Write(const uint32_t& arg_address, void* arg_data, const size_t& arg_bytes)
	{
		memcpy(&mData[arg_address], arg_data, arg_bytes);
	}
}
