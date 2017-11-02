#ifndef NESEMU_APU_H
#define NESEMU_APU_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>

const int AMPLITUDE = 28000;
const int SAMPLE_RATE = 44100;

const int SAMPLE_WRITE_BUFFER = 768; // 512 + 256

namespace nesemu
{
	enum SquareChannel
	{
		One,
		Two
	};

	class APU
	{
	private:
		bool mInitialised = false;
		Uint32 mTimeLastSample;
		
		uint64_t mSampleCounter = 0;
		double mTime = 0.0f;

		Sint16 mSampleBuffer[SAMPLE_RATE];
		size_t mReadPos = 0;
		size_t mWritePos = SAMPLE_WRITE_BUFFER;

		Sint16 mSampleWriteBuffer[SAMPLE_WRITE_BUFFER];
		size_t mSampleWriteBufferPos = 0;

		int TriangleDutyCycleSequence[32] =
		{ 8, 7, 6, 5, 4, 3, 2, 1, 0, -1, -2, -3, -4, -5, -6, -7
		 -7, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8 };

		int SquareDutyChannel[4][8] = 
		{
			{ -4, -4, -4, -4, -4, -4, -4,  4 },
			{ -4, -4, -4, -4, -4, -4,  4,  4 },
			{ -4, -4, -4, -4,  4,  4,  4,  4 },
			{  4,  4,  4,  4,  4,  4, -4, -4 }
		};

	public:
		APU();
		void Tick(int arg_cpucycles);
		void Initialise();
		void UpdateBuffer();

		double GetSquareChannelSampleValue(const SquareChannel arg_channel);
		double GetTriangleChannelSampleValue();

		static void audio_callback(void *user_data, Uint8 *raw_buffer, int bytes);
	};
}

#endif
