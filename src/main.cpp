#include <iostream>

#include "nes.h"

#undef main // TODO

using namespace nesemu;

int main()
{
	std::cout << "Starting from main" << std::endl;

	NES* nes = new NES();
	nes->SetROM("main.nes");
	nes->Start();

	while (nes->IsRunning())
	{
		nes->Update();
	}

	return 0;
}
