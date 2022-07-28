/*
	Segaapi audio library emulator
	Parts stolen from Sega, teknogods and jayfoxrox
	Modified by doozer in 2022 to work with Outrun 2 SP under modern Linux
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <unistd.h>
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include <AL/alut.h>

//#include "eax4.h"
#include "segadef.h"
#include "segaerr.h"
#include "segaeax.h"

#include "segaapi.h"

#define TSF_IMPLEMENTATION
#include "tsf.h"

// help and info
// https://www.openal.org/documentation/
// https://github.com/teknogods/OpenSegaAPI/blob/master/Opensegaapi/src/opensegaapi.cpp
// https://web.archive.org/web/20070218003259/http://www.devmaster.net/articles.php?catID=6

//#define DEBUG_SAMPLE
#define DEBUG_OUTPUT
//#define DUMP_WAV
//#define DUMP_BUFFER

#define CHECK()                                                \
	{                                                          \
		printf("%s %d\n", __func__, __LINE__);                 \
		unsigned int err;                                      \
		if ((err = alGetError()) != AL_NO_ERROR)               \
		{                                                      \
			printf("%s %d\n", __func__, __LINE__);             \
			dbgPrint(":%i AL Error: 0x%08X\n", __LINE__, err); \
			exit(2);                                           \
		}                                                      \
	}

SEGASTATUS g_LastStatus = SEGA_SUCCESS;

// outrun2 will complain if these aren't present
const GUID EAX_NULL_GUID;
// DEFINE_GUID(EAX_NULL_GUID, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
// DEFINE_GUID(EAXPROPERTYID_EAX40_FXSlot2, 0x883b431d, 0xf6f0, 0x3746, 0x91, 0x9f, 0x60, 0xe7, 0xe0, 0x6b, 0x5e, 0xdd);

const GUID EAX_FREQUENCYSHIFTER_EFFECT;
const GUID EAX_ECHO_EFFECT;
const GUID EAX_REVERB_EFFECT;
const GUID EAX_EQUALIZER_EFFECT;
const GUID EAX_DISTORTION_EFFECT;
const GUID EAX_AGCCOMPRESSOR_EFFECT;
const GUID EAX_PITCHSHIFTER_EFFECT;
const GUID EAX_FLANGER_EFFECT;
const GUID EAX_VOCALMORPHER_EFFECT;
const GUID EAX_AUTOWAH_EFFECT;
const GUID EAX_RINGMODULATOR_EFFECT;
const GUID EAX_CHORUS_EFFECT;

const GUID EAXPROPERTYID_EAX40_FXSlot0;
const GUID EAXPROPERTYID_EAX40_FXSlot1;
const GUID EAXPROPERTYID_EAX40_FXSlot2;
const GUID EAXPROPERTYID_EAX40_FXSlot3;

typedef struct
{
	void *userData;
	HAWOSEGABUFFERCALLBACK callback;
	bool synthesizer;
	bool loop;
	unsigned int channels;
	unsigned int startLoop;
	unsigned int endLoop;
	unsigned int endOffset;
	unsigned int sampleRate;
	unsigned int sampleFormat;
	uint8_t *data;
	size_t size;
	bool playing;
	bool paused;
	bool playWithSetup;

	ALuint alBuffer;
	ALuint alSource;

	tsf *synth;
	struct tsf_region *region;
} segaapiContext_t;

LPALBUFFERSAMPLESSOFT alBufferSamplesSOFT = NULL;
LPALBUFFERSUBSAMPLESSOFT alBufferSubSamplesSOFT = NULL;
LPALGETBUFFERSAMPLESSOFT alGetBufferSamplesSOFT = NULL;

#ifdef DEBUG_OUTPUT
void dbgPrint(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	printf("\n");
}
#else
void dbgPrint(const char *format, ...)
{
	return;
}
#endif

#ifdef DUMP_BUFFER
static void dumpBuffer(const char *path, void *data, size_t size)
{
	FILE *soundFile = NULL;

	soundFile = fopen(path, "wb");
	fwrite(data, size, 1, soundFile);
	fclose(soundFile);
}
#endif

#ifdef DUMP_WAV
static void dumpWaveBuffer(const char *path, unsigned int channels, unsigned int sampleRate, unsigned int sampleBits, void *data, size_t size)
{

	struct RIFF_Header
	{
		char chunkID[4];
		long chunkSize; // size not including chunkSize or chunkID
		char format[4];
	};

	struct WAVE_Format
	{
		char subChunkID[4];
		long subChunkSize;
		short audioFormat;
		short numChannels;
		long sampleRate;
		long byteRate;
		short blockAlign;
		short bitsPerSample;
	};

	struct WAVE_Data
	{
		char subChunkID[4]; // should contain the word data
		long subChunk2Size; // Stores the size of the data block
	};

	// Local Declarations
	FILE *soundFile = NULL;
	struct WAVE_Format wave_format;
	struct RIFF_Header riff_header;
	struct WAVE_Data wave_data;

	soundFile = fopen(path, "wb");

	// check for RIFF and WAVE tag in memeory
	riff_header.chunkID[0] = 'R';
	riff_header.chunkID[1] = 'I';
	riff_header.chunkID[2] = 'F';
	riff_header.chunkID[3] = 'F';
	riff_header.format[0] = 'W';
	riff_header.format[1] = 'A';
	riff_header.format[2] = 'V';
	riff_header.format[3] = 'E';

	// Read in the first chunk into the struct
	fwrite(&riff_header, sizeof(struct RIFF_Header), 1, soundFile);

	// check for fmt tag in memory
	wave_format.subChunkID[0] = 'f';
	wave_format.subChunkID[1] = 'm';
	wave_format.subChunkID[2] = 't';
	wave_format.subChunkID[3] = ' ';

	wave_format.audioFormat = 1;
	wave_format.sampleRate = sampleRate;
	wave_format.numChannels = channels;
	wave_format.bitsPerSample = sampleBits;
	wave_format.byteRate = (sampleRate * sampleBits * channels) / 8;
	wave_format.blockAlign = (sampleBits * channels) / 8;
	wave_format.subChunkSize = 16;

	// Read in the 2nd chunk for the wave info
	fwrite(&wave_format, sizeof(struct WAVE_Format), 1, soundFile);

	// Read in the the last byte of data before the sound file

	// check for data tag in memory
	wave_data.subChunkID[0] = 'd';
	wave_data.subChunkID[1] = 'a';
	wave_data.subChunkID[2] = 't';
	wave_data.subChunkID[3] = 'a';

	wave_data.subChunk2Size = size;

	fwrite(&wave_data, sizeof(struct WAVE_Data), 1, soundFile);

	// Read in the sound data into the soundData variable
	fwrite(data, wave_data.subChunk2Size, 1, soundFile);

	// clean up and return true if successful
	fclose(soundFile);

	return;
}
#endif


ALsizei FramesToBytes(ALsizei size, ALenum channels, ALenum type)
{
    switch(channels)
    {
    case AL_MONO_SOFT:    size *= 1; break;
    case AL_STEREO_SOFT:  size *= 2; break;
    case AL_REAR_SOFT:    size *= 2; break;
    case AL_QUAD_SOFT:    size *= 4; break;
    case AL_5POINT1_SOFT: size *= 6; break;
    case AL_6POINT1_SOFT: size *= 7; break;
    case AL_7POINT1_SOFT: size *= 8; break;
    }

    switch(type)
    {
    case AL_BYTE_SOFT:           size *= sizeof(ALbyte); break;
    case AL_UNSIGNED_BYTE_SOFT:  size *= sizeof(ALubyte); break;
    case AL_SHORT_SOFT:          size *= sizeof(ALshort); break;
    case AL_UNSIGNED_SHORT_SOFT: size *= sizeof(ALushort); break;
    case AL_INT_SOFT:            size *= sizeof(ALint); break;
    case AL_UNSIGNED_INT_SOFT:   size *= sizeof(ALuint); break;
    case AL_FLOAT_SOFT:          size *= sizeof(ALfloat); break;
    case AL_DOUBLE_SOFT:         size *= sizeof(ALdouble); break;
    }

    return size;
}


static unsigned int bufferSampleSize(segaapiContext_t *context)
{ // printf("%s %d\n", __func__, __LINE__);
	return context->channels * ((context->sampleFormat == HASF_SIGNED_16PCM) ? 2 : 1);
}

static void updateBufferLoop(segaapiContext_t *context)
{ // printf("%s %d\n", __func__, __LINE__);
	if (context == NULL)
		return;
	unsigned int sampleSize = bufferSampleSize(context);
	alSourcei(context->alSource, AL_BUFFER, AL_NONE);
	// CHECK();
	/*
	FIXME: Re-enable, only crashed before - so fix this too..
	  ALint loopPoints[] = { buffer->startLoop / sampleSize, buffer->endLoop / sampleSize };
	  alBufferiv(buffer->alBuffer,AL_LOOP_POINTS_SOFT,loopPoints);
	  CHECK();
	*/
	return;
}

