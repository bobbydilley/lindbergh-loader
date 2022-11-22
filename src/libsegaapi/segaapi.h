#ifndef __SEGAAPI_H
#define __SEGAAPI_H

#ifndef GUID_DEFINED
#define GUID_DEFINED
typedef struct _GUID
{
    unsigned long Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char Data4[8];
} GUID;
#endif // GUID_DEFINED

#ifndef DEFINE_GUID
    #define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) extern const GUID name
#endif // DEFINE_GUID

#define SEGARESULT_FAILURE(_x) ((1 << 31) | 0xA000 | (_x))
#define SEGA_SUCCESS 0L
#define SEGAERR_FAIL SEGARESULT_FAILURE(0)
#define SEGAERR_BAD_POINTER SEGARESULT_FAILURE(3)
#define SEGAERR_UNSUPPORTED SEGARESULT_FAILURE(5)
#define SEGAERR_BAD_PARAM SEGARESULT_FAILURE(9)
#define SEGAERR_INVALID_CHANNEL SEGARESULT_FAILURE(10)
#define SEGAERR_INVALID_SEND SEGARESULT_FAILURE(11)
#define SEGAERR_PLAYING SEGARESULT_FAILURE(12)
#define SEGAERR_NO_RESOURCES SEGARESULT_FAILURE(13)
#define SEGAERR_BAD_CONFIG SEGARESULT_FAILURE(14)
#define SEGAERR_BAD_HANDLE SEGARESULT_FAILURE(18)
#define SEGAERR_BAD_SAMPLERATE SEGARESULT_FAILURE(28)
#define SEGAERR_OUT_OF_MEMORY SEGARESULT_FAILURE(31)
#define SEGAERR_INIT_FAILED SEGARESULT_FAILURE(39)

// {A7FEEC3F-2BFD-4a40-891F-7423E38BAC1F}
DEFINE_GUID(EAXPROPERTYID_EAX40_SEGA_Custom, 0xa7feec3f, 0x2bfd, 0x4a40, 0x89, 0x1f, 0x74, 0x23, 0xe3, 0x8b, 0xac, 0x1f);

typedef enum
{
    EAXSEGA_STEREO_RETURN_FX2 = 0,
    EAXSEGA_STEREO_RETURN_FX3 = 1
} EAXSEGA_PROPERTY;

/**
 * The following defines all of the messages wave output clients
 * can receive as part of their callback routines.
 */
typedef enum
{
    HAWOS_RESOURCE_STOLEN = 0,
    HAWOS_NOTIFY = 2
} HAWOSMESSAGETYPE;

/*
 * The Playback status.
 */
typedef enum
{
    PLAYBACK_STATUS_STOP,        /* The voice is stopped */
    PLAYBACK_STATUS_ACTIVE,      /* The voice is playing */
    PLAYBACK_STATUS_PAUSE,       /* The voice is paused */
    PLAYBACK_STATUS_INVALID = -1 /* Invalid state */
} PlaybackStatus;

/*
 * dwFlags use in CreateBuffer.
 */
#define HABUF_SYNTH_BUFFER 0x00000001   // indiate to create a synth buffer
#define HABUF_ALLOC_USER_MEM 0x00000002 // indiate that caller allocate memory
#define HABUF_USE_MAPPED_MEM 0x00000003 // indiate that caller allocate memory

/*
 * The HAWOSEFORMAT structure is used to change the format of an output client.
 */

#ifndef __HAWAVE_H
#define HASF_UNSIGNED_8PCM 0x0004 /* Unsigned (offset 128) 8-bit PCM */
#define HASF_SIGNED_16PCM 0x0020  /* Signed 16-bit PCM */
#endif

typedef struct
{
    unsigned int dwSampleRate;   /* The sample rate the client desires (in Hz) */
    unsigned int dwSampleFormat; /* The sample format the client will use */
    unsigned int byNumChans;     /* The number of samples in the sample
                                  *  frame (1 = mono, 2 = stereo).  */
} HAWOSEFORMAT;

/*
 * HAWOSEMAPDATA contains
 */
typedef struct
{
    unsigned int dwSize;   /* Supply by caller. Size (in bytes) of the valid sample data */
    unsigned int dwOffset; /* Return by driver. Offset of buffer where the the first valid sample should be written to */
    void *hBufferHdr;      /* Return by driver. Memory address that user-space application can access. */
} HAWOSEMAPDATA;

/*
 * The HAWOSEBUFFERCONFIG structure is used to describe how an input or
 * output buffer client wishes to configure the device when it opens it.
 */
