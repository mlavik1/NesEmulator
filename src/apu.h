#ifndef NESEMU_APU_H
#define NESEMU_APU_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>

const int AMPLITUDE = 28000;
const int SAMPLE_RATE = 44100;

const int SAMPLE_WRITE_BUFFER = 768; // 512 + 256

namespace nesemu
{
	class APU
	{
	private:
		bool mInitialised = false;
		Uint32 mTimeLastSample;
		
		uint64_t mSampleCounter = 0;
		double mTime = 0.0f;

	public:
		APU();
		void Tick(int arg_cpucycles);

		static void audio_callback(void *user_data, Uint8 *raw_buffer, int bytes);
	};
}

#endif
