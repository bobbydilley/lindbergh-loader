#define TSF_IMPLEMENTATION

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include <AL/alut.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "segaapi.h"
#include "tsf.h"

// #define DEBUG_OUTPUT

#define NUM_BUFFERS 400

const GUID EAX_NULL_GUID;
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
	// SEGA API Parts
	void *userData;
	BufferCallback callback;
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

	int usedBuffers;
	// OpenAL Parts
	ALuint alBuffer[NUM_BUFFERS];
	ALuint alSource;

	// TinySoundFont Parts
	tsf *synth;
	struct tsf_region *region;
} SEGAContext;

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

/**
 * Returns the OpenAL Format Enum from the sampleFormat and channels
 * of the SEGA API.
 *
 * @param sampleFormat SEGA API Sample Format
 * @param channels Amount of channels to use
 * @returns The OpenAL Format
 */
ALenum getAlFormat(unsigned int sampleFormat, unsigned int channels)
{
	ALenum alFormat = -1;

	switch (sampleFormat)
	{
	case UNSIGNED_8PCM: /* Unsigned (offset 128) 8-bit PCM */
		switch (channels)
		{
		case 1:
			alFormat = AL_MONO8_SOFT;
			break;
		case 2:
			alFormat = AL_STEREO8_SOFT;
			break;
		default:
			break;
		}
		break;
	case SIGNED_16PCM: /* Signed 16-bit PCM */
		switch (channels)
		{
		case 1:
			alFormat = AL_MONO16_SOFT;
			break;
		case 2:
			alFormat = AL_STEREO16_SOFT;
			break;
		default:
			break;
		}
	default:
		break;
	}

	if (alFormat == -1)
	{
		printf("SEGAAPI Fatal Error: Unknown format - 0x%X with %d channels!\n", sampleFormat, channels);
		abort();
	}

	return alFormat;
}

static void updateBufferData(SEGAContext *context, unsigned int offset, size_t length)
{
	printf("Update buffer data - handle %p offset %d length %d\n", context, offset, length);

	if (length < 1)
		return;

	ALuint alBuffer;
	alGenBuffers(1, &alBuffer);
	alBufferData(alBuffer, getAlFormat(context->sampleFormat, context->channels), context->data + offset, length, context->sampleRate);
	alSourceQueueBuffers(context->alSource, 1, &alBuffer);

}

static void resetBuffer(SEGAContext *context)
{ // printf("%s %d\n", __func__, __LINE__);
	// *   - Send Routing
	// *      - for 1 channel buffer, channel is routed to Front-Left and Front-Right.
	// *      - for 2 channel buffer, channel 0 is routed Front-Left, channel 1 is routed Front-Right
	// *   - Send Levels are set to 0 (infinite attenuation)
	// *   - Channel Volume is set to 0xFFFFFFFF (no attenuation)
	// *   - No notification.
	// *   - StartLoopOffset is set to 0.
	// *   - EndLoopOffset and EndOffset are set to pConfig->mapdata.dwSize.
	// *   - No loop.

	context->startLoop = 0;
	context->endOffset = context->size;
	context->endLoop = context->size;
	context->loop = false;
	context->paused = false;
	context->usedBuffers = 0;

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

	alSourcei(context->alSource, AL_BUFFER, AL_NONE);
	alSourcei(context->alSource, AL_BYTE_OFFSET, 0);
	alSourceStop(context->alSource);
}

int SEGAAPI_Play(void *hHandle)
{
	dbgPrint("SEGAAPI_Play() 0x%x", hHandle);
	printf("play called %p\n", hHandle);

	SEGAContext *context = hHandle;

	if (context == NULL)
		return SEGA_ERROR_BAD_PARAM;

	alSourcePlay(context->alSource);

	return SEGA_SUCCESS;
}

int SEGAAPI_Pause(void *hHandle)
{
	dbgPrint("SEGAAPI_Pause() 0x%x", hHandle);

	SEGAContext *context = hHandle;

	if (context == NULL)
		return SEGA_ERROR_BAD_PARAM;

	alSourcePause(context->alSource);

	return SEGA_SUCCESS;
}