typedef struct
{
    unsigned int dwPriority;     /* The priority with which the voices
                                  *  should be allocated.  This is used
                                  *  when voices need to be ripped off. */
    unsigned int dwSampleRate;   /* The sample rate the voice desires */
    unsigned int dwSampleFormat; /* The sample format the voice will use */
    unsigned int byNumChans;     /* The number of samples in the sample
                                  *  frame. (1 = mono, 2 = stereo). */
    unsigned int dwReserved;     /* Reserved field */
    void *hUserData;             /* User data */
    HAWOSEMAPDATA mapData;       /* The sample memory mapping for the buffer. */
} HAWOSEBUFFERCONFIG;

/**
 * Default values
 */
#define HAWOSEVOL_MAX 0xFFFFFFFF /* Maximum volume; no attenuation */

/**
 * Since Tina has up to 64- voices, voice priorities typically ranging
 * from 0 to 63, where 0 is lower priority (more likely to get ripped off)
 * than 63.
 *
 * Set voice priority to HAWOSEP_MAXIMUM if a voice must never get ripped
 * off under any circumstances.
 */
#define HAWOSEP_MINIMUM 0
#define HAWOSEP_MAXIMUM 0xFFFFFFFF

/** @brief Routing List
 *
 * voice sends routing to speakers or effects ports.
 *
 */
#define HAWOSE_UNUSED_SEND 0xFFFF0001

typedef enum HAROUTING
{
    HA_UNUSED_PORT = HAWOSE_UNUSED_SEND,

    // Dry multi-channel outputs
    HA_FRONT_LEFT_PORT = 0,
    HA_FRONT_RIGHT_PORT = 1,
    HA_FRONT_CENTER_PORT = 2,
    HA_LFE_PORT = 3,
    HA_REAR_LEFT_PORT = 4,
    HA_REAR_RIGHT_PORT = 5,

    // effect outputs
    HA_FXSLOT0_PORT = 10,
    HA_FXSLOT1_PORT = 11,
    HA_FXSLOT2_PORT = 12,
    HA_FXSLOT3_PORT = 13

} HAROUTING;

/**
 * The following defines SPDIF-Out sampling rate.
 */
typedef enum
{
    HASPDIFOUT_44_1KHZ = 0,
    HASPDIFOUT_48KHZ,
    HASPDIFOUT_96KHZ
} HASPDIFOUTRATE;

/**
 * The following defines inputs and outputs of SEGA sound board.
 */
typedef enum HAPHYSICALIO
{
    // analog outputs
    HA_OUT_FRONT_LEFT = 0,
    HA_OUT_FRONT_RIGHT = 1,
    HA_OUT_FRONT_CENTER = 2,
    HA_OUT_LFE_PORT = 3,
    HA_OUT_REAR_LEFT = 4,
    HA_OUT_REAR_RIGHT = 5,

    // optical Outputs
    HA_OUT_OPTICAL_LEFT = 10,
    HA_OUT_OPTICAL_RIGHT = 11,

    // Line In
    HA_IN_LINEIN_LEFT = 20,
    HA_IN_LINEIN_RIGHT = 21

} HAPHYSICALIO;

/** @brief Synth parameters enumeration list
 *
 * This table defines the most common (and hardware-supported)
 * control routings in Real World Unit.
 *
 * Refers to DLS spec or SoundFont spec for details of these Parameters,
 * their units and their ranges.
 */
