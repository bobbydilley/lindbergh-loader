/*
* This file is part of the OpenParrot project - https://teknoparrot.com / https://github.com/teknogods
*
* See LICENSE and MENTIONS in the root of the source tree for information
* regarding licensing.
*
* Rewritten to use the FAudio lib by doozer.
*/

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "opensegaapi.h"
#include "segaerr.h"
#include "dqueue.h"

//#define _DEBUG

#define minfn(a,b)            (((a) < (b)) ? (a) : (b))
	
#include <vector>
#define XAUDIO2_HELPER_FUNCTIONS
#include <FAudio.h>
#define TSF_IMPLEMENTATION
#include "tsf.h"
#define CHECK_HR(exp) { HRESULT hr = exp; if (SEGA_FAILED(hr)) { printf("failed %s: %08x\n", #exp, hr); abort(); } }

extern "C" {
	GUID EAX_NULL_GUID;
	GUID EAX_FREQUENCYSHIFTER_EFFECT;
	GUID EAX_ECHO_EFFECT;
	GUID EAX_REVERB_EFFECT;
	GUID EAX_EQUALIZER_EFFECT;
	GUID EAX_DISTORTION_EFFECT;
	GUID EAX_AGCCOMPRESSOR_EFFECT;
	GUID EAX_PITCHSHIFTER_EFFECT;
	GUID EAX_FLANGER_EFFECT;
	GUID EAX_VOCALMORPHER_EFFECT;
	GUID EAX_AUTOWAH_EFFECT;
	GUID EAX_RINGMODULATOR_EFFECT;
	GUID EAX_CHORUS_EFFECT;

	GUID EAXPROPERTYID_EAX40_FXSlot0;
	GUID EAXPROPERTYID_EAX40_FXSlot1;
	GUID EAXPROPERTYID_EAX40_FXSlot2;
	GUID EAXPROPERTYID_EAX40_FXSlot3;
}

#include <functional>

struct OPEN_segaapiBuffer_t;

#ifdef _DEBUG
void dbgprint(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	printf("\r\n");
	fflush(stdout);
}
#else
void dbgprint(const char *format, ...)
{
	return;
}
#endif

void hexDump(const char *desc, void *addr, int len) 
{
	int i;
	unsigned char buff[17];
	unsigned char *pc = (unsigned char*)addr;

	// Output description if given.
	if (desc != NULL)
		printf("%s:\n", desc);

	if (len == 0) {
		printf("  ZERO LENGTH\n");
		return;
	}
	if (len < 0) {
		printf("  NEGATIVE LENGTH: %i\n", len);
		return;
	}

	// Process every byte in the data.
	for (i = 0; i < len; i++) {
		// Multiple of 16 means new line (with line offset).

		if ((i % 16) == 0) {
			// Just don't print ASCII for the zeroth line.
			if (i != 0)
				printf("  %s\n", buff);

			// Output the offset.
			printf("  %04x ", i);
		}

		// Now the hex code for the specific character.
		printf(" %02x", pc[i]);

		// And store a printable ASCII character for later.
		if ((pc[i] < 0x20) || (pc[i] > 0x7e))
			buff[i % 16] = '.';
		else
			buff[i % 16] = pc[i];
		buff[(i % 16) + 1] = '\0';
	}

	// Pad out last line if not exactly 16 characters.
	while ((i % 16) != 0) {
		printf("   ");
		i++;
	}

	// And print the final ASCII bit.
	printf("  %s\n", buff);
	fflush(stdout);
}

// Calculate the argument to SetFrequencyRatio from a semitone value
__inline float FAudioSemitonesToFrequencyRatio(float Semitones)
{
    // FrequencyRatio = 2 ^ Octaves
    //                = 2 ^ (Semitones / 12)
    return powf(2.0f, Semitones / 12.0f);
}

class FAudio_BufferNotify : public FAudioVoiceCallback {
public:
	OPEN_segaapiBuffer_t* buffer = NULL;
	queue_t * defers = NULL;
	FAudioSourceVoice* xaVoice;

    	FAudio_BufferNotify() {
        	OnBufferEnd = &FAudio_BufferNotify::StaticOnBufferEnd;
	        OnVoiceProcessingPassStart = &FAudio_BufferNotify::StaticOnVoiceProcessingPassStart;
	        OnVoiceProcessingPassEnd = &FAudio_BufferNotify::StaticOnVoiceProcessingPassEnd;
	        OnStreamEnd = &FAudio_BufferNotify::StaticOnStreamEnd;
	        OnBufferStart = &FAudio_BufferNotify::StaticOnBufferStart;
	        OnLoopEnd = &FAudio_BufferNotify::StaticOnLoopEnd;
	        OnVoiceError = &FAudio_BufferNotify::StaticOnVoiceError;
	}
	~FAudio_BufferNotify() = default;

private:
	void SignalBufferEnd() 
	{
        dbgprint("SignalBufferEnd()");
        FAudioSourceVoice* returned_voice;

        while (!queue_isempty(defers))
        {
                FAudioVoiceState vs;

                FAudioSourceVoice_GetState(xaVoice, &vs, 0);

                if (vs.BuffersQueued > 0)
                {
                        FAudioSourceVoice_FlushSourceBuffers(xaVoice);
                        return;
                }

                returned_voice = (FAudioSourceVoice*)queue_pop(defers);
                if (returned_voice)
                {
                        dbgprint("SignalBufferEnd: voice = %08x", returned_voice);
                        FAudioSourceVoice_FlushSourceBuffers(returned_voice);
                }
        }
    	
	}
	
	static void StaticOnBufferEnd(FAudioVoiceCallback* callback, void*) 
	{
        	static_cast<FAudio_BufferNotify*>(callback)->SignalBufferEnd();
    }
    static void StaticOnVoiceProcessingPassStart(FAudioVoiceCallback*, uint32_t) {}
    static void StaticOnVoiceProcessingPassEnd(FAudioVoiceCallback*) {}
    static void StaticOnStreamEnd(FAudioVoiceCallback*) {}
    static void StaticOnBufferStart(FAudioVoiceCallback*, void*) {}
    static void StaticOnLoopEnd(FAudioVoiceCallback*, void*) {}
    static void StaticOnVoiceError(FAudioVoiceCallback*, void*, uint32_t) {}
};

struct OPEN_segaapiBuffer_t
{
	void* userData;
	OPEN_HAWOSEGABUFFERCALLBACK callback;
	bool synthesizer;
	bool loop;
	unsigned int channels;
	unsigned int startLoop;
	unsigned int endLoop;
	unsigned int endOffset;
	unsigned int sampleRate;
	unsigned int sampleFormat;
	uint8_t* data;
	size_t size;
	bool playing;
	bool paused;
	bool playWithSetup;

	FAudioWaveFormatEx xaFormat;
	FAudioBuffer xaBuffer;

	float sendVolumes[7];
	int sendChannels[7];
	OPEN_HAROUTING sendRoutes[7];
	float channelVolumes[6];

	tsf* synth;
	tsf_region* region;

	FAudio_BufferNotify xaCallback;  // buffer end notification
};

