#include "nes.h"

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
		mCPU->Tick();
	}

	bool NES::IsRunning()
	{
		return mIsRunning;
	}
}