typedef enum HASYNTHPARAMSEXT
{
    HAVP_ATTENUATION,              ///< 0,         0x00,  initialAttenuation
    HAVP_PITCH,                    ///< 1,         0x01,  fineTune + coarseTune * 100
    HAVP_FILTER_CUTOFF,            ///< 2,         0x02,  initialFilterFc
    HAVP_FILTER_Q,                 ///< 3,         0x03,  initialFilterQ
    HAVP_DELAY_VOL_ENV,            ///< 4,         0x04,  delayVolEnv
    HAVP_ATTACK_VOL_ENV,           ///< 5,         0x05,  attackVolEnv
    HAVP_HOLD_VOL_ENV,             ///< 6,         0x06,  holdVolEnv
    HAVP_DECAY_VOL_ENV,            ///< 7,         0x07,  decayVolEnv
    HAVP_SUSTAIN_VOL_ENV,          ///< 8,         0x08,  sustainVolEnv
    HAVP_RELEASE_VOL_ENV,          ///< 9,         0x09,  releaseVolEnv
    HAVP_DELAY_MOD_ENV,            ///< 10,        0x0A,  delayModEnv
    HAVP_ATTACK_MOD_ENV,           ///< 11,        0x0B,  attackModEnv
    HAVP_HOLD_MOD_ENV,             ///< 12,        0x0C,  holdModEnv
    HAVP_DECAY_MOD_ENV,            ///< 13,        0x0D,  decayModEnv
    HAVP_SUSTAIN_MOD_ENV,          ///< 14,        0x0E,  sustainModEnv
    HAVP_RELEASE_MOD_ENV,          ///< 15,        0x0F,  releaseModEnv
    HAVP_DELAY_MOD_LFO,            ///< 16,        0x10,  delayModLFO
    HAVP_FREQ_MOD_LFO,             ///< 17,        0x11,  freqModLFO
    HAVP_DELAY_VIB_LFO,            ///< 18,        0x12,  delayVibLFO
    HAVP_FREQ_VIB_LFO,             ///< 19,        0x13,  freqVibLFO
    HAVP_MOD_LFO_TO_PITCH,         ///< 20,        0x14,  modLfoToPitch
    HAVP_VIB_LFO_TO_PITCH,         ///< 21,        0x15,  vibLfoToPitch
    HAVP_MOD_LFO_TO_FILTER_CUTOFF, ///< 22,        0x16,  modLfoToFilterFc
    HAVP_MOD_LFO_TO_ATTENUATION,   ///< 23,        0x17,  modLfoToVolume
    HAVP_MOD_ENV_TO_PITCH,         ///< 24,        0x18,  modEnvToPitch
    HAVP_MOD_ENV_TO_FILTER_CUTOFF  ///< 25,        0x19,  modEnvToFilterFc

} HASYNTHPARAMSEXT;

#ifndef __SYNTHPARAMSET_
#define __SYNTHPARAMSET_
typedef struct SynthParamSetExt
{
    HASYNTHPARAMSEXT param;
    int lPARWValue;
} SynthParamSet;
#endif

/*
How this SYNTH PARAMS EXT maps to Sega API requests:

    HAVP_ATTENUATION,                           SetVolume()
    HAVP_PITCH,                                 SetPitch()
    HAVP_FILTER_CUTOFF,                         SetFilter()
    HAVP_FILTER_Q,                              SetFilter()
    HAVP_DELAY_VOL_ENV,                         SetEG()
    HAVP_ATTACK_VOL_ENV,                        SetEG()
    HAVP_HOLD_VOL_ENV,                          SetEG()
    HAVP_DECAY_VOL_ENV,                         SetEG()
    HAVP_SUSTAIN_VOL_ENV,                       SetEG()
    HAVP_RELEASE_VOL_ENV,                       SetEG()
    HAVP_DELAY_MOD_ENV,                         SetEG()
    HAVP_ATTACK_MOD_ENV,                        SetEG()
    HAVP_HOLD_MOD_ENV,                          SetEG()
    HAVP_DECAY_MOD_ENV,                         SetEG()
    HAVP_SUSTAIN_MOD_ENV,                       SetEG()
    HAVP_RELEASE_MOD_ENV,                       SetEG()
    HAVP_DELAY_MOD_LFO,                         SetLFO()
    HAVP_FREQ_MOD_LFO,                          SetLFO()
    HAVP_DELAY_VIB_LFO,                         SetLFO()
    HAVP_FREQ_VIB_LFO,                          SetLFO()
    HAVP_MOD_LFO_TO_PITCH,                      SetLFO()
    HAVP_VIB_LFO_TO_PITCH,                      SetLFO()
    HAVP_MOD_LFO_TO_FILTER_CUTOFF,              SetLFO()
    HAVP_MOD_LFO_TO_ATTENUATION,                SetLFO()
    HAVP_MOD_ENV_TO_PITCH,                      SetEG()
    HAVP_MOD_ENV_TO_FILTER_CUTOFF,              SetEG()

*/