void defer_buffer_call(FAudioSourceVoice* voice, queue_t* defers, uint32_t samplerate)
{
	dbgprint("defer_buffer_call()");
	if (voice)
	{
		FAudioVoiceState vs;
		dbgprint("defer_buffer_call: call FAudioSourceVoice_GetState");
		FAudioSourceVoice_GetState(voice, &vs, 0);
		dbgprint("defer_buffer_call: call complete: %i", vs.BuffersQueued);
		if (vs.BuffersQueued > 0)
		{
			dbgprint("defer_buffer_call: call queue_push voice = %08x", voice);
			queue_push((const void*)voice, defers);
			dbgprint("defer_buffer_call: call complete");

			FAudioSourceVoice_FlushSourceBuffers(voice);

			return;
		}
	}
	FAudioSourceVoice_SetSourceSampleRate(voice, samplerate);
}

static void dumpWaveBuffer(const char* path, unsigned int channels, unsigned int sampleRate, unsigned int sampleBits, void* data, size_t size)
{
	dbgprint("dumpWaveBuffer path %s channels %d sampleRate %d sampleBits %d size %d", path, channels, sampleRate, sampleBits, size);

	struct RIFF_Header
	{
		char chunkID[4];
		long chunkSize;
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
		char subChunkID[4];
		long subChunk2Size;
	};

	FILE* soundFile = NULL;
	struct WAVE_Format wave_format;
	struct RIFF_Header riff_header;
	struct WAVE_Data wave_data;

	soundFile = fopen(path, "wb");

	riff_header.chunkID[0] = 'R';
	riff_header.chunkID[1] = 'I';
	riff_header.chunkID[2] = 'F';
	riff_header.chunkID[3] = 'F';
	riff_header.format[0] = 'W';
	riff_header.format[1] = 'A';
	riff_header.format[2] = 'V';
	riff_header.format[3] = 'E';

	fwrite(&riff_header, sizeof(struct RIFF_Header), 1, soundFile);

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

	fwrite(&wave_format, sizeof(struct WAVE_Format), 1, soundFile);

	wave_data.subChunkID[0] = 'd';
	wave_data.subChunkID[1] = 'a';
	wave_data.subChunkID[2] = 't';
	wave_data.subChunkID[3] = 'a';

	wave_data.subChunk2Size = size;

	fwrite(&wave_data, sizeof(struct WAVE_Data), 1, soundFile);
	fwrite(data, wave_data.subChunk2Size, 1, soundFile);

	fclose(soundFile);
}

static unsigned int bufferSampleSize(OPEN_segaapiBuffer_t* buffer)
{
	return buffer->channels * ((buffer->sampleFormat == OPEN_HASF_SIGNED_16PCM) ? 2 : 1);
}

static void updateSynthOnPlay(OPEN_segaapiBuffer_t* buffer, unsigned int offset, size_t length)
{
	dbgprint("updateSynthOnPlay");
	// TODO
	//// synth
	//if (buffer->synthesizer)
	//{
	//	if (!buffer->synth->voices)
	//	{
	//		struct tsf_voice *voice = new tsf_voice;
	//		memset(voice, 0, sizeof(tsf_voice));

	//		auto region = buffer->region;

	//		TSF_BOOL doLoop; float filterQDB;
	//		voice->playingPreset = -1;

	//		voice->region = region;
	//		voice->noteGainDB = 0.0f - region->volume;

	//		tsf_voice_calcpitchratio(voice, 0, buffer->synth->outSampleRate);
	//		// The SFZ spec is silent about the pan curve, but a 3dB pan law seems common. This sqrt() curve matches what Dimension LE does; Alchemy Free seems closer to sin(adjustedPan * pi/2).
	//		voice->panFactorLeft = TSF_SQRTF(0.5f - region->pan);
	//		voice->panFactorRight = TSF_SQRTF(0.5f + region->pan);

	//		// Offset/end.
	//		voice->sourceSamplePosition = region->offset;

	//		// Loop.
	//		doLoop = (region->loop_mode != TSF_LOOPMODE_NONE && region->loop_start < region->loop_end);
	//		voice->loopStart = (doLoop ? region->loop_start : 0);
	//		voice->loopEnd = (doLoop ? region->loop_end : 0);

	//		// Setup envelopes.
	//		tsf_voice_envelope_setup(&voice->ampenv, &region->ampenv, 0, 0, TSF_TRUE, buffer->synth->outSampleRate);
	//		tsf_voice_envelope_setup(&voice->modenv, &region->modenv, 0, 0, TSF_FALSE, buffer->synth->outSampleRate);

	//		// Setup lowpass filter.
	//		filterQDB = region->initialFilterQ / 10.0f;
	//		voice->lowpass.QInv = 1.0 / TSF_POW(10.0, (filterQDB / 20.0));
	//		voice->lowpass.z1 = voice->lowpass.z2 = 0;
	//		voice->lowpass.active = (region->initialFilterFc </*=*/ 13500);
	//		if (voice->lowpass.active) tsf_voice_lowpass_setup(&voice->lowpass, tsf_cents2Hertz((float)region->initialFilterFc) / buffer->synth->outSampleRate);

	//		// Setup LFO filters.
	//		tsf_voice_lfo_setup(&voice->modlfo, region->delayModLFO, region->freqModLFO, buffer->synth->outSampleRate);
	//		tsf_voice_lfo_setup(&voice->viblfo, region->delayVibLFO, region->freqVibLFO, buffer->synth->outSampleRate);

	//		voice->pitchInputTimecents = (log(1.0) / log(2.0) * 1200);
	//		voice->pitchOutputFactor = 1.0f;

	//		buffer->synth->voices = voice;
	//	}

	//	buffer->synth->voices->region = buffer->region;

	//	// make input
	//	buffer->synth->outputmode = TSF_MONO;

	//	auto soffset = offset;
	//	auto slength = length;

	//	if (offset == -1)
	//	{
	//		soffset = 0;
	//	}

	//	if (length == -1)
	//	{
	//		slength = buffer->size;
	//	}

	//	std::vector<float> fontSamples(slength / bufferSampleSize(buffer));
	//	buffer->synth->fontSamples = &fontSamples[0];

	//	buffer->region->end = double(fontSamples.size());

	//	for (int i = 0; i < fontSamples.size(); i++)
	//	{
	//		if (buffer->sampleFormat == OPEN_HASF_UNSIGNED_8PCM)
	//		{
	//			fontSamples[i] = (buffer->data[soffset + i] / 128.0f) - 1.0f;
	//		}
	//		else if (buffer->sampleFormat == OPEN_HASF_SIGNED_16PCM)
	//		{
	//			fontSamples[i] = (*(int16_t*)&buffer->data[soffset + (i * 2)]) / 32768.0f;
	//		}
	//	}

	//	std::vector<float> outSamples(slength / bufferSampleSize(buffer));
	//	tsf_voice_render(buffer->synth, buffer->synth->voices, &outSamples[0], outSamples.size());

	//	for (int i = 0; i < outSamples.size(); i++)
	//	{
	//		if (buffer->sampleFormat == OPEN_HASF_UNSIGNED_8PCM)
	//		{
	//			buffer->data[soffset + i] = (uint8_t)((outSamples[i] + 1.0f) * 128.0f);
	//		}
	//		else if (buffer->sampleFormat == OPEN_HASF_SIGNED_16PCM)
	//		{
	//			*(int16_t*)&buffer->data[soffset + (i * 2)] = outSamples[i] * 32768.0f;
	//		}
	//	}
	//}
}

