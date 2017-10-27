#include "ppu.h"

#include "memory.h"

namespace nesemu
{
	void PPU::Tick(int arg_cpucycles)
	{
		mCPUCycle += arg_cpucycles;
		mPPUCycle = mCPUCycle * 3;
		mScanline = mPPUCycle / PPUCyclesPerScanline;

		// Reset cycle counters if done with frame
		int cyclesOverdue = mPPUCycle - PPUCyclesPerFrame;
		if (cyclesOverdue >= 0)
		{
			mPPUCycle -= PPUCyclesPerFrame;
			mCPUCycle = mPPUCycle / 3;
			mScanline -= ScanlinesPerFrame;
			mVBlank = false;
			mVBlankCompleted = false;
		}

		// VBlank
		if (!mVBlank && !mVBlankCompleted && mScanline >= SCANLINE_VBLANK)
		{
			StartVBlank();
		}
		else if (mVBlank && !mVBlankCompleted && mScanline >= SCANLINE_VBLANK_END)
		{
			mVBlank = false;
			mVBlankCompleted = true;
		}

	}

	void PPU::InterruptNMI()
	{

	}

	void PPU::StartVBlank()
	{
		uint8_t val = 1 << 7;
		GMemory->Write(MEMLOC_VBLANK, &val, sizeof(val));
		mVBlank = true;
	}
}