void AL_APIENTRY wrap_BufferSamples(ALuint buffer, ALuint samplerate,
									ALenum internalformat, ALsizei samples,
									ALenum channels, ALenum type,
									const ALvoid *data)
{
	alBufferData(buffer, internalformat, data,
				 FramesToBytes(samples, channels, type),
				 samplerate);
}


static void updateBufferData(segaapiContext_t *context, unsigned int offset, size_t length)
{

	ALenum alFormat = -1;
	ALenum alChannels = -1;
	ALenum alType;

	switch (context->sampleFormat)
	{
	case HASF_UNSIGNED_8PCM: /* Unsigned (offset 128) 8-bit PCM */
		alType = AL_BYTE_SOFT;
		switch (context->channels)
		{
		case 1:
			alFormat = AL_MONO8_SOFT;
			alChannels = AL_MONO_SOFT;
			break;
		case 2:
			alFormat = AL_STEREO8_SOFT;
			alChannels = AL_STEREO_SOFT;
			break;
		default:
			break;
		}
		break;
	case HASF_SIGNED_16PCM: /* Signed 16-bit PCM */
		alType = AL_SHORT_SOFT;
		switch (context->channels)
		{
		case 1:
			alFormat = AL_MONO16_SOFT;
			alChannels = AL_MONO_SOFT;
			break;
		case 2:
			alFormat = AL_STEREO16_SOFT;
			alChannels = AL_STEREO_SOFT;
			break;
		default:
			break;
		}
	default:
		break;
	}
	if (alFormat == -1)
	{
		dbgPrint("Unknown format! 0x%X with %u channels!\n", context->sampleFormat, context->channels);
	}

	ALint position;
	alGetSourcei(context->alSource, AL_SAMPLE_OFFSET, &position); // TODO: Patch if looping is active
	// CHECK();

	ALint unsafe[2];
	alGetSourceiv(context->alSource, AL_BYTE_RW_OFFSETS_SOFT, unsafe); // AL_BYTE_OFFSET
	// CHECK();
	if (offset != -1)
	{
		//printf("CANNOT DO IT\n");
		//exit(1);
		wrap_BufferSamples(context->alBuffer, context->sampleRate, alFormat, context->size / bufferSampleSize(context), alChannels, alType, &(context->data[offset]));

		//alBufferSubSamplesSOFT(context->alBuffer, offset / bufferSampleSize(context), length / bufferSampleSize(context), alChannels, alType, &context->data[offset]);
		// CHECK();
		dbgPrint("Soft update in buffer %X at %u (%u bytes) - buffer playing at %u, unsafe region is %u to %u\n", (uintptr_t)context, offset, length, position, unsafe[0], unsafe[1]);
	}
	else
	{

	
		alSourcei(context->alSource, AL_BUFFER, AL_NONE);
		// CHECK();
		wrap_BufferSamples(context->alBuffer, context->sampleRate, alFormat, context->size / bufferSampleSize(context), alChannels, alType, context->data);
		// CHECK();
		alSourcei(context->alSource, AL_BUFFER, context->alBuffer);
		// CHECK();
		updateBufferLoop(context);
		dbgPrint("Hard update in buffer %X (%u bytes) - buffer playing at %u, unsafe region is %u to %u\n", (uintptr_t)context, context->size, position, unsafe[0], unsafe[1]);
	}

#ifdef DUMP_WAV
	// This dumps the buffer
	if (context->data != NULL)
	{
		uint8_t *nonZero = context->data;
		while (*nonZero++ == 0x00)
			;
		if ((nonZero - (uint8_t *)context->data) < context->size)
		{
			printf("%s %d\n", __func__, __LINE__);
			char buf[1000];
			sprintf(buf, "SAMPLE-%X-%i-%04X-%u.wav", (uintptr_t)context, context->channels, context->sampleFormat, context->sampleRate);
			dbgPrint("Writing: %s (%i)", buf, context->size);
			void *tmp = malloc(context->size);
			if (tmp != NULL)
			{
				alGetBufferSamplesSOFT(context->alBuffer, 0, context->size / bufferSampleSize(context), alChannels, alType, tmp);
				// CHECK();
				dumpWaveBuffer(buf, context->channels, context->sampleRate, (context->sampleFormat == HASF_SIGNED_16PCM) ? 16 : 8, tmp, context->size);
				free(tmp);
			}
		}
	}
#endif
	return;
}