int SEGAAPI_Stop(void *hHandle)
{
	dbgPrint("SEGAAPI_Stop() 0x%x", hHandle);

	SEGAContext *context = hHandle;

	if (context == NULL)
		return SEGA_ERROR_BAD_PARAM;

	alSourceStop(context->alSource);

	return SEGA_SUCCESS;
}

int SEGAAPI_PlayWithSetup(void *hHandle)
{
	dbgPrint("SEGAAPI_PlayWithSetup() 0x%x", hHandle);

	SEGAContext *context = hHandle;

	if (context == NULL)
		return SEGA_ERROR_BAD_PARAM;

	alSourcePlay(context->alSource);

	return SEGA_ERROR_UNSUPPORTED;
}

PlaybackStatus SEGAAPI_GetPlaybackStatus(void *hHandle)
{
	dbgPrint("SEGAAPI_GetPlaybackStatus() 0x%x", hHandle);

	SEGAContext *context = hHandle;
	if (context == NULL)
		return PLAYBACK_STATUS_INVALID;

	ALint state;
	alGetSourcei(context->alSource, AL_SOURCE_STATE, &state);

	switch (state)
	{
	case AL_PLAYING:
		printf("%p get playback status return active\n", hHandle);
		return PLAYBACK_STATUS_ACTIVE;
	case AL_PAUSED:
		printf("%p get playback status return pause\n", hHandle);

		return PLAYBACK_STATUS_PAUSE;
	case AL_INITIAL:
		printf("%p get playback status return active\n", hHandle);

		return PLAYBACK_STATUS_ACTIVE;
	case AL_STOPPED:
		printf("%p get playback status return stopped\n", hHandle);

		return PLAYBACK_STATUS_STOP;
	default:
		printf("%p get playback status return invalid\n", hHandle);

		return PLAYBACK_STATUS_INVALID;
	}

	return PLAYBACK_STATUS_INVALID;
}

int SEGAAPI_SetFormat(void *hHandle, OutputFormat *pFormat)
{
	dbgPrint("SEGAAPI_SetFormat() 0x%x", hHandle);
	return SEGA_ERROR_UNSUPPORTED;
}

int SEGAAPI_GetFormat(void *hHandle, OutputFormat *pFormat)
{
	dbgPrint("SEGAAPI_GetFormat() 0x%x", hHandle);
	return SEGA_ERROR_UNSUPPORTED;
}

int SEGAAPI_SetSampleRate(void *hHandle, unsigned int dwSampleRate)
{
	dbgPrint("SEGAAPI_SetSampleRate() 0x%x 0x%x", hHandle, dwSampleRate);

	if (hHandle == NULL)
		return SEGA_ERROR_BAD_HANDLE;

	SEGAContext *context = hHandle;
	context->sampleRate = dwSampleRate;

	return SEGA_SUCCESS;
}

unsigned int SEGAAPI_GetSampleRate(void *hHandle)
{
	dbgPrint("SEGAAPI_GetSampleRate() 0x%x", hHandle);
	if (hHandle == NULL)
		return SEGA_ERROR_BAD_HANDLE;

	SEGAContext *context = hHandle;

	return context->sampleRate;
}

int SEGAAPI_SetPriority(void *hHandle, unsigned int dwPriority)
{
	dbgPrint("SEGAAPI_SetPriority() 0x%x 0x%x", hHandle, dwPriority);
	return SEGA_ERROR_UNSUPPORTED;
}

unsigned int SEGAAPI_GetPriority(void *hHandle)
{
	dbgPrint("SEGAAPI_GetPriority() 0x%x", hHandle);
	return SEGA_ERROR_UNSUPPORTED;
}

