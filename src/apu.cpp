#include "apu.h"
#include <math.h>
#include <memory>
#include "memory.h"
#include <sdl2/SDL_timer.h>

namespace nesemu
{
	APU::APU()
	{
		std::fill_n(mSampleBuffer, SAMPLE_RATE, 0);
	}

	void APU::Tick(int arg_cpucycles)
	{
		UpdateBuffer();

		if (!mInitialised)
		{
			Initialise();
		}
	}

	void APU::UpdateBuffer()
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
				mSampleWriteBuffer[mSampleWriteBufferPos] = 0;

				const double square1 = GetSquareChannelSampleValue(SquareChannel::One);
				const double square2 = GetSquareChannelSampleValue(SquareChannel::Two);
				const double triangle = GetTriangleChannelSampleValue();
				mSampleWriteBuffer[mSampleWriteBufferPos] += (Sint16)(AMPLITUDE * square1);
				mSampleWriteBuffer[mSampleWriteBufferPos] += (Sint16)(AMPLITUDE * square2);
				mSampleWriteBuffer[mSampleWriteBufferPos] += (Sint16)(AMPLITUDE * triangle);

				mSampleWriteBufferPos++;

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
	}

	void APU::Initialise()
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
		audioSpec.userdata = this;

		SDL_AudioSpec desiredSpec;
		if (SDL_OpenAudio(&audioSpec, &desiredSpec) != 0)
			SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Failed to open audio: %s", SDL_GetError());
		if (audioSpec.format != desiredSpec.format)
			SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Failed to get the desired AudioSpec");

		SDL_PauseAudio(0);
	}

	double APU::GetSquareChannelSampleValue(const SquareChannel arg_channel)
	{
		// read frequency from memory
		const uint8_t b1 = GMemory->ReadByte(arg_channel == SquareChannel::One ? 0x4000 : 0x4004);
		const uint8_t b2 = GMemory->ReadByte(arg_channel == SquareChannel::One ? 0x4001 : 0x4005);
		const uint8_t b3 = GMemory->ReadByte(arg_channel == SquareChannel::One ? 0x4002 : 0x4006);
		const uint8_t b4 = GMemory->ReadByte(arg_channel == SquareChannel::One ? 0x4003 : 0x4007);

		uint16_t freqVal = b3 | (((uint16_t)b4 << 8) & 0b0000011100000000);

		if (freqVal > 0)
		{
			const uint8_t duty = b1 >> 6; // first two bits of $4000 / $4004
			const double fPulse = (double)freqVal;
			const double freq = ((1789773.0f) / (16.0f * (fPulse + 1.0f))) - 1.0f;

			const double f = sin(2.0f * M_PI * (freq)* mTime);
			const double fRel = (f + 1.0f) / 2.0f;
			const int iSample = (fRel * 8);

			return SquareDutyChannel[duty][iSample];
		}
		return 0.0f;
	}

	double APU::GetTriangleChannelSampleValue()
	{
		// read sound data from memory
		const uint8_t b1 = GMemory->ReadByte(0x4008);
		const uint8_t b2 = GMemory->ReadByte(0x400A);
		const uint8_t b3 = GMemory->ReadByte(0x400B);

		uint16_t freqVal = b2 | (((uint16_t)b3 << 8) & 0b0000011100000000);

		if (freqVal > 0)
		{
			const double fPulse = (double)freqVal;
			const double freq = ((1789773.0f) / (16.0f * (fPulse + 1.0f))) - 1.0f;

			const double f = sin(2.0f * M_PI * (freq)* mTime);
			const double fRel = (f + 1.0f) / 2.0f;
			const int iSample = (fRel * 32);

			return TriangleDutyCycleSequence[iSample];
		}
	}

	void APU::audio_callback(void *user_data, Uint8 *raw_buffer, int bytes)
	{
		APU* apu = (APU*)user_data;
		Sint16 *buffer = (Sint16*)raw_buffer;
		int length = bytes / 2; // 2 bytes per sample for AUDIO_S16SYS
		
		const size_t totalLen = bytes / 2;
		const size_t lenToEnd = SAMPLE_RATE - apu->mReadPos;
		const size_t len = lenToEnd > totalLen ? totalLen : lenToEnd;
		const size_t lenBytes = len * 2;

		memcpy(buffer, apu->mSampleBuffer + apu->mReadPos,  lenBytes);
		apu->mReadPos += totalLen;
		if (apu->mReadPos >= SAMPLE_RATE)
		{
			apu->mReadPos -= SAMPLE_RATE;
			if (apu->mReadPos > 0)
			{
				memcpy(buffer + len, apu->mSampleBuffer, apu->mReadPos * 2);
			}
		}
	}

}
