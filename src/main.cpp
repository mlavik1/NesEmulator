#include <iostream>

#include "nes.h"

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