static void resetBuffer(segaapiContext_t *context)
{ // printf("%s %d\n", __func__, __LINE__);
	// *   - Send Routing
	// *      - for 1 channel buffer, channel is routed to Front-Left and Front-Right.
	// *      - for 2 channel buffer, channel 0 is routed Front-Left, channel 1 is routed Front-Right
	// *   - Send Levels are set to 0 (infinite attenuation)
	// *   - Channel Volume is set to 0xFFFFFFFF (no attenuation)
	// *   - No notification.
	// *   - StartLoopOffset is set to 0.
	context->startLoop = 0;
	// *   - EndLoopOffset and EndOffset are set to pConfig->mapdata.dwSize.
	context->endOffset = context->size;
	context->endLoop = context->size;
	// *   - No loop.
	context->loop = false;
	context->paused = false;
	context->playWithSetup = false;

	tsf *res = (tsf *)TSF_MALLOC(sizeof(tsf));
	TSF_MEMSET(res, 0, sizeof(tsf));
	res->presetNum = 0;
	res->outSampleRate = context->sampleRate;

	context->synth = res;

	struct tsf_region *region = malloc(sizeof(struct tsf_region));
	memset(region, 0, sizeof(struct tsf_region));

	tsf_region_clear(region, 0);

	region->ampenv.delay = 0;
	region->ampenv.hold = 300.0f;
	region->ampenv.attack = 0;
	region->ampenv.decay = 0;
	region->ampenv.release = 0;
	region->ampenv.sustain = 0;

	context->region = region;

	// *   - Buffer is in the stop state.
	// *   - Play position is set to 0.
	updateBufferData(context, -1, -1);
}

SEGASTATUS SEGAAPI_Play(CTHANDLE hHandle)
{
	dbgPrint("SEGAAPI_Play() 0x%x", hHandle);
	segaapiContext_t *context = hHandle;
	if (context == NULL)
		return SEGAERR_BAD_PARAM;
#ifdef DUMP_BUFFER
	if (context->data != NULL)
	{
		if (context->synthesizer)
		{
			if (context->data)
			{
				char filename[1000];
				sprintf(filename, "SYNTH-%X-%i-%04X-%u.bin", (uintptr_t)context, context->channels, context->sampleFormat, context->sampleRate);
				dbgPrint("Writing: %s (%i)", filename, context->size);
				dumpBuffer(filename, context->data, context->size);
			}
		}
	}
#endif

	alSourcei(context->alSource, AL_LOOPING, context->loop ? AL_TRUE : AL_FALSE);
	alSourcei(context->alSource, AL_BUFFER, context->alBuffer);
	alSourcePlay(context->alSource);
	return SEGA_SUCCESS;
}

SEGASTATUS SEGAAPI_Pause(CTHANDLE hHandle)
{
	dbgPrint("SEGAAPI_Pause() 0x%x", hHandle);
	segaapiContext_t *context = hHandle;
	if (context == NULL)
		return SEGAERR_BAD_PARAM;
	alSourcePause(context->alSource);
	return SEGA_SUCCESS;
}

SEGASTATUS SEGAAPI_Stop(CTHANDLE hHandle)
{
	dbgPrint("SEGAAPI_Stop() 0x%x", hHandle);
	segaapiContext_t *context = hHandle;
	if (context == NULL)
		return SEGAERR_BAD_PARAM;
	alSourceStop(context->alSource);
	return SEGA_SUCCESS;
}

SEGASTATUS SEGAAPI_PlayWithSetup(CTHANDLE hHandle)
{
	dbgPrint("SEGAAPI_PlayWithSetup() 0x%x", hHandle);
	segaapiContext_t *context = hHandle;
	if (context == NULL)
		return SEGAERR_BAD_PARAM;
	alSourcei(context->alSource, AL_LOOPING, context->loop ? AL_TRUE : AL_FALSE);
	alSourcei(context->alSource, AL_BUFFER, context->alBuffer);
	alSourcePlay(context->alSource);
	return SEGAERR_UNSUPPORTED;
}

HAWOSTATUS SEGAAPI_GetPlaybackStatus(CTHANDLE hHandle)
{
	ALint state;

	dbgPrint("SEGAAPI_GetPlaybackStatus() 0x%x", hHandle);
	segaapiContext_t *context = hHandle;
	if (context == NULL)
		return HAWOSTATUS_INVALID;

	alGetSourcei(context->alSource, AL_SOURCE_STATE, &state);
	switch (state)
	{
	case AL_PLAYING:
		return HAWOSTATUS_ACTIVE;
	case AL_PAUSED:
		return HAWOSTATUS_PAUSE;
	case AL_INITIAL:
	case AL_STOPPED:
		return HAWOSTATUS_STOP;
	default:
		return HAWOSTATUS_INVALID;
	}

	return HAWOSTATUS_INVALID;
}

