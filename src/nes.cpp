#include "nes.h"

#include "sdl2/SDL.h"
#include <iostream>

namespace nesemu
{
	NES::NES()
	{
		if (SDL_Init(SDL_INIT_AUDIO) != 0)
		{
			std::cout << "FAILED TO INITIALISE AUDIO" << std::endl;
		}
	}

	void NES::SetROM(const char* arg_file)
	{
		mCurrentROM = arg_file;
	}

	void NES::Start()
	{
		GMemory = new Memory();
		mMemory = GMemory;
		mCPU = new CPU();
		mPPU = new PPU();
		mAPU = new APU();
		mROM = new ROM();

		std::function<void()> vBlakCallback = [&] {mCPU->Interrupt(InterruptType::NMI); };
		mPPU->SetVBlankCallback(vBlakCallback);

		bool romLoaded = false;
		if (mCurrentROM != "")
		{
			romLoaded = mROM->Load(mCurrentROM.c_str());
			mROM->CopyToMemory(); // TODO: MMU
		
			mCPU->Initialise();
		}

		mIsRunning = romLoaded;
	}

	void NES::Update()
	{
		mCPU->Tick();

		int currentFrameCycles = mCPU->GetCurrentFrameCycles();
		if (currentFrameCycles > 0)
		{
			mPPU->Tick(currentFrameCycles);
			mAPU->Tick(currentFrameCycles);
		}

		int currTime = SDL_GetTicks();
		int elapsedTime = currTime - mTimeLastDelay;
		mCycleCounter += currentFrameCycles;

		int expectedTime = (mCycleCounter * 1000) / mCPU->CPUClockRate;
		int delay = expectedTime - elapsedTime;
		if (delay > 0 && elapsedTime > 0)
		{
			SDL_Delay(delay);
			mTimeLastDelay = currTime; // TODO
			mCycleCounter = 0; // TODO
		}
	}

	bool NES::IsRunning()
	{
		return mIsRunning;
	}
}
