#ifndef NESEMU_NES_H
#define NESEMU_NES_H

#include "memory.h"
#include "cpu.h"
#include "rom.h"
#include <string>
#include "apu.h"
#include "ppu.h"

namespace nesemu
{
	class NES
	{
	private:
		CPU* mCPU = nullptr;
		PPU* mPPU = nullptr;
		APU* mAPU = nullptr;
		Memory* mMemory = nullptr;
		ROM* mROM = nullptr;
		std::string mCurrentROM;
		bool mIsRunning = false;

		int mTimeLastDelay = 0;
		int mCycleCounter = 0;

	public:
		NES();
		void SetROM(const char* arg_file);
		void Start();
		void Update();
		bool IsRunning();

	};
}

#endif