int SEGAAPI_SetUserData(void *hHandle, void *hUserData)
{
	dbgPrint("SEGAAPI_SetUserData() 0x%x 0x%x", hHandle, hUserData);
	if (hHandle == NULL)
		return SEGA_ERROR_BAD_HANDLE;

	SEGAContext *context = hHandle;
	context->userData = hUserData;
	return SEGA_SUCCESS;
}

void *SEGAAPI_GetUserData(void *hHandle)
{
	dbgPrint("SEGAAPI_GetPriority() 0x%x", hHandle);
	if (hHandle == NULL)
		return NULL;

	SEGAContext *context = hHandle;
	return context->userData;
}

int SEGAAPI_SetSendRouting(void *hHandle, unsigned int dwChannel, unsigned int dwSend, Routing dwDest)
{
	dbgPrint("SEGAAPI_SetSendRouting() 0x%x 0x%x 0x%x 0x%x", hHandle, dwChannel, dwSend, dwDest);
	return SEGA_SUCCESS;
}

Routing SEGAAPI_GetSendRouting(void *hHandle, unsigned int dwChannel, unsigned int dwSend)
{
	dbgPrint("SEGAAPI_GetSendRouting() 0x%x 0x%x 0x%x", hHandle, dwChannel, dwSend);
	return UNUSED_PORT;
}

int SEGAAPI_SetSendLevel(void *hHandle, unsigned int dwChannel, unsigned int dwSend, unsigned int dwLevel)
{
	dbgPrint("SEGAAPI_SetSendLevel() 0x%x 0x%x 0x%x 0x%x", hHandle, dwChannel, dwSend, dwLevel);
	return SEGA_SUCCESS;
}

unsigned int SEGAAPI_GetSendLevel(void *hHandle, unsigned int dwChannel, unsigned int dwSend)
{
	dbgPrint("SEGAAPI_GetSendLevel() 0x%x 0x%x 0x%x", hHandle, dwChannel, dwSend);
	return SEGA_SUCCESS;
}

int SEGAAPI_SetChannelVolume(void *hHandle, unsigned int dwChannel, unsigned int dwVolume)
{
	dbgPrint("SEGAAPI_SetChannelVolume() 0x%x 0x%x 0x%x", hHandle, dwChannel, dwVolume);
	return SEGA_ERROR_UNSUPPORTED;
}

unsigned int SEGAAPI_GetChannelVolume(void *hHandle, unsigned int dwChannel)
{
	dbgPrint("SEGAAPI_GetChannelVolume() 0x%x 0x%x", hHandle, dwChannel);
	return 0;
}

int SEGAAPI_SetPlaybackPosition(void *hHandle, unsigned int dwPlaybackPos)
{
	dbgPrint("SEGAAPI_SetPlaybackPosition() 0x%x 0x%x", hHandle, dwPlaybackPos);
	if (hHandle == NULL)
		return SEGA_ERROR_BAD_HANDLE;

	SEGAContext *context = hHandle;
	alSourcei(context->alSource, AL_BYTE_OFFSET, dwPlaybackPos);

	return SEGA_SUCCESS;
}

unsigned int SEGAAPI_GetPlaybackPosition(void *hHandle)
{
	dbgPrint("SEGAAPI_GetPlaybackPosition() 0x%x", hHandle);

	if (hHandle == NULL)
		return SEGA_ERROR_BAD_HANDLE;

	SEGAContext *context = hHandle;

	ALint position;
	alGetSourcei(context->alSource, AL_BYTE_OFFSET, &position);

	if(context->loop) {
		position = position % context->size;
	}

	printf("get playback position %p -> %d\n", hHandle, position);

	

	return position;
}

int SEGAAPI_SetNotificationFrequency(void *hHandle, unsigned int dwFrameCount)
{
	dbgPrint("SEGAAPI_SetNotificationFrequency() 0x%x 0x%x", hHandle, dwFrameCount);
	return SEGA_ERROR_UNSUPPORTED;
}

