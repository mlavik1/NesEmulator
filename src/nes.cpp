#include "nes.h"

#include "sdl2/SDL.h"

namespace nesemu
{
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
		if (mPPU->mNMIQueued)
		{
			mCPU->QueueNMI(); // TODO: CPU::QueueInterrupt(type)
			mPPU->mNMIQueued = false; // TODO: accessor function or callback
		}

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