SEGASTATUS SEGAAPI_SetFormat(CTHANDLE hHandle, HAWOSEFORMAT *pFormat)
{
	dbgPrint("SEGAAPI_SetFormat() 0x%x", hHandle);
	return SEGAERR_UNSUPPORTED;
}

SEGASTATUS SEGAAPI_GetFormat(CTHANDLE hHandle, HAWOSEFORMAT *pFormat)
{
	dbgPrint("SEGAAPI_GetFormat() 0x%x", hHandle);
	return SEGAERR_UNSUPPORTED;
}

SEGASTATUS SEGAAPI_SetSampleRate(CTHANDLE hHandle, CTDWORD dwSampleRate)
{
	dbgPrint("SEGAAPI_SetSampleRate() 0x%x 0x%x", hHandle, dwSampleRate);
	if (hHandle == NULL)
	{ // Not sure if this is correct here, but ABC currently tries to call this with a null pointer..
		return SEGAERR_BAD_HANDLE;
	}
	segaapiContext_t *context = hHandle;
	context->sampleRate = dwSampleRate;
	updateBufferData(context, -1, -1);
	return SEGA_SUCCESS;
}

CTDWORD SEGAAPI_GetSampleRate(CTHANDLE hHandle)
{
	dbgPrint("SEGAAPI_GetSampleRate() 0x%x", hHandle);
	return SEGAERR_UNSUPPORTED;
}

SEGASTATUS SEGAAPI_SetPriority(CTHANDLE hHandle, CTDWORD dwPriority)
{
	dbgPrint("SEGAAPI_SetPriority() 0x%x 0x%x", hHandle, dwPriority);
	return SEGAERR_UNSUPPORTED;
}

CTDWORD SEGAAPI_GetPriority(CTHANDLE hHandle)
{
	dbgPrint("SEGAAPI_GetPriority() 0x%x", hHandle);
	return SEGAERR_UNSUPPORTED;
}

SEGASTATUS SEGAAPI_SetUserData(CTHANDLE hHandle, CTHANDLE hUserData)
{
	dbgPrint("SEGAAPI_SetUserData() 0x%x 0x%x", hHandle, hUserData);
	if (hHandle == NULL)
	{ // Not sure if this is correct here, but ABC currently tries to call this with a null pointer..
		dbgPrint("SEGAAPI_SetUserData() SEGAERR_BAD_HANDLE");
		return SEGAERR_BAD_HANDLE;
	}
	segaapiContext_t *context = hHandle;
	context->userData = hUserData;
	dbgPrint("SEGAAPI_SetUserData() complete");
	return SEGA_SUCCESS;
}

CTHANDLE SEGAAPI_GetUserData(CTHANDLE hHandle)
{
	dbgPrint("SEGAAPI_GetPriority() 0x%x", hHandle);
	if (hHandle == NULL)
		return NULL;
	segaapiContext_t *context = hHandle;
	return context->userData;
}

SEGASTATUS SEGAAPI_SetSendRouting(CTHANDLE hHandle, CTDWORD dwChannel, CTDWORD dwSend, HAROUTING dwDest)
{
	dbgPrint("SEGAAPI_SetSendRouting() 0x%x 0x%x 0x%x 0x%x", hHandle, dwChannel, dwSend, dwDest);
	return SEGA_SUCCESS;
}

HAROUTING SEGAAPI_GetSendRouting(CTHANDLE hHandle, CTDWORD dwChannel, CTDWORD dwSend)
{
	dbgPrint("SEGAAPI_GetSendRouting() 0x%x 0x%x 0x%x", hHandle, dwChannel, dwSend);
	return HA_UNUSED_PORT;
}

SEGASTATUS SEGAAPI_SetSendLevel(CTHANDLE hHandle, CTDWORD dwChannel, CTDWORD dwSend, CTDWORD dwLevel)
{
	dbgPrint("SEGAAPI_SetSendLevel() 0x%x 0x%x 0x%x 0x%x", hHandle, dwChannel, dwSend, dwLevel);
	return SEGA_SUCCESS;
}

CTDWORD SEGAAPI_GetSendLevel(CTHANDLE hHandle, CTDWORD dwChannel, CTDWORD dwSend)
{
	dbgPrint("SEGAAPI_GetSendLevel() 0x%x 0x%x 0x%x", hHandle, dwChannel, dwSend);
	return 0;
}

SEGASTATUS SEGAAPI_SetChannelVolume(CTHANDLE hHandle, CTDWORD dwChannel, CTDWORD dwVolume)
{
	dbgPrint("SEGAAPI_SetChannelVolume() 0x%x 0x%x 0x%x", hHandle, dwChannel, dwVolume);
	return SEGAERR_UNSUPPORTED;
}

CTDWORD SEGAAPI_GetChannelVolume(CTHANDLE hHandle, CTDWORD dwChannel)
{
	dbgPrint("SEGAAPI_GetChannelVolume() 0x%x 0x%x", hHandle, dwChannel);
	return 0;
}

SEGASTATUS SEGAAPI_SetPlaybackPosition(CTHANDLE hHandle, CTDWORD dwPlaybackPos)
{
	dbgPrint("SEGAAPI_SetPlaybackPosition() 0x%x 0x%x", hHandle, dwPlaybackPos);
	segaapiContext_t *context = hHandle;
	alSourcei(context->alSource, AL_BYTE_OFFSET, dwPlaybackPos);
	return SEGA_SUCCESS;
}

CTDWORD SEGAAPI_GetPlaybackPosition(CTHANDLE hHandle)
{
	ALint position;
	dbgPrint("SEGAAPI_GetPlaybackPosition() 0x%x", hHandle);
	segaapiContext_t *context = hHandle;
	alGetSourcei(context->alSource, AL_BYTE_OFFSET, &position);
	return position;
}