int SEGAAPI_Play(void *hHandle);
int SEGAAPI_Pause(void *hHandle);
int SEGAAPI_Stop(void *hHandle);
int SEGAAPI_PlayWithSetup(void *hHandle);
PlaybackStatus SEGAAPI_GetPlaybackStatus(void *hHandle);
int SEGAAPI_SetFormat(void *hHandle, HAWOSEFORMAT *pFormat);
int SEGAAPI_GetFormat(void *hHandle, HAWOSEFORMAT *pFormat);
int SEGAAPI_SetSampleRate(void *hHandle, unsigned int dwSampleRate);
unsigned int SEGAAPI_GetSampleRate(void *hHandle);
int SEGAAPI_SetPriority(void *hHandle, unsigned int dwPriority);
unsigned int SEGAAPI_GetPriority(void *hHandle);
int SEGAAPI_SetUserData(void *hHandle, void *hUserData);
void *SEGAAPI_GetUserData(void *hHandle);
int SEGAAPI_SetSendRouting(void *hHandle, unsigned int dwChannel, unsigned int dwSend, HAROUTING dwDest);
HAROUTING SEGAAPI_GetSendRouting(void *hHandle, unsigned int dwChannel, unsigned int dwSend);
int SEGAAPI_SetSendLevel(void *hHandle, unsigned int dwChannel, unsigned int dwSend, unsigned int dwLevel);
unsigned int SEGAAPI_GetSendLevel(void *hHandle, unsigned int dwChannel, unsigned int dwSend);
int SEGAAPI_SetChannelVolume(void *hHandle, unsigned int dwChannel, unsigned int dwVolume);
unsigned int SEGAAPI_GetChannelVolume(void *hHandle, unsigned int dwChannel);
int SEGAAPI_SetPlaybackPosition(void *hHandle, unsigned int dwPlaybackPos);
unsigned int SEGAAPI_GetPlaybackPosition(void *hHandle);
int SEGAAPI_SetNotificationFrequency(void *hHandle, unsigned int dwFrameCount);
int SEGAAPI_SetNotificationPoint(void *hHandle, unsigned int dwBufferOffset);
int SEGAAPI_ClearNotificationPoint(void *hHandle, unsigned int dwBufferOffset);
int SEGAAPI_SetStartLoopOffset(void *hHandle, unsigned int dwOffset);
unsigned int SEGAAPI_GetStartLoopOffset(void *hHandle);
int SEGAAPI_SetEndLoopOffset(void *hHandle, unsigned int dwOffset);
unsigned int SEGAAPI_GetEndLoopOffset(void *hHandle);
int SEGAAPI_SetEndOffset(void *hHandle, unsigned int dwOffset);
unsigned int SEGAAPI_GetEndOffset(void *hHandle);
int SEGAAPI_SetLoopState(void *hHandle, int bDoContinuousLooping);
int SEGAAPI_GetLoopState(void *hHandle);
int SEGAAPI_UpdateBuffer(void *hHandle, unsigned int dwStartOffset, unsigned int dwLength);
int SEGAAPI_SetSynthParam(void *hHandle, HASYNTHPARAMSEXT param, int lPARWValue);
int SEGAAPI_GetSynthParam(void *hHandle, HASYNTHPARAMSEXT param);
int SEGAAPI_SetSynthParamMultiple(void *hHandle, unsigned int dwNumParams, SynthParamSet *pSynthParams);
int SEGAAPI_GetSynthParamMultiple(void *hHandle, unsigned int dwNumParams, SynthParamSet *pSynthParams);
int SEGAAPI_SetReleaseState(void *hHandle, int bSet);
typedef void (*HAWOSEGABUFFERCALLBACK)(void *hHandle, HAWOSMESSAGETYPE message);
int SEGAAPI_CreateBuffer(HAWOSEBUFFERCONFIG *pConfig, HAWOSEGABUFFERCALLBACK pCallback, unsigned int dwFlags, void **phHandle);
int SEGAAPI_DestroyBuffer(void *hHandle);
int SEGAAPI_SetGlobalEAXProperty(GUID *guid, unsigned long ulProperty, void *pData, unsigned long ulDataSize);
int SEGAAPI_GetGlobalEAXProperty(GUID *guid, unsigned long ulProperty, void *pData, unsigned long ulDataSize);
int SEGAAPI_SetSPDIFOutChannelStatus(unsigned int dwChannelStatus, unsigned int dwExtChannelStatus);
int SEGAAPI_GetSPDIFOutChannelStatus(unsigned int *pdwChannelStatus, unsigned int *pdwExtChannelStatus);
int SEGAAPI_SetSPDIFOutSampleRate(HASPDIFOUTRATE dwSamplingRate);
HASPDIFOUTRATE SEGAAPI_GetSPDIFOutSampleRate(void);
int SEGAAPI_SetSPDIFOutChannelRouting( unsigned int dwChannel, HAROUTING dwSource);
HAROUTING SEGAAPI_GetSPDIFOutChannelRouting(unsigned int dwChannel);
int SEGAAPI_SetIOVolume(HAPHYSICALIO dwPhysIO, unsigned int dwVolume);
unsigned int SEGAAPI_GetIOVolume(HAPHYSICALIO dwPhysIO);
void SEGAAPI_SetLastStatus(int LastStatus);
int SEGAAPI_GetLastStatus(void);
int SEGAAPI_Reset(void);
int SEGAAPI_Init(void);
int SEGAAPI_Exit(void);

#endif /* __SEGAAPI_H */