int SEGAAPI_SetNotificationPoint(void *hHandle, unsigned int dwBufferOffset)
{
	dbgPrint("SEGAAPI_SetNotificationPoint() 0x%x 0x%x", hHandle, dwBufferOffset);
	return SEGA_ERROR_UNSUPPORTED;
}

int SEGAAPI_ClearNotificationPoint(void *hHandle, unsigned int dwBufferOffset)
{
	dbgPrint("SEGAAPI_ClearNotificationPoint() 0x%x 0x%x", hHandle, dwBufferOffset);
	return SEGA_ERROR_UNSUPPORTED;
}

int SEGAAPI_SetStartLoopOffset(void *hHandle, unsigned int dwOffset)
{
	dbgPrint("SEGAAPI_SetStartLoopOffset() 0x%x 0x%x", hHandle, dwOffset);
	if (hHandle == NULL)
		return SEGA_ERROR_BAD_HANDLE;

	SEGAContext *context = hHandle;

	context->startLoop = dwOffset;

	return SEGA_SUCCESS;
}

unsigned int SEGAAPI_GetStartLoopOffset(void *hHandle)
{
	dbgPrint("SEGAAPI_GetStartLoopOffset() 0x%x", hHandle);
	if (hHandle == NULL)
		return SEGA_ERROR_BAD_HANDLE;

	SEGAContext *context = hHandle;

	return context->startLoop;
}

int SEGAAPI_SetEndLoopOffset(void *hHandle, unsigned int dwOffset)
{
	dbgPrint("SEGAAPI_SetEndLoopOffset() 0x%x 0x%x", hHandle, dwOffset);
	if (hHandle == NULL)
		return SEGA_ERROR_BAD_HANDLE;

	SEGAContext *context = hHandle;
	context->endLoop = dwOffset;

	return SEGA_SUCCESS;
}

unsigned int SEGAAPI_GetEndLoopOffset(void *hHandle)
{
	dbgPrint("SEGAAPI_GetEndLoopOffset() 0x%x", hHandle);
	if (hHandle == NULL)
		return SEGA_ERROR_BAD_HANDLE;

	SEGAContext *context = hHandle;

	return context->endLoop;
}

int SEGAAPI_SetEndOffset(void *hHandle, unsigned int dwOffset)
{
	dbgPrint("SEGAAPI_SetEndOffset() 0x%x 0x%x", hHandle, dwOffset);
	if (hHandle == NULL)
		return SEGA_ERROR_BAD_HANDLE;

	SEGAContext *context = hHandle;
	context->endOffset = dwOffset;

	return SEGA_SUCCESS;
}

unsigned int SEGAAPI_GetEndOffset(void *hHandle)
{
	dbgPrint("SEGAAPI_GetEndOffset() 0x%x", hHandle);
	if (hHandle == NULL)
		return SEGA_ERROR_BAD_HANDLE;

	SEGAContext *context = hHandle;

	return context->endOffset;
}

int SEGAAPI_SetLoopState(void *hHandle, int loop)
{
	dbgPrint("SEGAAPI_SetLoopState() 0x%x 0x%x", hHandle, loop);
	if (hHandle == NULL)
		return SEGA_ERROR_BAD_HANDLE;

	SEGAContext *context = hHandle;

	context->loop = loop;

	return SEGA_SUCCESS;
}

int SEGAAPI_GetLoopState(void *hHandle)
{
	dbgPrint("SEGAAPI_GetLoopState() 0x%x", hHandle);
	if (hHandle == NULL)
		return SEGA_ERROR_BAD_HANDLE;

	SEGAContext *context = hHandle;

	return context->loop;
}

int SEGAAPI_UpdateBuffer(void *hHandle, unsigned int dwStartOffset, unsigned int dwLength)
{
	dbgPrint("SEGAAPI_UpdateBuffer() 0x%x 0x%x 0x%x", hHandle, dwStartOffset, dwLength);
	if (hHandle == NULL)
		return SEGA_ERROR_BAD_HANDLE;

	SEGAContext *context = hHandle;
	updateBufferData(context, dwStartOffset, dwLength);
	return SEGA_SUCCESS;
}