SEGASTATUS SEGAAPI_SetNotificationFrequency(CTHANDLE hHandle, CTDWORD dwFrameCount)
{
	dbgPrint("SEGAAPI_SetNotificationFrequency() 0x%x 0x%x", hHandle, dwFrameCount);
	return SEGAERR_UNSUPPORTED;
}

SEGASTATUS SEGAAPI_SetNotificationPoint(CTHANDLE hHandle, CTDWORD dwBufferOffset)
{
	dbgPrint("SEGAAPI_SetNotificationPoint() 0x%x 0x%x", hHandle, dwBufferOffset);
	return SEGAERR_UNSUPPORTED;
}

SEGASTATUS SEGAAPI_ClearNotificationPoint(CTHANDLE hHandle, CTDWORD dwBufferOffset)
{
	dbgPrint("SEGAAPI_ClearNotificationPoint() 0x%x 0x%x", hHandle, dwBufferOffset);
	return SEGAERR_UNSUPPORTED;
}

SEGASTATUS SEGAAPI_SetStartLoopOffset(CTHANDLE hHandle, CTDWORD dwOffset)
{
	dbgPrint("SEGAAPI_SetStartLoopOffset() 0x%x 0x%x", hHandle, dwOffset);
	if (hHandle == NULL)
	{ // Not sure if this is correct here, but ABC currently tries to call this with a null pointer..
		return SEGAERR_BAD_HANDLE;
	}
	segaapiContext_t *context = hHandle;
	context->startLoop = dwOffset;
	updateBufferLoop(context);
	return SEGA_SUCCESS;
}

CTDWORD SEGAAPI_GetStartLoopOffset(CTHANDLE hHandle)
{
	dbgPrint("SEGAAPI_GetStartLoopOffset() 0x%x", hHandle);
	return 0;
}

SEGASTATUS SEGAAPI_SetEndLoopOffset(CTHANDLE hHandle, CTDWORD dwOffset)
{
	dbgPrint("SEGAAPI_SetEndLoopOffset() 0x%x 0x%x", hHandle, dwOffset);
	if (hHandle == NULL)
	{ // Not sure if this is correct here, but ABC currently tries to call this with a null pointer..
		return SEGAERR_BAD_HANDLE;
	}
	segaapiContext_t *context = hHandle;
	context->endLoop = dwOffset;
	updateBufferLoop(context);
	return SEGA_SUCCESS;
}

CTDWORD SEGAAPI_GetEndLoopOffset(CTHANDLE hHandle)
{
	dbgPrint("SEGAAPI_GetEndLoopOffset() 0x%x", hHandle);
	return 0;
}

SEGASTATUS SEGAAPI_SetEndOffset(CTHANDLE hHandle, CTDWORD dwOffset)
{
	dbgPrint("SEGAAPI_SetEndOffset() 0x%x 0x%x", hHandle, dwOffset);
	return SEGAERR_UNSUPPORTED;
}

CTDWORD SEGAAPI_GetEndOffset(CTHANDLE hHandle)
{
	dbgPrint("SEGAAPI_GetEndOffset() 0x%x", hHandle);
	return 0;
}

SEGASTATUS SEGAAPI_SetLoopState(CTHANDLE hHandle, CTBOOL bDoContinuousLooping)
{
	dbgPrint("SEGAAPI_SetLoopState() 0x%x 0x%x", hHandle, bDoContinuousLooping);
	segaapiContext_t *context = hHandle;
	context->loop = bDoContinuousLooping;
	alSourcei(context->alSource, AL_LOOPING, context->loop ? AL_TRUE : AL_FALSE);
	return SEGA_SUCCESS;
}

CTBOOL SEGAAPI_GetLoopState(CTHANDLE hHandle)
{
	dbgPrint("SEGAAPI_GetLoopState() 0x%x", hHandle);
	return 0;
}

SEGASTATUS SEGAAPI_UpdateBuffer(CTHANDLE hHandle, CTDWORD dwStartOffset, CTDWORD dwLength)
{
	dbgPrint("SEGAAPI_UpdateBuffer() 0x%x 0x%x 0x%x", hHandle, dwStartOffset, dwLength);
	if (hHandle == NULL)
	{
		dbgPrint("SEGAAPI_UpdateBuffer() SEGAERR_BAD_HANDLE");
		return SEGAERR_BAD_HANDLE;
	}
	segaapiContext_t *context = hHandle;
	updateBufferData(context, dwStartOffset, dwLength);
	return SEGA_SUCCESS;
}

