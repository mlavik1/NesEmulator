#ifndef NESEMU_PPU_H
#define NESEMU_PPU_H

#include <functional>

// 262 scanlines per frame
// 1 scanlane = 341 PPU clock cycles
// 1 CPU cycle = 3 PPU cycles

#define SCANLINE_VBLANK			241
#define SCANLINE_VBLANK_END		260

namespace nesemu
{
	class PPU
	{
	private:
		void InterruptNMI();

		int mCPUCycle = 0;
		int mPPUCycle = 0;
		int mScanline = 0;

		std::function<void()> mVBlankCallback;

		const int ScanlinesPerFrame = 262;
		const int PPUCyclesPerScanline = 341;
		const int PPUCyclesPerFrame = ScanlinesPerFrame * PPUCyclesPerScanline;

		bool mVBlank = false;
		bool mVBlankCompleted = false;

		void StartVBlank();

	public:
		const float TicksPerCPUCycle = 2.3f;

		void Tick(int arg_cpucycles);

		void SetVBlankCallback(std::function<void()> arg_callback);
	};
}

#endif