int SEGAAPI_SetSynthParam(void *hHandle, SynthParams param, int lPARWValue)
{
	dbgPrint("SEGAAPI_SetSynthParam() 0x%x 0x%x 0x%x", hHandle, param, lPARWValue);

	SEGAContext *context = hHandle;

	if (context == NULL)
		return SEGA_ERROR_BAD_PARAM;

	switch (param)
	{
	case ATTENUATION:
	{
		float volume = tsf_decibelsToGain(0.0f - lPARWValue / 10.0f);
		alListenerf(AL_GAIN, volume);
		// buffer->xaVoice->SetVolume(volume);
		dbgPrint("SEGAAPI_SetSynthParam() HAVP_ATTENUATION gain: %f dB: %d", volume, lPARWValue);
	}
	break;

	case PITCH:
	{
		float semiTones = lPARWValue / 100.0f;
		// freqRatio = XAudio2SemitonesToFrequencyRatio(semiTones);
		//  http://www-personal.umich.edu/~bazald/l/api/_x_audio2_8h_source.html
		float freqRatio = powf(2.0f, semiTones / 12.0f);
		// buffer->xaVoice->SetFrequencyRatio(freqRatio);
		alSourcef(context->alSource, AL_PITCH, freqRatio);
		dbgPrint("SEGAAPI_SetSynthParam() HAVP_PITCH hHandle: %08X semitones: %f freqRatio: %f", hHandle, semiTones, freqRatio);
	}
	break;

	default:
		dbgPrint("SEGAAPI_SetSynthParam() unsupported param: 0x%x", param);
	}

	return SEGA_SUCCESS;
}

int SEGAAPI_GetSynthParam(void *hHandle, SynthParams param)
{
	dbgPrint("SEGAAPI_GetSynthParam() 0x%x 0x%x", hHandle, param);
	return 0;
}

int SEGAAPI_SetSynthParamMultiple(void *hHandle, unsigned int dwNumParams, SynthParamSet *pSynthParams)
{
	dbgPrint("SEGAAPI_SetSynthParamMultiple() 0x%x 0x%x 0x%x", hHandle, dwNumParams, pSynthParams);
	SEGAContext *context = hHandle;

	if (context == NULL)
		return SEGA_ERROR_BAD_HANDLE;

	for (int i = 0; i < dwNumParams; i++)
	{
		SEGAAPI_SetSynthParam(hHandle, pSynthParams[i].param, pSynthParams[i].lPARWValue);
	}

	return SEGA_SUCCESS;
}

int SEGAAPI_GetSynthParamMultiple(void *hHandle, unsigned int dwNumParams, SynthParamSet *pSynthParams)
{
	dbgPrint("SEGAAPI_GetSynthParamMultiple() 0x%x 0x%x 0x%x", hHandle, dwNumParams, pSynthParams);
	return SEGA_ERROR_UNSUPPORTED;
}

int SEGAAPI_SetReleaseState(void *hHandle, int enterReleasePhase)
{
	dbgPrint("SEGAAPI_SetReleaseState() 0x%x 0x%x", hHandle, enterReleasePhase);
	if (hHandle == NULL)
		return SEGA_ERROR_BAD_HANDLE;

	SEGAContext *context = hHandle;

	if (!enterReleasePhase)
		return SEGA_SUCCESS;

	context->playing = false;
	alSourceStop(context->alSource);

	return SEGA_SUCCESS;
}