SEGASTATUS SEGAAPI_SetSynthParam(CTHANDLE hHandle, HASYNTHPARAMSEXT param, CTLONG lPARWValue)
{
	float volume;
	float semiTones;
	float freqRatio;

	dbgPrint("SEGAAPI_SetSynthParam() 0x%x 0x%x 0x%x", hHandle, param, lPARWValue);

	segaapiContext_t *context = hHandle;
	if (context == NULL)
		return SEGAERR_BAD_PARAM;

	enum
	{
		StartAddrsOffset,
		EndAddrsOffset,
		StartloopAddrsOffset,
		EndloopAddrsOffset,
		StartAddrsCoarseOffset,
		ModLfoToPitch,
		VibLfoToPitch,
		ModEnvToPitch,
		InitialFilterFc,
		InitialFilterQ,
		ModLfoToFilterFc,
		ModEnvToFilterFc,
		EndAddrsCoarseOffset,
		ModLfoToVolume,
		Unused1,
		ChorusEffectsSend,
		ReverbEffectsSend,
		Pan,
		Unused2,
		Unused3,
		Unused4,
		DelayModLFO,
		FreqModLFO,
		DelayVibLFO,
		FreqVibLFO,
		DelayModEnv,
		AttackModEnv,
		HoldModEnv,
		DecayModEnv,
		SustainModEnv,
		ReleaseModEnv,
		KeynumToModEnvHold,
		KeynumToModEnvDecay,
		DelayVolEnv,
		AttackVolEnv,
		HoldVolEnv,
		DecayVolEnv,
		SustainVolEnv,
		ReleaseVolEnv,
		KeynumToVolEnvHold,
		KeynumToVolEnvDecay,
		Instrument,
		Reserved1,
		KeyRange,
		VelRange,
		StartloopAddrsCoarseOffset,
		Keynum,
		Velocity,
		InitialAttenuation,
		Reserved2,
		EndloopAddrsCoarseOffset,
		CoarseTune,
		FineTune,
		SampleID,
		SampleModes,
		Reserved3,
		ScaleTuning,
		ExclusiveClass,
		OverridingRootKey,
		Unused5,
		EndOper
	};

	int mapping[26] = {
		InitialAttenuation, ///< 0,         0x00,  initialAttenuation
		FineTune,			///< 1,         0x01,  fineTune + coarseTune * 100
		InitialFilterFc,	///< 2,         0x02,  initialFilterFc
		InitialFilterQ,		///< 3,         0x03,  initialFilterQ
		DelayVolEnv,		///< 4,         0x04,  delayVolEnv
		AttackVolEnv,		///< 5,         0x05,  attackVolEnv
		HoldVolEnv,			///< 6,         0x06,  holdVolEnv
		DecayVolEnv,		///< 7,         0x07,  decayVolEnv
		SustainVolEnv,		///< 8,         0x08,  sustainVolEnv
		ReleaseVolEnv,		///< 9,         0x09,  releaseVolEnv
		DelayModEnv,		///< 10,        0x0A,  delayModEnv
		AttackModEnv,		///< 11,        0x0B,  attackModEnv
		HoldModEnv,			///< 12,        0x0C,  holdModEnv
		DecayModEnv,		///< 13,        0x0D,  decayModEnv
		SustainModEnv,		///< 14,        0x0E,  sustainModEnv
		ReleaseModEnv,		///< 15,        0x0F,  releaseModEnv
		DelayModLFO,		///< 16,        0x10,  delayModLFO
		FreqModLFO,			///< 17,        0x11,  freqModLFO
		DelayVibLFO,		///< 18,        0x12,  delayVibLFO
		FreqVibLFO,			///< 19,        0x13,  freqVibLFO
		ModLfoToPitch,		///< 20,        0x14,  modLfoToPitch
		VibLfoToPitch,		///< 21,        0x15,  vibLfoToPitch
		ModLfoToFilterFc,	///< 22,        0x16,  modLfoToFilterFc
		ModLfoToVolume,		///< 23,        0x17,  modLfoToVolume
		ModEnvToPitch,		///< 24,        0x18,  modEnvToPitch
		ModEnvToFilterFc	///< 25,        0x19,  modEnvToFilterFc
	};

	int realParam = mapping[param];

	switch (param)
	{
	case HAVP_ATTENUATION:
		volume = tsf_decibelsToGain(0.0f - lPARWValue / 10.0f);
		alListenerf(AL_GAIN, volume);
		// buffer->xaVoice->SetVolume(volume);
		dbgPrint("SEGAAPI_SetSynthParam() HAVP_ATTENUATION gain: %f dB: %d", volume, lPARWValue);
		break;
	case HAVP_PITCH:
		semiTones = lPARWValue / 100.0f;
		// freqRatio = XAudio2SemitonesToFrequencyRatio(semiTones);
		//  http://www-personal.umich.edu/~bazald/l/api/_x_audio2_8h_source.html
		freqRatio = powf(2.0f, semiTones / 12.0f);

		// buffer->xaVoice->SetFrequencyRatio(freqRatio);
		alSourcef(context->alSource, AL_PITCH, freqRatio);
		dbgPrint("SEGAAPI_SetSynthParam() HAVP_PITCH hHandle: %08X semitones: %f freqRatio: %f", hHandle, semiTones, freqRatio);
		break;
	default:
		dbgPrint("SEGAAPI_SetSynthParam() unsupported param: 0x%x", param);
	}

	return SEGAERR_UNSUPPORTED;
}

CTLONG SEGAAPI_GetSynthParam(CTHANDLE hHandle, HASYNTHPARAMSEXT param)
{
	dbgPrint("SEGAAPI_GetSynthParam() 0x%x 0x%x", hHandle, param);
	return 0;
}

SEGASTATUS SEGAAPI_SetSynthParamMultiple(CTHANDLE hHandle, CTDWORD dwNumParams, SynthParamSet *pSynthParams)
{
	dbgPrint("SEGAAPI_SetSynthParamMultiple() 0x%x 0x%x 0x%x", hHandle, dwNumParams, pSynthParams);
	segaapiContext_t *context = hHandle;
	if (context == NULL)
		return SEGAERR_BAD_PARAM;

	for (int i = 0; i < dwNumParams; i++)
	{
		SEGAAPI_SetSynthParam(hHandle, pSynthParams[i].param, pSynthParams[i].lPARWValue);
	}

	return SEGAERR_UNSUPPORTED;
}

SEGASTATUS SEGAAPI_GetSynthParamMultiple(CTHANDLE hHandle, CTDWORD dwNumParams, SynthParamSet *pSynthParams)
{
	dbgPrint("SEGAAPI_GetSynthParamMultiple() 0x%x 0x%x 0x%x", hHandle, dwNumParams, pSynthParams);
	return SEGAERR_UNSUPPORTED;
}

SEGASTATUS SEGAAPI_SetReleaseState(CTHANDLE hHandle, CTBOOL bSet)
{
	dbgPrint("SEGAAPI_SetReleaseState() 0x%x 0x%x", hHandle, bSet);
	return SEGAERR_UNSUPPORTED;
}