static void resetBuffer(OPEN_segaapiBuffer_t* buffer)
{
	dbgprint("resetBuffer %08x size:%d", buffer, buffer->size);
	if (buffer == NULL)
		return;
	buffer->startLoop = 0;
	buffer->endOffset = buffer->size;
	buffer->endLoop = buffer->size;
	buffer->loop = false;
	buffer->paused = false;
	buffer->playWithSetup = false;
	buffer->sendRoutes[0] = OPEN_HA_FRONT_LEFT_PORT;
	buffer->sendRoutes[1] = OPEN_HA_FRONT_RIGHT_PORT;
	buffer->sendRoutes[2] = OPEN_HA_UNUSED_PORT;
	buffer->sendRoutes[3] = OPEN_HA_UNUSED_PORT;
	buffer->sendRoutes[4] = OPEN_HA_UNUSED_PORT;
	buffer->sendRoutes[5] = OPEN_HA_UNUSED_PORT;
	buffer->sendRoutes[6] = OPEN_HA_UNUSED_PORT;
	buffer->sendVolumes[0] = 0.0f;
	buffer->sendVolumes[1] = 0.0f;
	buffer->sendVolumes[2] = 0.0f;
	buffer->sendVolumes[3] = 0.0f;
	buffer->sendVolumes[4] = 0.0f;
	buffer->sendVolumes[5] = 0.0f;
	buffer->sendVolumes[6] = 0.0f;
	buffer->channelVolumes[0] = 1.0f;
	buffer->channelVolumes[1] = 1.0f;
	buffer->channelVolumes[2] = 1.0f;
	buffer->channelVolumes[3] = 1.0f;
	buffer->channelVolumes[4] = 1.0f;
	buffer->channelVolumes[5] = 1.0f;
	buffer->sendChannels[0] = 0;
	buffer->sendChannels[1] = 1;
	buffer->sendChannels[2] = 0;
	buffer->sendChannels[3] = 0;
	buffer->sendChannels[4] = 0;
	buffer->sendChannels[5] = 0;
	buffer->sendChannels[6] = 0;

	auto res = (tsf*)TSF_MALLOC(sizeof(tsf));
	TSF_MEMSET(res, 0, sizeof(tsf));
	res->presetNum = 0;
	res->outSampleRate = buffer->sampleRate;

	buffer->synth = res;

	tsf_region* region = new tsf_region;
	memset(region, 0, sizeof(tsf_region));

	tsf_region_clear(region, 0);

	region->ampenv.delay = 0;
	region->ampenv.hold = 300.0f;
	region->ampenv.attack = 0;
	region->ampenv.decay = 0;
	region->ampenv.release = 0;
	region->ampenv.sustain = 0;

	buffer->region = region;
}

static FAudio* g_xa2;
static FAudioMasteringVoice* g_masteringVoice;
static FAudioSubmixVoice* g_submixVoices[6];

static void updateBufferNew(OPEN_segaapiBuffer_t* buffer, unsigned int offset, size_t length)
{
	dbgprint("updateBufferNew voice: %08x data: %p size: %d", buffer->xaCallback.xaVoice, buffer->data, buffer->size);

	// don't update with pending defers
	if (!queue_isempty(buffer->xaCallback.defers))
	{
		dbgprint("updateBufferNew: DEFER!");
		return;
	}

	CHECK_HR(FAudioSourceVoice_FlushSourceBuffers(buffer->xaCallback.xaVoice));

	buffer->xaBuffer.Flags = 0;
	buffer->xaBuffer.AudioBytes = buffer->size;
	buffer->xaBuffer.pAudioData = buffer->data;
#ifdef _DEBUG
	hexDump("updateBufferNew", buffer->data, 256);
#endif
	if (buffer->loop)
	{

		// Note: Sega uses byte offsets for begin and end
		//       Xaudio2 uses start sample and length in samples
		buffer->xaBuffer.PlayBegin = buffer->startLoop / bufferSampleSize(buffer);
		buffer->xaBuffer.PlayLength = (minfn(buffer->endLoop, buffer->endOffset) - buffer->startLoop) / bufferSampleSize(buffer);
		buffer->xaBuffer.LoopBegin = buffer->xaBuffer.PlayBegin;
		buffer->xaBuffer.LoopLength = buffer->xaBuffer.PlayLength;
		buffer->xaBuffer.LoopCount = FAUDIO_LOOP_INFINITE;
		buffer->xaBuffer.pContext = NULL;
		dbgprint("updateBufferNew: loop PlayBegin: %d PlayLength: %d", buffer->xaBuffer.PlayBegin, buffer->xaBuffer.PlayLength);
	}
	else
	{
		buffer->xaBuffer.PlayBegin = buffer->startLoop / bufferSampleSize(buffer);
		buffer->xaBuffer.PlayLength = (minfn(buffer->endLoop, buffer->endOffset) - buffer->startLoop) / bufferSampleSize(buffer);
		buffer->xaBuffer.LoopBegin = 0;
		buffer->xaBuffer.LoopLength = 0;
		buffer->xaBuffer.LoopCount = 0;
		buffer->xaBuffer.pContext = NULL;
		dbgprint("updateBufferNew: no loop PlayBegin: %d PlayLength: %d", buffer->xaBuffer.PlayBegin, buffer->xaBuffer.PlayLength);
	}

	dbgprint("updateBufferNew: call FAudioSourceVoice_SubmitSourceBuffer voice: %p buffer: %p",
		(void*)buffer->xaCallback.xaVoice, (void*)&buffer->xaBuffer);
	FAudioSourceVoice_SubmitSourceBuffer(buffer->xaCallback.xaVoice, &buffer->xaBuffer, NULL);
	dbgprint("updateBufferNew: call complete size: %d", buffer->size);

	// Uncomment to dump audio buffers to wav files (super slow)
	/*auto sampleBits = (buffer->sampleFormat == OPEN_HASF_SIGNED_16PCM) ? 16 : 8;
	char path[255];
	sprintf(path, "C:\\dump\\%08X.wav", &buffer);
	dumpWaveBuffer(path, buffer->channels, buffer->sampleRate, sampleBits, buffer->data, buffer->size);*/
}

