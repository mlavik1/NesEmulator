#ifndef NESEMU_NES_H
#define NESEMU_NES_H

#include "memory.h"
#include "cpu.h"
#include "rom.h"
#include <string>

namespace nesemu
{
	class NES
	{
	private:
		CPU* mCPU = nullptr;
		Memory* mMemory = nullptr;
		ROM* mROM = nullptr;
		std::string mCurrentROM;
		bool mIsRunning = false;

	public:
		void SetROM(const char* arg_file);
		void Start();
		void Update();
		bool IsRunning();
	};
}

#endif
