#include "apu.h"
#include <math.h>
#include <memory>
#include "memory.h"
#include <sdl2/SDL_timer.h>

namespace nesemu
{
	Sint16 mSampleBuffer[SAMPLE_RATE];
	size_t mReadPos = 0;
	size_t mWritePos = SAMPLE_WRITE_BUFFER;

	Sint16 mSampleWriteBuffer[SAMPLE_WRITE_BUFFER];
	size_t mSampleWriteBufferPos = 0;

	APU::APU()
	{
		for (int i = 0; i < SAMPLE_RATE; i++)
		{
			//double time = (double)i / (double)SAMPLE_RATE;
			//double f = sin(2.0f * M_PI * 441.0f * time);
			mSampleBuffer[i] = 0;
			//mSampleBuffer[i] = (Sint16)(AMPLITUDE * f);

		}
	}

	void APU::Tick(int arg_cpucycles)
	{
		Uint32 currTime = SDL_GetTicks();

		Uint32 timeDiff = currTime - mTimeLastSample;
		
		if (timeDiff > 0)
		{
			const int samplesPerMS = (SAMPLE_RATE / 1000) + 1; // TEMP
			const int samplesThisFrame = samplesPerMS * timeDiff;
			mTimeLastSample = currTime;
			for (int i = 0; i < samplesThisFrame; i++)
			{
				uint8_t b1 = GMemory->ReadByte(0x4002);
				uint8_t b2 = GMemory->ReadByte(0x4003);

				uint16_t freqVal = b1 | (b2 << 5);

				const double freq = (double)freqVal;

				const double f = sin(2.0f * M_PI * (441.0f + (mTime*4.0f)) * mTime);
				const double fRel = (f + 1.0f) / 2.0f;
				const int iSample = (fRel * 8);
				
				mSampleWriteBuffer[mSampleWriteBufferPos++] = (Sint16)(AMPLITUDE * (iSample >= 6 ? 1.0f : -1.0f));
				
				// Copy to sample buffer
				if (mSampleWriteBufferPos >= SAMPLE_WRITE_BUFFER)
				{
					const size_t totalLen = SAMPLE_WRITE_BUFFER;
					const size_t lenToEnd = SAMPLE_RATE - mWritePos;
					const size_t len = lenToEnd > totalLen ? totalLen : lenToEnd;
					const size_t lenBytes = len * 2;
					memcpy(mSampleBuffer + mWritePos, mSampleWriteBuffer, lenBytes);
					mWritePos += totalLen;
					if (mWritePos >= SAMPLE_RATE)
					{
						mWritePos -= SAMPLE_RATE;
						if (mWritePos > 0)
						{
							memcpy(mSampleBuffer, mSampleWriteBuffer + len, (totalLen - len) * 2);
						}
					}

					mSampleWriteBufferPos = 0;
				}

				mSampleCounter += 1;
				mTime = (double)mSampleCounter / (double)SAMPLE_RATE;
			}
		}

		if (!mInitialised)
		{
			mInitialised = true;

			mTimeLastSample = SDL_GetTicks();

			int sample_nr = 0;

			SDL_AudioSpec audioSpec;
			audioSpec.freq = SAMPLE_RATE; // samples per second
			audioSpec.format = AUDIO_S16SYS; // signed short (16 bit)
			audioSpec.channels = 1;
			audioSpec.samples = 512; // buffer-size
			audioSpec.callback = audio_callback;
			audioSpec.userdata = &sample_nr;

			SDL_AudioSpec desiredSpec;
			if (SDL_OpenAudio(&audioSpec, &desiredSpec) != 0)
				SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Failed to open audio: %s", SDL_GetError());
			if (audioSpec.format != desiredSpec.format)
				SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Failed to get the desired AudioSpec");

			SDL_PauseAudio(0);
		}
	}

	void APU::audio_callback(void *user_data, Uint8 *raw_buffer, int bytes)
	{
		Sint16 *buffer = (Sint16*)raw_buffer;
		int length = bytes / 2; // 2 bytes per sample for AUDIO_S16SYS
		
		const size_t totalLen = bytes / 2;
		const size_t lenToEnd = SAMPLE_RATE - mReadPos;
		const size_t len = lenToEnd > totalLen ? totalLen : lenToEnd;
		const size_t lenBytes = len * 2;

		memcpy(buffer, mSampleBuffer + mReadPos,  lenBytes);
		mReadPos += totalLen;
		if (mReadPos >= SAMPLE_RATE)
		{
			mReadPos -= SAMPLE_RATE;
			if (mReadPos > 0)
			{
				memcpy(buffer + len, mSampleBuffer, mReadPos * 2);
			}
		}
	}

}