extern "C" {
	OPEN_SEGASTATUS SEGAAPI_CreateBuffer(OPEN_HAWOSEBUFFERCONFIG* pConfig, OPEN_HAWOSEGABUFFERCALLBACK pCallback, unsigned int dwFlags, void* * phHandle)
	{
		if (phHandle == NULL || pConfig == NULL)
		{
			dbgprint("SEGAAPI_CreateBuffer: Handle: %08X, Status: OPEN_SEGAERR_BAD_POINTER", phHandle);
			return OPEN_SEGAERR_BAD_POINTER;
		}

		OPEN_segaapiBuffer_t* buffer = new OPEN_segaapiBuffer_t;

		dbgprint("SEGAAPI_CreateBuffer: hHandle: %08X synth: %d, mem caller: %d, mem last: %d, mem alloc: %d, size: %d SampleRate: %d, byNumChans: %d, dwPriority: %d, dwSampleFormat: %d", 
			buffer, 
			(dwFlags & OPEN_HABUF_SYNTH_BUFFER), 
			(dwFlags & OPEN_HABUF_ALLOC_USER_MEM) >> 1, 
			(dwFlags & OPEN_HABUF_USE_MAPPED_MEM) >> 2, 
			dwFlags == 0, 
			pConfig->mapData.dwSize, 
			pConfig->dwSampleRate, 
			pConfig->byNumChans, 
			pConfig->dwPriority, 
			pConfig->dwSampleFormat);

		buffer->xaCallback.defers = queue_init(20, 20, sizeof(FAudioSourceVoice*));
		if (buffer->xaCallback.defers == NULL)
			printf("queue_init failed!\r\n");

		buffer->playing = false;
		buffer->callback = pCallback;
		buffer->synthesizer = dwFlags & OPEN_HABUF_SYNTH_BUFFER;
		buffer->sampleFormat = pConfig->dwSampleFormat;
		buffer->sampleRate = pConfig->dwSampleRate;
		buffer->channels = pConfig->byNumChans;
		buffer->userData = pConfig->hUserData;
		buffer->size = pConfig->mapData.dwSize;
		pConfig->mapData.dwOffset = 0;

		// Use buffer supplied by caller
		if (dwFlags & OPEN_HABUF_ALLOC_USER_MEM)
		{
			buffer->data = (uint8_t*)pConfig->mapData.hBufferHdr;
		}
		// Reuse buffer
		else if (dwFlags & OPEN_HABUF_USE_MAPPED_MEM)
		{
			buffer->data = (uint8_t*)pConfig->mapData.hBufferHdr;
		}
		// Allocate new buffer (caller will fill it later)
		else
		{
			dbgprint("SEGAAPI_CreateBuffer malloc %d", buffer->size);
			buffer->data = (uint8_t*)malloc(buffer->size);
			memset(buffer->data, 0, buffer->size);
		}

		pConfig->mapData.hBufferHdr = buffer->data;

		auto sampleRate = pConfig->dwSampleRate;
		auto sampleBits = (pConfig->dwSampleFormat == OPEN_HASF_SIGNED_16PCM) ? 16 : 8;
		auto channels = pConfig->byNumChans;

		buffer->xaFormat.cbSize = sizeof(FAudioWaveFormatEx);
		buffer->xaFormat.nAvgBytesPerSec = (sampleRate * sampleBits * channels) / 8;
		buffer->xaFormat.nSamplesPerSec = sampleRate;
		buffer->xaFormat.wBitsPerSample = sampleBits;
		buffer->xaFormat.nChannels = channels;
		buffer->xaFormat.wFormatTag = 1;
		buffer->xaFormat.nBlockAlign = (sampleBits * channels) / 8;
		buffer->xaCallback.buffer = buffer;

		CHECK_HR(FAudio_CreateSourceVoice(g_xa2, 
			&buffer->xaCallback.xaVoice, 
			&buffer->xaFormat, 0, 2.0f, 
			&buffer->xaCallback, NULL, NULL));

		buffer->xaBuffer = { 0 };

		if (buffer->synthesizer)
		{
			// Not supported
		}
		resetBuffer(buffer);

		*phHandle = buffer;
		
		return OPEN_SEGA_SUCCESS;
	}

	OPEN_SEGASTATUS SEGAAPI_SetUserData(void* hHandle, void* hUserData)
	{
		if (hHandle == NULL)
		{
			dbgprint("SEGAAPI_SetUserData: Handle: %08X, Status: OPEN_SEGAERR_BAD_HANDLE", hHandle);
			return OPEN_SEGAERR_BAD_HANDLE;
		}

		dbgprint("SEGAAPI_SetUserData: Handle: %08X UserData: %08X", hHandle, hUserData);

		OPEN_segaapiBuffer_t* buffer = (OPEN_segaapiBuffer_t*)hHandle;
		buffer->userData = hUserData;
		return OPEN_SEGA_SUCCESS;
	}


	void* SEGAAPI_GetUserData(void* hHandle)
	{
		if (hHandle == NULL)
		{
			return nullptr;
		}

		dbgprint("SEGAAPI_GetUserData: Handle: %08X", hHandle);

		OPEN_segaapiBuffer_t* buffer = (OPEN_segaapiBuffer_t*)hHandle;
		return buffer->userData;
	}

	OPEN_SEGASTATUS SEGAAPI_UpdateBuffer(void* hHandle, unsigned int dwStartOffset, unsigned int dwLength)
	{
		if (hHandle == NULL)
		{
			dbgprint("SEGAAPI_UpdateBuffer: Handle: %08X, Status: OPEN_SEGAERR_BAD_HANDLE", hHandle);
			return OPEN_SEGAERR_BAD_HANDLE;
		}

		dbgprint("SEGAAPI_UpdateBuffer: Handle: %08X dwStartOffset: %08X, dwLength: %d", hHandle, dwStartOffset, dwLength);

		OPEN_segaapiBuffer_t* buffer = (OPEN_segaapiBuffer_t*)hHandle;

		updateBufferNew(buffer, dwStartOffset, dwLength);
		dbgprint("SEGAAPI_UpdateBuffer: complete");
		return OPEN_SEGA_SUCCESS;
	}

	OPEN_SEGASTATUS SEGAAPI_SetEndOffset(void* hHandle, unsigned int dwOffset)
	{
		if (hHandle == NULL)
		{
			dbgprint("SEGAAPI_SetEndOffset: Handle: %08X, Status: OPEN_SEGAERR_BAD_HANDLE", hHandle);
			return OPEN_SEGAERR_BAD_HANDLE;
		}

		dbgprint("SEGAAPI_SetEndOffset: Handle: %08X dwOffset: %08X", hHandle, dwOffset);

		OPEN_segaapiBuffer_t* buffer = (OPEN_segaapiBuffer_t*)hHandle;
		buffer->endOffset = dwOffset;
		return OPEN_SEGA_SUCCESS;
	}

	OPEN_SEGASTATUS SEGAAPI_SetEndLoopOffset(void* hHandle, unsigned int dwOffset)
	{
		if (hHandle == NULL)
		{
			dbgprint("SEGAAPI_SetEndLoopOffset: Handle: %08X, Status: OPEN_SEGAERR_BAD_HANDLE", hHandle);
			return OPEN_SEGAERR_BAD_HANDLE;
		}

		dbgprint("SEGAAPI_SetEndLoopOffset: Handle: %08X dwOffset: %08X", hHandle, dwOffset);

		OPEN_segaapiBuffer_t* buffer = (OPEN_segaapiBuffer_t*)hHandle;
		buffer->endLoop = dwOffset;
		return OPEN_SEGA_SUCCESS;
	}

	OPEN_SEGASTATUS SEGAAPI_SetStartLoopOffset(void* hHandle, unsigned int dwOffset)
	{
		if (hHandle == NULL)
		{
			dbgprint("SEGAAPI_SetStartLoopOffset: Handle: %08X, Status: OPEN_SEGAERR_BAD_HANDLE", hHandle);
			return OPEN_SEGAERR_BAD_HANDLE;
		}

		dbgprint("SEGAAPI_SetStartLoopOffset: Handle: %08X dwOffset: %08X", hHandle, dwOffset);

		OPEN_segaapiBuffer_t* buffer = (OPEN_segaapiBuffer_t*)hHandle;
		buffer->startLoop = dwOffset;
		return OPEN_SEGA_SUCCESS;
	}

	OPEN_SEGASTATUS SEGAAPI_SetSampleRate(void* hHandle, unsigned int dwSampleRate)
	{
		if (hHandle == NULL)
		{
			dbgprint("SEGAAPI_SetSampleRate: Handle: %08X, Status: OPEN_SEGAERR_BAD_HANDLE", hHandle);
			return OPEN_SEGAERR_BAD_HANDLE;
		}

		dbgprint("SEGAAPI_SetSampleRate: Handle: %08X dwSampleRate: %08X", hHandle, dwSampleRate);

		OPEN_segaapiBuffer_t* buffer = (OPEN_segaapiBuffer_t*)hHandle;
		buffer->sampleRate = dwSampleRate;

		defer_buffer_call(buffer->xaCallback.xaVoice, buffer->xaCallback.defers, dwSampleRate);
		return OPEN_SEGA_SUCCESS;
	}

	OPEN_SEGASTATUS SEGAAPI_SetLoopState(void* hHandle, int bDoContinuousLooping)
	{
		if (hHandle == NULL)
		{
			dbgprint("SEGAAPI_SetLoopState: Handle: %08X, Status: OPEN_SEGAERR_BAD_HANDLE", hHandle);
			return OPEN_SEGAERR_BAD_HANDLE;
		}

		dbgprint("SEGAAPI_SetLoopState: Handle: %08X bDoContinuousLooping: %d", hHandle, bDoContinuousLooping);

		OPEN_segaapiBuffer_t* buffer = (OPEN_segaapiBuffer_t*)hHandle;
		buffer->loop = bDoContinuousLooping;

		return OPEN_SEGA_SUCCESS;
	}

	OPEN_SEGASTATUS SEGAAPI_SetPlaybackPosition(void* hHandle, unsigned int dwPlaybackPos)
	{
		if (hHandle == NULL)
		{
			dbgprint("SEGAAPI_SetPlaybackPosition: Handle: %08X, Status: OPEN_SEGAERR_BAD_HANDLE", hHandle);
			return OPEN_SEGAERR_BAD_HANDLE;
		}

		dbgprint("SEGAAPI_SetPlaybackPosition: Handle: %08X dwPlaybackPos: %08X", hHandle, dwPlaybackPos);

		OPEN_segaapiBuffer_t* buffer = (OPEN_segaapiBuffer_t*)hHandle;

		if (dwPlaybackPos != 0)
		{
			dbgprint("SEGAAPI_SetPlaybackPosition: not supported!");
		}

		// XA2 TODO
		return OPEN_SEGA_SUCCESS;
	}

	unsigned int SEGAAPI_GetPlaybackPosition(void* hHandle)
	{
		if (hHandle == NULL)
		{
			return 0;
		}
		dbgprint("SEGAAPI_GetPlaybackPosition");

		OPEN_segaapiBuffer_t* buffer = (OPEN_segaapiBuffer_t*)hHandle;

		FAudioVoiceState vs;
		FAudioSourceVoice_GetState(buffer->xaCallback.xaVoice, &vs, 0);

		unsigned int result = (vs.SamplesPlayed * (buffer->xaFormat.wBitsPerSample / 8) * buffer->xaFormat.nChannels) % buffer->size;
		
		dbgprint("SEGAAPI_GetPlaybackPosition: Handle: %08X Samples played: %08d BitsPerSample %08d/%08d nChannels %08d bufferSize %08d Result: %08X", 
			hHandle, 
			vs.SamplesPlayed, 
			buffer->xaFormat.wBitsPerSample, 
			(buffer->xaFormat.wBitsPerSample / 8), 
			buffer->xaFormat.nChannels, 
			buffer->size, 
			result);

		return result;
	}

	static void updateRouting(OPEN_segaapiBuffer_t* buffer);

	OPEN_SEGASTATUS SEGAAPI_Play(void* hHandle)
	{
		if (hHandle == NULL)
		{
			dbgprint("SEGAAPI_Play: Handle: %08X, Status: OPEN_SEGAERR_BAD_HANDLE", hHandle);
			return OPEN_SEGAERR_BAD_HANDLE;
		}

		dbgprint("SEGAAPI_Play: Handle: %08X", hHandle);

		OPEN_segaapiBuffer_t* buffer = (OPEN_segaapiBuffer_t*)hHandle;

		updateRouting(buffer);
		updateBufferNew(buffer, -1, -1);

		buffer->playing = true;
		buffer->paused = false;

		// Uncomment to mute music
		//if (buffer->playWithSetup)
		//{
			dbgprint("SEGAAPI_Play: call FAudioSourceVoice_Start voice: %08x size: %d", buffer->xaCallback.xaVoice, buffer->size);
#ifdef _DEBUG
			hexDump("SEGAAPI_Play", buffer->data, 256);
#endif
			CHECK_HR(FAudioSourceVoice_Start(buffer->xaCallback.xaVoice, 0, FAUDIO_COMMIT_NOW));
		//}
		return OPEN_SEGA_SUCCESS;
	}

	OPEN_SEGASTATUS SEGAAPI_Stop(void* hHandle)
	{
		if (hHandle == NULL)
		{
			dbgprint("SEGAAPI_Stop: Handle: %08X, Status: OPEN_SEGAERR_BAD_HANDLE", hHandle);
			return OPEN_SEGAERR_BAD_HANDLE;
		}

		dbgprint("SEGAAPI_Stop: Handle: %08X", hHandle);

		OPEN_segaapiBuffer_t* buffer = (OPEN_segaapiBuffer_t*)hHandle;
		buffer->playing = false;
		buffer->paused = false;
		CHECK_HR(FAudioSourceVoice_Stop(buffer->xaCallback.xaVoice, 0, FAUDIO_COMMIT_NOW));
		return OPEN_SEGA_SUCCESS;
	}

	OPEN_HAWOSTATUS SEGAAPI_GetPlaybackStatus(void* hHandle)
	{
		if (hHandle == NULL)
		{
			dbgprint("SEGAAPI_GetPlaybackStatus: Handle: %08X, Status: OPEN_HAWOSTATUS_INVALID", hHandle);
			return OPEN_HAWOSTATUS_INVALID;
		}
		dbgprint("SEGAAPI_GetPlaybackStatus");

		OPEN_segaapiBuffer_t* buffer = (OPEN_segaapiBuffer_t*)hHandle;

		if (buffer->paused)
		{
			dbgprint("SEGAAPI_GetPlaybackStatus: Handle: %08X, Status: OPEN_HAWOSTATUS_PAUSE, buffer is paused", hHandle);
			return OPEN_HAWOSTATUS_PAUSE;
		}

		// XA2 TODO
		FAudioVoiceState vs;
		FAudioSourceVoice_GetState(buffer->xaCallback.xaVoice, &vs, 0);

		if (vs.BuffersQueued == 0)
		{
			dbgprint("SEGAAPI_GetPlaybackStatus: Handle: %08X, Status: OPEN_HAWOSTATUS_STOP, buffersqueued is 0", hHandle);
			return OPEN_HAWOSTATUS_STOP;
		}

		if (!buffer->loop && vs.SamplesPlayed >= (minfn(buffer->size, buffer->endOffset) / bufferSampleSize(buffer)))
		{
			dbgprint("SEGAAPI_GetPlaybackStatus: Handle: %08X, Status: OPEN_HAWOSTATUS_STOP, Loop false and samples played bigger", hHandle);
			return OPEN_HAWOSTATUS_STOP;
		}

		if (buffer->playing)
		{
			dbgprint("SEGAAPI_GetPlaybackStatus: Handle: %08X, Status: OPEN_HAWOSTATUS_ACTIVE, playing true!", hHandle);
			return OPEN_HAWOSTATUS_ACTIVE;
		}
		else
		{
			dbgprint("SEGAAPI_GetPlaybackStatus: Handle: %08X, Status: OPEN_HAWOSTATUS_STOP, Playing false!", hHandle);
			return OPEN_HAWOSTATUS_STOP;
		}
	}

	OPEN_SEGASTATUS SEGAAPI_SetReleaseState(void* hHandle, int bSet)
	{
		if (hHandle == NULL)
		{
			dbgprint("SEGAAPI_SetReleaseState: Handle: %08X, Status: OPEN_SEGAERR_BAD_HANDLE", hHandle);
			return OPEN_SEGAERR_BAD_HANDLE;
		}

		dbgprint("SEGAAPI_SetReleaseState: Handle: %08X bSet: %08X", hHandle, bSet);

		OPEN_segaapiBuffer_t* buffer = (OPEN_segaapiBuffer_t*)hHandle;

		if (bSet)
		{
			buffer->playing = false;
			FAudioSourceVoice_FlushSourceBuffers(buffer->xaCallback.xaVoice);
			FAudioSourceVoice_Stop(buffer->xaCallback.xaVoice, 0, FAUDIO_COMMIT_NOW);
		}

		return OPEN_SEGA_SUCCESS;
	}

	OPEN_SEGASTATUS SEGAAPI_DestroyBuffer(void* hHandle)
	{
		if (hHandle == NULL)
		{
			dbgprint("SEGAAPI_DestroyBuffer: Handle: %08X, Status: OPEN_SEGAERR_BAD_HANDLE", hHandle);
			return OPEN_SEGAERR_BAD_HANDLE;
		}

		dbgprint("SEGAAPI_DestroyBuffer: Handle: %08X", hHandle);

		OPEN_segaapiBuffer_t* buffer = (OPEN_segaapiBuffer_t*)hHandle;

		FAudioVoice_DestroyVoice(buffer->xaCallback.xaVoice);
		queue_destroy(buffer->xaCallback.defers);
		delete buffer;
		return OPEN_SEGA_SUCCESS;
	}

	int SEGAAPI_SetGlobalEAXProperty(GUID * guid, unsigned long ulProperty, void * pData, unsigned long ulDataSize)
	{
		dbgprint("SEGAAPI_SetGlobalEAXProperty:");

		// Everything is fine
		return 1;
	}

	OPEN_SEGASTATUS SEGAAPI_Init(void)
	{
		dbgprint("SEGAAPI_Init");

		CHECK_HR(FAudioCreate(&g_xa2, 0, FAUDIO_DEFAULT_PROCESSOR));
		/*
		//XAUDIO2_DEBUG_CONFIGURATION cfg = { 0 };
		FAudioDebugConfiguration cfg = { 0 };
		//cfg.TraceMask = XAUDIO2_LOG_ERRORS;
		cfg.TraceMask = FAUDIO_LOG_ERRORS;
		//cfg.BreakMask = XAUDIO2_LOG_ERRORS;
		//g_xa2->SetDebugConfiguration(&cfg);
		FAudio_SetDebugConfiguration(g_xa2, &cfg, NULL);
		*/

		CHECK_HR(FAudio_CreateMasteringVoice(g_xa2, &g_masteringVoice, FAUDIO_DEFAULT_CHANNELS, FAUDIO_DEFAULT_SAMPLERATE, 0, 0, NULL));

		FAudioVoiceDetails vd;
		FAudioVoice_GetVoiceDetails(g_masteringVoice, &vd);

		for (auto& g_submixVoice : g_submixVoices)
		{
			CHECK_HR(FAudio_CreateSubmixVoice(g_xa2, &g_submixVoice, 1, vd.InputSampleRate, 0, 0, NULL, NULL));
		}

		int numChannels = vd.InputChannels;

		auto setSubmixVoice = [=](OPEN_HAROUTING index, float frontLeft, float frontRight, float frontCenter, float lfe,
			float rearLeft, float rearRight)
		{
			float levelMatrix[12] = { 0 };
			levelMatrix[0] = frontLeft + rearLeft;
			levelMatrix[1] = frontRight + rearRight;

			if (numChannels == 2)
			{
				// TODO
			}

			levelMatrix[2] = lfe;

			// TODO: surround data - SetOutputMatrix order is somewhat unclear :/
			FAudioVoice_SetOutputMatrix(g_submixVoices[index], g_masteringVoice, 1, numChannels, levelMatrix, FAUDIO_COMMIT_NOW);
		};

		setSubmixVoice(OPEN_HA_FRONT_LEFT_PORT, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
		setSubmixVoice(OPEN_HA_FRONT_RIGHT_PORT, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
		setSubmixVoice(OPEN_HA_FRONT_CENTER_PORT, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
		setSubmixVoice(OPEN_HA_REAR_LEFT_PORT, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
		setSubmixVoice(OPEN_HA_REAR_RIGHT_PORT, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
		setSubmixVoice(OPEN_HA_LFE_PORT, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);

		return OPEN_SEGA_SUCCESS;
	}

	OPEN_SEGASTATUS SEGAAPI_Exit(void)
	{
		dbgprint("SEGAAPI_Exit");

		for (auto& g_submixVoice : g_submixVoices)
		{
			FAudioVoice_DestroyVoice(g_submixVoice);
		}

		// TODO: deinit XA2
		return OPEN_SEGA_SUCCESS;
	}

	OPEN_SEGASTATUS SEGAAPI_Reset(void)
	{
		dbgprint("SEGAAPI_Reset");
		return OPEN_SEGA_SUCCESS;
	}

	OPEN_SEGASTATUS SEGAAPI_SetIOVolume(OPEN_HAPHYSICALIO dwPhysIO, unsigned int dwVolume)
	{
		dbgprint("SEGAAPI_SetIOVolume: dwPhysIO: %08X dwVolume: %08X", dwPhysIO, dwVolume);
		FAudioVoice_SetVolume(g_masteringVoice, dwVolume / (float)0xFFFFFFFF, FAUDIO_COMMIT_NOW);
		return OPEN_SEGA_SUCCESS;
	}

	static void updateRouting(OPEN_segaapiBuffer_t* buffer)
	{
		float levels[7 * 2];
		FAudioSubmixVoice* outVoices[7];

		int numRoutes = 0;

		dbgprint("updateRouting");
		for (int i = 0; i < /*7*/2; i++)
		{
			if (buffer->sendRoutes[i] != OPEN_HA_UNUSED_PORT && buffer->sendRoutes[i] < 6)
			{
				outVoices[numRoutes] = g_submixVoices[buffer->sendRoutes[i]];

				int levelOff = numRoutes * buffer->channels;

				for (int ch = 0; ch < buffer->channels; ch++)
				{
					levels[levelOff + ch] = 0;
				}

				float level = buffer->sendVolumes[i] * buffer->channelVolumes[buffer->sendChannels[i]];
				levels[levelOff + buffer->sendChannels[i]] = level;

				++numRoutes;
			}
		}

		// can't set no routes
		if (numRoutes == 0)
		{
			return;
		}

		FAudioSendDescriptor sendDescs[7];
		for (int i = 0; i < numRoutes; i++)
		{
			sendDescs[i].Flags = 0;
			sendDescs[i].pOutputVoice = outVoices[i];
		}

		FAudioVoiceSends sends;
		sends.SendCount = numRoutes;
		sends.pSends = sendDescs;
		CHECK_HR(FAudioVoice_SetOutputVoices(buffer->xaCallback.xaVoice, &sends));

		for (int i = 0; i < numRoutes; i++)
		{
			CHECK_HR(FAudioVoice_SetOutputMatrix(buffer->xaCallback.xaVoice, outVoices[i], buffer->channels, 1, &levels[i * buffer->channels], FAUDIO_COMMIT_NOW));
		}
	}

	OPEN_SEGASTATUS SEGAAPI_SetSendRouting(void* hHandle, unsigned int dwChannel, unsigned int dwSend, OPEN_HAROUTING dwDest)
	{
		if (hHandle == NULL)
		{
			dbgprint("SEGAAPI_SetSendRouting: Handle: %08X, Status: OPEN_SEGAERR_BAD_HANDLE", hHandle);
			return OPEN_SEGAERR_BAD_HANDLE;
		}

		dbgprint("SEGAAPI_SetSendRouting: hHandle: %08X dwChannel: %08X dwSend: %08X dwDest: %08X", hHandle, dwChannel, dwSend, dwDest);

		OPEN_segaapiBuffer_t* buffer = (OPEN_segaapiBuffer_t*)hHandle;
		buffer->sendRoutes[dwSend] = dwDest;
		buffer->sendChannels[dwSend] = dwChannel;

		updateRouting(buffer);
		return OPEN_SEGA_SUCCESS;
	}

	OPEN_SEGASTATUS SEGAAPI_SetSendLevel(void* hHandle, unsigned int dwChannel, unsigned int dwSend, unsigned int dwLevel)
	{
		if (hHandle == NULL)
		{
			dbgprint("SEGAAPI_SetSendLevel: Handle: %08X, Status: OPEN_SEGAERR_BAD_HANDLE", hHandle);
			return OPEN_SEGAERR_BAD_HANDLE;
		}

		dbgprint("SEGAAPI_SetSendLevel: hHandle: %08X dwChannel: %08X dwSend: %08X dwLevel: %08X", hHandle, dwChannel, dwSend, dwLevel);

		OPEN_segaapiBuffer_t* buffer = (OPEN_segaapiBuffer_t*)hHandle;
		buffer->sendVolumes[dwSend] = dwLevel / (float)0xFFFFFFFF;
		buffer->sendChannels[dwSend] = dwChannel;

		updateRouting(buffer);
		return OPEN_SEGA_SUCCESS;
	}

	OPEN_SEGASTATUS SEGAAPI_SetSynthParam(void* hHandle, OPEN_HASYNTHPARAMSEXT param, int lPARWValue)
	{
		if (hHandle == NULL)
		{
			dbgprint("SEGAAPI_SetSynthParam: Handle: %08X, Status: OPEN_SEGAERR_BAD_HANDLE", hHandle);
			return OPEN_SEGAERR_BAD_HANDLE;
		}

		dbgprint("SEGAAPI_SetSynthParam: hHandle: %08X OPEN_HASYNTHPARAMSEXT: %08X lPARWValue: %08X", hHandle, param, lPARWValue);

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
			FineTune, ///< 1,         0x01,  fineTune + coarseTune * 100
			InitialFilterFc, ///< 2,         0x02,  initialFilterFc
			InitialFilterQ, ///< 3,         0x03,  initialFilterQ
			DelayVolEnv, ///< 4,         0x04,  delayVolEnv
			AttackVolEnv, ///< 5,         0x05,  attackVolEnv
			HoldVolEnv, ///< 6,         0x06,  holdVolEnv
			DecayVolEnv, ///< 7,         0x07,  decayVolEnv
			SustainVolEnv, ///< 8,         0x08,  sustainVolEnv
			ReleaseVolEnv, ///< 9,         0x09,  releaseVolEnv
			DelayModEnv, ///< 10,        0x0A,  delayModEnv
			AttackModEnv, ///< 11,        0x0B,  attackModEnv
			HoldModEnv, ///< 12,        0x0C,  holdModEnv
			DecayModEnv, ///< 13,        0x0D,  decayModEnv
			SustainModEnv, ///< 14,        0x0E,  sustainModEnv
			ReleaseModEnv, ///< 15,        0x0F,  releaseModEnv
			DelayModLFO, ///< 16,        0x10,  delayModLFO
			FreqModLFO, ///< 17,        0x11,  freqModLFO
			DelayVibLFO, ///< 18,        0x12,  delayVibLFO
			FreqVibLFO, ///< 19,        0x13,  freqVibLFO
			ModLfoToPitch, ///< 20,        0x14,  modLfoToPitch
			VibLfoToPitch, ///< 21,        0x15,  vibLfoToPitch
			ModLfoToFilterFc, ///< 22,        0x16,  modLfoToFilterFc
			ModLfoToVolume, ///< 23,        0x17,  modLfoToVolume
			ModEnvToPitch, ///< 24,        0x18,  modEnvToPitch
			ModEnvToFilterFc ///< 25,        0x19,  modEnvToFilterFc
		};

		int realParam = mapping[param];

		OPEN_segaapiBuffer_t* buffer = (OPEN_segaapiBuffer_t*)hHandle;
		//tsf_hydra_genamount amount;
		//amount.shortAmount = lPARWValue;
		//tsf_region_operator(buffer->region, realParam, &amount);

		if (param == OPEN_HAVP_ATTENUATION)
		{
			float volume = tsf_decibelsToGain(0.0f - lPARWValue / 10.0f);

			FAudioVoice_SetVolume(buffer->xaCallback.xaVoice, volume, FAUDIO_COMMIT_NOW);
			dbgprint("SEGAAPI_SetSynthParam: OPEN_HAVP_ATTENUATION gain: %f dB: %d", volume, lPARWValue);
		}
		else if (param == OPEN_HAVP_PITCH)
		{
			float semiTones = lPARWValue / 100.0f;
			float freqRatio = FAudioSemitonesToFrequencyRatio(semiTones);

			FAudioSourceVoice_SetFrequencyRatio(buffer->xaCallback.xaVoice, freqRatio, FAUDIO_COMMIT_NOW);
			dbgprint("SEGAAPI_SetSynthParam: OPEN_HAVP_PITCH hHandle: %08X semitones: %f freqRatio: %f", hHandle, semiTones, freqRatio);
		}

		return OPEN_SEGA_SUCCESS;
	}

	int SEGAAPI_GetSynthParam(void * hHandle, OPEN_HASYNTHPARAMSEXT param)
	{
		if (hHandle == NULL)
		{
			dbgprint("SEGAAPI_GetSynthParam: Handle: %08X, Status: OPEN_SEGAERR_BAD_HANDLE", hHandle);
			return OPEN_SEGAERR_BAD_HANDLE;
		}

		dbgprint("SEGAAPI_GetSynthParam: hHandle: %08X OPEN_HASYNTHPARAMSEXT: %08X", hHandle, param);

		return 0; //todo not sure if actually used
	}

	OPEN_SEGASTATUS SEGAAPI_SetSynthParamMultiple(void* hHandle, unsigned int dwNumParams, OPEN_SynthParamSet* pSynthParams)
	{
		if (hHandle == NULL)
		{
			dbgprint("SEGAAPI_SetSynthParamMultiple: Handle: %08X, Status: OPEN_SEGAERR_BAD_HANDLE", hHandle);
			return OPEN_SEGAERR_BAD_HANDLE;
		}

		dbgprint("SEGAAPI_SetSynthParamMultiple: hHandle: %08X dwNumParams: %08X pSynthParams: %08X", hHandle, dwNumParams, pSynthParams);

		for (int i = 0; i < dwNumParams; i++)
		{
			SEGAAPI_SetSynthParam(hHandle, pSynthParams[i].param, pSynthParams[i].lPARWValue);
		}

		return OPEN_SEGA_SUCCESS;
	}

	OPEN_SEGASTATUS SEGAAPI_SetChannelVolume(void* hHandle, unsigned int dwChannel, unsigned int dwVolume)
	{
		if (hHandle == NULL)
		{
			dbgprint("SEGAAPI_SetChannelVolume: Handle: %08X, Status: OPEN_SEGAERR_BAD_HANDLE", hHandle);
			return OPEN_SEGAERR_BAD_HANDLE;
		}

		dbgprint("SEGAAPI_SetChannelVolume: hHandle: %08X dwChannel: %08X dwVolume: %08X", hHandle, dwChannel, dwVolume);

		OPEN_segaapiBuffer_t* buffer = (OPEN_segaapiBuffer_t*)hHandle;
		buffer->channelVolumes[dwChannel] = dwVolume / (float)0xFFFFFFFF;
		return OPEN_SEGA_SUCCESS;
	}

	unsigned int SEGAAPI_GetChannelVolume(void* hHandle, unsigned int dwChannel)
	{
		if (hHandle == NULL)
		{
			dbgprint("SEGAAPI_GetChannelVolume: Handle: %08X, Status: OPEN_SEGAERR_BAD_HANDLE", hHandle);
			return OPEN_SEGAERR_BAD_HANDLE;
		}

		dbgprint("SEGAAPI_GetChannelVolume: hHandle: %08X dwChannel: %08X", hHandle, dwChannel);

		OPEN_segaapiBuffer_t* buffer = (OPEN_segaapiBuffer_t*)hHandle;
		return buffer->channelVolumes[dwChannel];
	}

	OPEN_SEGASTATUS SEGAAPI_Pause(void* hHandle)
	{
		if (hHandle == NULL)
		{
			dbgprint("SEGAAPI_Pause: Handle: %08X, Status: OPEN_SEGAERR_BAD_HANDLE", hHandle);
			return OPEN_SEGAERR_BAD_HANDLE;
		}

		dbgprint("SEGAAPI_Pause: hHandle: %08X", hHandle);

		OPEN_segaapiBuffer_t* buffer = (OPEN_segaapiBuffer_t*)hHandle;

		buffer->playing = false;
		buffer->paused = true;
		CHECK_HR(FAudioSourceVoice_Stop(buffer->xaCallback.xaVoice, 0, FAUDIO_COMMIT_NOW));
		
		return OPEN_SEGA_SUCCESS;
	}

	OPEN_SEGASTATUS SEGAAPI_PlayWithSetup(
		void* hHandle,
		unsigned int dwNumSendRouteParams, OPEN_SendRouteParamSet* pSendRouteParams,
		unsigned int dwNumSendLevelParams, OPEN_SendLevelParamSet* pSendLevelParams,
		unsigned int dwNumVoiceParams, OPEN_VoiceParamSet* pVoiceParams,
		unsigned int dwNumSynthParams, OPEN_SynthParamSet* pSynthParams
	)
	{
		if (hHandle == NULL)
		{
			dbgprint("SEGAAPI_PlayWithSetup: Handle: %08X, Status: OPEN_SEGAERR_BAD_HANDLE", hHandle);
			return OPEN_SEGAERR_BAD_HANDLE;
		}

		dbgprint("SEGAAPI_PlayWithSetup: hHandle: %08X dwNumSendRouteParams: %d pSendRouteParams: %08X dwNumSendLevelParams: %d pSendLevelParams: %08X dwNumVoiceParams: %d pVoiceParams: %08X dwNumSynthParams: %d pSynthParams: %08X", hHandle, dwNumSendRouteParams, *pSendRouteParams, dwNumSendLevelParams, *pSendLevelParams, dwNumVoiceParams, *pVoiceParams, dwNumSynthParams, *pSynthParams);
		dbgprint("dwNumSynthParams: %d", dwNumSynthParams);

		OPEN_segaapiBuffer_t* buffer = (OPEN_segaapiBuffer_t*)hHandle;
		buffer->playWithSetup = true;

		for (int i = 0; i < dwNumSendRouteParams; i++)
		{
			SEGAAPI_SetSendRouting(hHandle, pSendRouteParams[i].dwChannel, pSendRouteParams[i].dwSend, pSendRouteParams[i].dwDest);
		}

		for (int i = 0; i < dwNumSendLevelParams; i++)
		{
			SEGAAPI_SetSendLevel(hHandle, pSendLevelParams[i].dwChannel, pSendLevelParams[i].dwSend, pSendLevelParams[i].dwLevel);
		}

		unsigned int loopStart = 0;
		unsigned int loopEnd = 0;
		unsigned int loopState = 0;
		unsigned int endOffset = 0;

		for (int i = 0; i < dwNumVoiceParams; i++)
		{
			switch (pVoiceParams[i].VoiceIoctl)
			{
			case OPEN_VOICEIOCTL_SET_START_LOOP_OFFSET:
				SEGAAPI_SetStartLoopOffset(hHandle, pVoiceParams[i].dwParam1);
				loopStart = pVoiceParams[i].dwParam1;
				break;
			case OPEN_VOICEIOCTL_SET_END_LOOP_OFFSET:
				SEGAAPI_SetEndLoopOffset(hHandle, pVoiceParams[i].dwParam1);
				loopEnd = pVoiceParams[i].dwParam1;
				break;
			case OPEN_VOICEIOCTL_SET_END_OFFSET:
				SEGAAPI_SetEndOffset(hHandle, pVoiceParams[i].dwParam1);
				endOffset = pVoiceParams[i].dwParam1;
				break;
			case OPEN_VOICEIOCTL_SET_LOOP_STATE:
				SEGAAPI_SetLoopState(hHandle, pVoiceParams[i].dwParam1);
				loopState = pVoiceParams[i].dwParam1;
				break;
			case OPEN_VOICEIOCTL_SET_NOTIFICATION_POINT:
				dbgprint("Unimplemented! OPEN_VOICEIOCTL_SET_NOTIFICATION_POINT");
				break;
			case OPEN_VOICEIOCTL_CLEAR_NOTIFICATION_POINT:
				dbgprint("Unimplemented! OPEN_VOICEIOCTL_CLEAR_NOTIFICATION_POINT");
				break;
			case OPEN_VOICEIOCTL_SET_NOTIFICATION_FREQUENCY:
				dbgprint("Unimplemented! OPEN_VOICEIOCTL_SET_NOTIFICATION_FREQUENCY");
				break;
			}
		}

		dbgprint("Loopdata: hHandle: %08X, loopStart: %08X, loopEnd: %08X, endOffset: %08X, loopState: %d, size: %d", hHandle, loopStart, loopEnd, endOffset, loopState, buffer->size);

		for (int i = 0; i < dwNumSynthParams; i++)
		{
			SEGAAPI_SetSynthParam(hHandle, pSynthParams[i].param, pSynthParams[i].lPARWValue);
		}

		SEGAAPI_Play(hHandle);

		return OPEN_SEGA_SUCCESS;
	}

	OPEN_SEGASTATUS SEGAAPI_GetLastStatus(void)
	{
		dbgprint("SEGAAPI_GetLastStatus");
		return OPEN_SEGA_SUCCESS;
	}
}
#pragma optimize("", on)