SEGASTATUS SEGAAPI_CreateBuffer(HAWOSEBUFFERCONFIG *pConfig, HAWOSEGABUFFERCALLBACK pCallback, CTDWORD dwFlags, CTHANDLE *phHandle)
{
	dbgPrint("SEGAAPI_CreateBuffer() 0x%x 0x%x 0x%x 0x%x", pConfig, pCallback, dwFlags, phHandle);
	if ((phHandle == NULL) || (pConfig == NULL))
	{
		g_LastStatus = SEGAERR_BAD_POINTER;
		dbgPrint("SEGAAPI_CreateBuffer() SEGAERR_BAD_POINTER");
		return SEGAERR_BAD_POINTER;
	}
	segaapiContext_t *context = malloc(sizeof(segaapiContext_t));
	if (context == NULL)
	{
		dbgPrint("SEGAAPI_CreateBuffer() SEGAERR_OUT_OF_MEMORY");
		return SEGAERR_OUT_OF_MEMORY;
	}
	// dbgPrint("SEGAAPI_CreateBuffer() allocated %i bytes",sizeof(segaapiContext_t));
	context->playing = false;
	context->callback = pCallback;
	context->synthesizer = dwFlags & HABUF_SYNTH_BUFFER;
	context->sampleRate = pConfig->dwSampleRate;
	context->sampleFormat = pConfig->dwSampleFormat;
	context->channels = pConfig->byNumChans;
	context->userData = pConfig->hUserData;
	context->size = pConfig->mapData.dwSize;
	pConfig->mapData.dwOffset = 0;

	// can't have all 3 types at once - sanity check
	if ((dwFlags & 0x06) == 0x06)
	{
		dbgPrint("SEGAAPI_CreateBuffer() SEGAERR_BAD_PARAM");
		free(context);
		return SEGAERR_BAD_PARAM;
	}
	// indiate that caller allocate memory
	if (dwFlags & HABUF_ALLOC_USER_MEM)
	{
		context->data = pConfig->mapData.hBufferHdr;
		dbgPrint("SEGAAPI_CreateBuffer() user memory 0x%x", context->data);
	}
	// reuse memory
	else if (dwFlags & HABUF_USE_MAPPED_MEM)
	{
		context->data = pConfig->mapData.hBufferHdr;
		if (context->data == NULL)
		{
			// null pointer, allocate memory
			context->data = malloc(context->size);
			if (context->data == NULL)
			{
				dbgPrint("SEGAAPI_CreateBuffer() SEGAERR_OUT_OF_MEMORY");
				return SEGAERR_OUT_OF_MEMORY;
			}
			dbgPrint("SEGAAPI_CreateBuffer() bad pointer, allocated %i data bytes", context->size);
		}
		else
			dbgPrint("SEGAAPI_CreateBuffer() reusing memory 0x%x", context->data);
	}
	// Allocate new buffer (caller will fill it later)
	else
	{
		context->data = malloc(context->size);
		if (context->data == NULL)
		{
			dbgPrint("SEGAAPI_CreateBuffer() SEGAERR_OUT_OF_MEMORY");
			return SEGAERR_OUT_OF_MEMORY;
		}
		dbgPrint("SEGAAPI_CreateBuffer() allocated %i data bytes", context->size);
	}

#ifdef DEBUG_SAMPLE
	dbgPrint("SEGAAPI_CreateBuffer()  dwPriority = 0x%08X;         // The priority with which the voices should be allocated.  This is used when voices need to be ripped off.", pConfig->dwPriority);
	dbgPrint("SEGAAPI_CreateBuffer()  dwSampleRate = %u;       // The sample rate the voice desires", pConfig->dwSampleRate);
	dbgPrint("SEGAAPI_CreateBuffer()  dwSampleFormat = 0x%08X;     // The sample format the voice will use", pConfig->dwSampleFormat);
	dbgPrint("SEGAAPI_CreateBuffer()  byNumChans = 0x%08X;         // The number of samples in the sample frame. (1 = mono, 2 = stereo).", pConfig->byNumChans);
	dbgPrint("SEGAAPI_CreateBuffer()  dwReserved = 0x%08X;         // Reserved field", pConfig->dwReserved);
	dbgPrint("SEGAAPI_CreateBuffer()  hUserData = 0x%08X;          // User data", (uintptr_t)pConfig->hUserData);
	dbgPrint("SEGAAPI_CreateBuffer()  mapData {                   // The sample memory mapping for the buffer.");
	dbgPrint("SEGAAPI_CreateBuffer()  dwSize = 0x%08X;       // Supply by caller. Size (in bytes) of the valid sample data", pConfig->mapData.dwSize);
	dbgPrint("SEGAAPI_CreateBuffer()  dwOffset = 0x%08X;     // Return by driver. Offset of buffer where the the first valid sample should be written to", pConfig->mapData.dwOffset);
	dbgPrint("SEGAAPI_CreateBuffer()  hBufferHdr = 0x%08X;   // Memory address that user-space application can access, or mapped memory handle.", (uintptr_t)pConfig->mapData.hBufferHdr);
	dbgPrint("SEGAAPI_CreateBuffer()  }");
#endif

	pConfig->mapData.hBufferHdr = context->data;

	alGenBuffers(1, &context->alBuffer);
	alGenSources(1, &context->alSource);

	/*
	TODO:
	* HABUF_ALLOC_USER_MEM bit when set indicates caller allocate sound data memory buffer.
	* HABUF_USE_MAPPED_MEM
	Can't be used at the same time!!!
	*/
	if (context->synthesizer)
	{
		dbgPrint("SEGAAPI_CreateBuffer() !!! Doesn't support synth buffers yet!");
		// https://stackoverflow.com/questions/44157238/can-i-produce-a-synthetic-sound-using-openal
	}

	resetBuffer(context);
	*phHandle = context;

	return SEGA_SUCCESS;
}

SEGASTATUS SEGAAPI_DestroyBuffer(CTHANDLE hHandle)
{
	dbgPrint("SEGAAPI_DestroyBuffer() 0x%x", hHandle);
	if (hHandle == NULL)
		return SEGAERR_BAD_PARAM;
	free(hHandle);
	return SEGA_SUCCESS;
}