int SEGAAPI_CreateBuffer(BufferConfig *pConfig, BufferCallback pCallback, unsigned int dwFlags, void **phHandle)
{
	dbgPrint("SEGAAPI_CreateBuffer() 0x%x 0x%x 0x%x 0x%x", pConfig, pCallback, dwFlags, phHandle);
	if ((phHandle == NULL) || (pConfig == NULL))
	{
		dbgPrint("SEGAAPI_CreateBuffer() SEGA_ERROR_BAD_POINTER");
		return SEGA_ERROR_BAD_POINTER;
	}

	SEGAContext *context = malloc(sizeof(SEGAContext));
	if (context == NULL)
	{
		dbgPrint("SEGAAPI_CreateBuffer() SEGA_ERROR_OUT_OF_MEMORY");
		return SEGA_ERROR_OUT_OF_MEMORY;
	}

	// dbgPrint("SEGAAPI_CreateBuffer() allocated %i bytes",sizeof(SEGAContext));
	context->playing = false;
	context->callback = pCallback;
	context->synthesizer = dwFlags & SYNTH_BUFFER;
	context->sampleRate = pConfig->dwSampleRate;
	context->sampleFormat = pConfig->dwSampleFormat;
	context->channels = pConfig->byNumChans;
	context->userData = pConfig->hUserData;
	context->size = pConfig->mapData.dwSize;
	pConfig->mapData.dwOffset = 0;

	// can't have all 3 types at once - sanity check
	if ((dwFlags & 0x06) == 0x06)
	{
		dbgPrint("SEGAAPI_CreateBuffer() SEGA_ERROR_BAD_PARAM");
		free(context);
		return SEGA_ERROR_BAD_PARAM;
	}

	// The caller allocates the buffer memory
	if (dwFlags & ALLOC_USER_MEM)
	{
		context->data = pConfig->mapData.hBufferHdr;
		dbgPrint("SEGAAPI_CreateBuffer() user memory 0x%x", context->data);
	}

	// We should reuse mapped memory for the buffer
	else if (dwFlags & USE_MAPPED_MEM)
	{
		context->data = pConfig->mapData.hBufferHdr;
		if (context->data == NULL)
		{
			// null pointer, allocate memory
			context->data = malloc(context->size);
			if (context->data == NULL)
			{
				dbgPrint("SEGAAPI_CreateBuffer() SEGA_ERROR_OUT_OF_MEMORY");
				return SEGA_ERROR_OUT_OF_MEMORY;
			}
			dbgPrint("SEGAAPI_CreateBuffer() bad pointer, allocated %i data bytes", context->size);
		}
		else
			dbgPrint("SEGAAPI_CreateBuffer() reusing memory 0x%x", context->data);
	}

	// We should allocate new buffer which the caller will fill
	else
	{
		context->data = malloc(context->size);
		if (context->data == NULL)
		{
			dbgPrint("SEGAAPI_CreateBuffer() SEGA_ERROR_OUT_OF_MEMORY");
			return SEGA_ERROR_OUT_OF_MEMORY;
		}
		dbgPrint("SEGAAPI_CreateBuffer() allocated %i data bytes", context->size);
	}

	pConfig->mapData.hBufferHdr = context->data;

	alGenBuffers(NUM_BUFFERS, context->alBuffer);
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

int SEGAAPI_DestroyBuffer(void *buffer)
{
	dbgPrint("SEGAAPI_DestroyBuffer() 0x%x", buffer);
	if (buffer == NULL)
		return SEGA_ERROR_BAD_PARAM;

	free(buffer);

	return SEGA_SUCCESS;
}

int SEGAAPI_SetGlobalEAXProperty(GUID *guid, unsigned long ulProperty, void *pData, unsigned long ulDataSize)
{
	dbgPrint("SEGAAPI_SetGlobalEAXProperty() 0x%x 0x%x 0x%x 0x%x", guid, ulProperty, pData, ulDataSize);
	return SEGA_SUCCESS;
}

int SEGAAPI_GetGlobalEAXProperty(GUID *guid, unsigned long ulProperty, void *pData, unsigned long ulDataSize)
{
	dbgPrint("SEGAAPI_GetGlobalEAXProperty() 0x%x 0x%x 0x%x 0x%x", guid, ulProperty, pData, ulDataSize);
	return SEGA_ERROR_UNSUPPORTED;
}

int SEGAAPI_SetSPDIFOutChannelStatus(unsigned int dwChannelStatus, unsigned int dwExtChannelStatus)
{
	dbgPrint("SEGAAPI_SetSPDIFOutChannelStatus() 0x%x 0x%x", dwChannelStatus, dwExtChannelStatus);
	return SEGA_ERROR_UNSUPPORTED;
}

int SEGAAPI_GetSPDIFOutChannelStatus(unsigned int *pdwChannelStatus, unsigned int *pdwExtChannelStatus)
{
	dbgPrint("SEGAAPI_GetSPDIFOutChannelStatus() 0x%x 0x%x", pdwChannelStatus, pdwExtChannelStatus);
	return SEGA_ERROR_UNSUPPORTED;
}

int SEGAAPI_SetSPDIFOutSampleRate(SPDIFOutputSampleRate dwSamplingRate)
{
	dbgPrint("SEGAAPI_SetSPDIFOutSampleRate() 0x%x", dwSamplingRate);
	return SEGA_ERROR_UNSUPPORTED;
}

SPDIFOutputSampleRate SEGAAPI_GetSPDIFOutSampleRate(void)
{
	dbgPrint("SEGAAPI_GetSPDIFOutSampleRate()");
	return SPDIFOUT_48KHZ;
}

int SEGAAPI_SetSPDIFOutChannelRouting(unsigned int dwChannel, Routing dwSource)
{
	switch (dwChannel)
	{
	case OUT_FRONT_LEFT:
		dbgPrint("SEGAAPI_SetSPDIFOutChannelRouting() dwChannel = LEFT; dwSource = 0x%x", dwSource);
		break;
	case OUT_FRONT_RIGHT:
		dbgPrint("SEGAAPI_SetSPDIFOutChannelRouting() dwChannel = RIGHT; dwSource = 0x%x", dwSource);
		break;
	default:
		dbgPrint("SEGAAPI_SetSPDIFOutChannelRouting() dwChannel = UNKNOWN; dwSource = 0x%x", dwSource);
		break;
	}
	return SEGA_ERROR_UNSUPPORTED;
}

Routing SEGAAPI_GetSPDIFOutChannelRouting(unsigned int dwChannel)
{
	dbgPrint("SEGAAPI_GetSPDIFOutChannelRouting() 0x%x", dwChannel);
	return UNUSED_PORT;
}

int SEGAAPI_SetIOVolume(SoundBoardIO dwPhysIO, unsigned int dwVolume)
{
	// float v = (dwVolume >> 16) & 0xffff;
	dbgPrint("SEGAAPI_SetIOVolume() 0x%x 0x%x", dwPhysIO, dwVolume);
	// alListenerf(AL_GAIN, v);
	return SEGA_SUCCESS;
}

unsigned int SEGAAPI_GetIOVolume(SoundBoardIO dwPhysIO)
{
	dbgPrint("SEGAAPI_GetIOVolume() 0x%x", dwPhysIO);
	return 0xffffffff;
}

void SEGAAPI_SetLastStatus(int LastStatus)
{
	dbgPrint("SEGAAPI_SetLastStatus() 0x%x", LastStatus);
	return;
}

int SEGAAPI_GetLastStatus(void)
{
	dbgPrint("SEGAAPI_GetLastStatus()");
	return SEGA_SUCCESS;
}

int SEGAAPI_Reset(void)
{
	dbgPrint("SEGAAPI_Reset()");
	return SEGA_SUCCESS;
}

int SEGAAPI_Init(void)
{
	dbgPrint("SEGAAPI_Init()");

	if (alutInit(NULL, NULL) == AL_FALSE)
	{
		dbgPrint("SEGAAPI_Init() alutInit() failed!");
		return SEGA_ERROR_FAIL;
	}

	return SEGA_SUCCESS;
}

int SEGAAPI_Exit(void)
{
	dbgPrint("SEGAAPI_Exit()");
	alutExit();
	return SEGA_SUCCESS;
}