CTBOOL SEGAAPI_SetGlobalEAXProperty(GUID *guid, unsigned long ulProperty, void *pData, unsigned long ulDataSize)
{
	dbgPrint("SEGAAPI_SetGlobalEAXProperty() 0x%x 0x%x 0x%x 0x%x", guid, ulProperty, pData, ulDataSize);
	return 0;
}

CTBOOL SEGAAPI_GetGlobalEAXProperty(GUID *guid, unsigned long ulProperty, void *pData, unsigned long ulDataSize)
{
	dbgPrint("SEGAAPI_GetGlobalEAXProperty() 0x%x 0x%x 0x%x 0x%x", guid, ulProperty, pData, ulDataSize);
	return 0;
}

SEGASTATUS SEGAAPI_SetSPDIFOutChannelStatus(CTDWORD dwChannelStatus, CTDWORD dwExtChannelStatus)
{
	dbgPrint("SEGAAPI_SetSPDIFOutChannelStatus() 0x%x 0x%x", dwChannelStatus, dwExtChannelStatus);
	return SEGAERR_UNSUPPORTED;
}

SEGASTATUS SEGAAPI_GetSPDIFOutChannelStatus(CTDWORD *pdwChannelStatus, CTDWORD *pdwExtChannelStatus)
{
	dbgPrint("SEGAAPI_GetSPDIFOutChannelStatus() 0x%x 0x%x", pdwChannelStatus, pdwExtChannelStatus);
	return SEGAERR_UNSUPPORTED;
}

SEGASTATUS SEGAAPI_SetSPDIFOutSampleRate(HASPDIFOUTRATE dwSamplingRate)
{
	dbgPrint("SEGAAPI_SetSPDIFOutSampleRate() 0x%x", dwSamplingRate);
	return SEGAERR_UNSUPPORTED;
}

HASPDIFOUTRATE SEGAAPI_GetSPDIFOutSampleRate(void)
{
	dbgPrint("SEGAAPI_GetSPDIFOutSampleRate()");
	return HASPDIFOUT_48KHZ;
}

SEGASTATUS SEGAAPI_SetSPDIFOutChannelRouting(CTDWORD dwChannel, HAROUTING dwSource)
{
	switch (dwChannel)
	{
	case 0: // left
		dbgPrint("SEGAAPI_SetSPDIFOutChannelRouting() dwChannel = LEFT; dwSource = 0x%x", dwSource);
		break;
	case 1: // right
		dbgPrint("SEGAAPI_SetSPDIFOutChannelRouting() dwChannel = RIGHT; dwSource = 0x%x", dwSource);
		break;
	default:
		dbgPrint("SEGAAPI_SetSPDIFOutChannelRouting() dwChannel = UNKNOWN; dwSource = 0x%x", dwSource);
		break;
	}
	return SEGAERR_UNSUPPORTED;
}

HAROUTING SEGAAPI_GetSPDIFOutChannelRouting(CTDWORD dwChannel)
{
	dbgPrint("SEGAAPI_GetSPDIFOutChannelRouting() 0x%x", dwChannel);
	return HA_UNUSED_PORT;
}

SEGASTATUS SEGAAPI_SetIOVolume(HAPHYSICALIO dwPhysIO, CTDWORD dwVolume)
{
	// float v = (dwVolume >> 16) & 0xffff;
	dbgPrint("SEGAAPI_SetIOVolume() 0x%x 0x%x", dwPhysIO, dwVolume);
	// alListenerf(AL_GAIN, v);
	return SEGA_SUCCESS;
}

CTDWORD SEGAAPI_GetIOVolume(HAPHYSICALIO dwPhysIO)
{
	dbgPrint("SEGAAPI_GetIOVolume() 0x%x", dwPhysIO);
	return 0xffffffff;
}

void SEGAAPI_SetLastStatus(SEGASTATUS LastStatus)
{
	dbgPrint("SEGAAPI_SetLastStatus() 0x%x", LastStatus);
	return;
}

SEGASTATUS SEGAAPI_GetLastStatus(void)
{
	dbgPrint("SEGAAPI_GetLastStatus()");
	return SEGA_SUCCESS;
}

SEGASTATUS SEGAAPI_Reset(void)
{
	dbgPrint("SEGAAPI_Reset()");
	return SEGA_SUCCESS;
}

SEGASTATUS SEGAAPI_Init(void)
{
	dbgPrint("SEGAAPI_Init()");

	int res = alutInit(NULL, NULL);
	if (res == AL_FALSE)
	{
		dbgPrint("SEGAAPI_Init() alutInit failed");
		return SEGAERR_FAIL;
	}

	alBufferSamplesSOFT = alGetProcAddress("alBufferSamplesSOFT");
	if (alBufferSamplesSOFT == NULL)
	{
		dbgPrint("Could not resolve AL extension!\n");
		//exit(1);
	}

	alBufferSubSamplesSOFT = alGetProcAddress("alBufferSubSamplesSOFT");
	if (alBufferSubSamplesSOFT == NULL)
	{
		dbgPrint("Could not resolve AL extension!\n");
		//exit(1);
	}
	alGetBufferSamplesSOFT = alGetProcAddress("alGetBufferSamplesSOFT");
	if (alGetBufferSamplesSOFT == NULL)
	{
		dbgPrint("Could not resolve AL extension!\n");
		//exit(1);
	}

	SEGAAPI_SetGlobalEAXProperty((GUID *)&EAXPROPERTYID_EAX40_FXSlot2, 0, (void *)&EAX_NULL_GUID, 16);

	SEGAAPI_SetSPDIFOutChannelRouting(0, 0);
	SEGAAPI_SetSPDIFOutChannelRouting(1, 1);
	SEGAAPI_SetSPDIFOutSampleRate(1);
	return SEGA_SUCCESS;
}

SEGASTATUS SEGAAPI_Exit(void)
{
	dbgPrint("SEGAAPI_Exit()");
	alutExit();
	return SEGA_SUCCESS;
}
