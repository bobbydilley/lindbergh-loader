#ifndef __SEGAAPI_H
#define __SEGAAPI_H

/* GUID Definitions */
typedef struct _GUID
{
    unsigned long Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char Data4[8];
} GUID;

#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) extern const GUID name

DEFINE_GUID(EAXPROPERTYID_EAX40_SEGA_Custom, 0xa7feec3f, 0x2bfd, 0x4a40, 0x89, 0x1f, 0x74, 0x23, 0xe3, 0x8b, 0xac, 0x1f);

/* Return values */
#define SEGARESULT_FAILURE(_x) ((1 << 31) | 0xA000 | (_x))
#define SEGA_SUCCESS 0L
#define SEGA_ERROR_FAIL SEGARESULT_FAILURE(0)
#define SEGA_ERROR_BAD_POINTER SEGARESULT_FAILURE(3)
#define SEGA_ERROR_UNSUPPORTED SEGARESULT_FAILURE(5)
#define SEGA_ERROR_BAD_PARAM SEGARESULT_FAILURE(9)
#define SEGA_ERROR_INVALID_CHANNEL SEGARESULT_FAILURE(10)
#define SEGA_ERROR_INVALID_SEND SEGARESULT_FAILURE(11)
#define SEGA_ERROR_PLAYING SEGARESULT_FAILURE(12)
#define SEGA_ERROR_NO_RESOURCES SEGARESULT_FAILURE(13)
#define SEGA_ERROR_BAD_CONFIG SEGARESULT_FAILURE(14)
#define SEGA_ERROR_BAD_HANDLE SEGARESULT_FAILURE(18)
#define SEGA_ERROR_BAD_SAMPLERATE SEGARESULT_FAILURE(28)
#define SEGA_ERROR_OUT_OF_MEMORY SEGARESULT_FAILURE(31)
#define SEGA_ERROR_INIT_FAILED SEGARESULT_FAILURE(39)

/* Values used in various functions */
#define SYNTH_BUFFER 0x00000001
#define ALLOC_USER_MEM 0x00000002
#define USE_MAPPED_MEM 0x00000003
#define UNSIGNED_8PCM 0x0004
#define SIGNED_16PCM 0x0020
#define VOL_MAX 0xFFFFFFFF
#define P_MINIMUM 0
#define P_MAXIMUM 0xFFFFFFFF
#define UNUSED_SEND 0xFFFF0001

typedef enum
{
    STEREO_RETURN_FX2 = 0,
    STEREO_RETURN_FX3 = 1
} SegaProperty;

typedef enum
{
    RESOURCE_STOLEN = 0,
    NOTIFY = 2
} CallbackMessage;

typedef enum
{
    PLAYBACK_STATUS_STOP,
    PLAYBACK_STATUS_ACTIVE,
    PLAYBACK_STATUS_PAUSE,
    PLAYBACK_STATUS_INVALID = -1
} PlaybackStatus;

typedef struct
{
    unsigned int dwSampleRate;
    unsigned int dwSampleFormat;
    unsigned int byNumChans;
} OutputFormat;

typedef struct
{
    unsigned int dwSize;   
    unsigned int dwOffset; 
    void *hBufferHdr;      
} MapData;

typedef struct
{
    unsigned int dwPriority;                 
    unsigned int dwSampleRate;   
    unsigned int dwSampleFormat; 
    unsigned int byNumChans;     
    unsigned int dwReserved;     
    void *hUserData;             
    MapData mapData;             
} BufferConfig;

typedef enum Routing
{
    UNUSED_PORT = UNUSED_SEND,
    FRONT_LEFT_PORT = 0,
    FRONT_RIGHT_PORT = 1,
    FRONT_CENTER_PORT = 2,
    LFE_PORT = 3,
    REAR_LEFT_PORT = 4,
    REAR_RIGHT_PORT = 5,
    FXSLOT0_PORT = 10,
    FXSLOT1_PORT = 11,
    FXSLOT2_PORT = 12,
    FXSLOT3_PORT = 13
} Routing;

typedef enum
{
    SPDIFOUT_44_1KHZ = 0,
    SPDIFOUT_48KHZ,
    SPDIFOUT_96KHZ
} SPDIFOutputSampleRate;

typedef enum SoundBoardIO
{
    OUT_FRONT_LEFT = 0,
    OUT_FRONT_RIGHT = 1,
    OUT_FRONT_CENTER = 2,
    OUT_LFE_PORT = 3,
    OUT_REAR_LEFT = 4,
    OUT_REAR_RIGHT = 5,
    OUT_OPTICAL_LEFT = 10,
    OUT_OPTICAL_RIGHT = 11,
    IN_LINEIN_LEFT = 20,
    IN_LINEIN_RIGHT = 21
} SoundBoardIO;

typedef enum SynthParams
{
    ATTENUATION,              
    PITCH,                    
    FILTER_CUTOFF,            
    FILTER_Q,                 
    DELAY_VOL_ENV,            
    ATTACK_VOL_ENV,           
    HOLD_VOL_ENV,             
    DECAY_VOL_ENV,            
    SUSTAIN_VOL_ENV,          
    RELEASE_VOL_ENV,          
    DELAY_MOD_ENV,            
    ATTACK_MOD_ENV,           
    HOLD_MOD_ENV,             
    DECAY_MOD_ENV,            
    SUSTAIN_MOD_ENV,          
    RELEASE_MOD_ENV,          
    DELAY_MOD_LFO,            
    FREQ_MOD_LFO,             
    DELAY_VIB_LFO,            
    FREQ_VIB_LFO,             
    MOD_LFO_TO_PITCH,         
    VIB_LFO_TO_PITCH,         
    MOD_LFO_TO_FILTER_CUTOFF, 
    MOD_LFO_TO_ATTENUATION,   
    MOD_ENV_TO_PITCH,         
    MOD_ENV_TO_FILTER_CUTOFF  

} SynthParams;

typedef struct SynthParamSetExt
{
    SynthParams param;
    int lPARWValue;
} SynthParamSet;

int SEGAAPI_Play(void *hHandle);
int SEGAAPI_Pause(void *hHandle);
int SEGAAPI_Stop(void *hHandle);
int SEGAAPI_PlayWithSetup(void *hHandle);
PlaybackStatus SEGAAPI_GetPlaybackStatus(void *hHandle);
int SEGAAPI_SetFormat(void *hHandle, OutputFormat *pFormat);
int SEGAAPI_GetFormat(void *hHandle, OutputFormat *pFormat);
int SEGAAPI_SetSampleRate(void *hHandle, unsigned int dwSampleRate);
unsigned int SEGAAPI_GetSampleRate(void *hHandle);
int SEGAAPI_SetPriority(void *hHandle, unsigned int dwPriority);
unsigned int SEGAAPI_GetPriority(void *hHandle);
int SEGAAPI_SetUserData(void *hHandle, void *hUserData);
void *SEGAAPI_GetUserData(void *hHandle);
int SEGAAPI_SetSendRouting(void *hHandle, unsigned int dwChannel, unsigned int dwSend, Routing dwDest);
Routing SEGAAPI_GetSendRouting(void *hHandle, unsigned int dwChannel, unsigned int dwSend);
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
int SEGAAPI_SetSynthParam(void *hHandle, SynthParams param, int lPARWValue);
int SEGAAPI_GetSynthParam(void *hHandle, SynthParams param);
int SEGAAPI_SetSynthParamMultiple(void *hHandle, unsigned int dwNumParams, SynthParamSet *pSynthParams);
int SEGAAPI_GetSynthParamMultiple(void *hHandle, unsigned int dwNumParams, SynthParamSet *pSynthParams);
int SEGAAPI_SetReleaseState(void *hHandle, int bSet);
typedef void (*BufferCallback)(void *hHandle, CallbackMessage message);
int SEGAAPI_CreateBuffer(BufferConfig *pConfig, BufferCallback pCallback, unsigned int dwFlags, void **phHandle);
int SEGAAPI_DestroyBuffer(void *hHandle);
int SEGAAPI_SetGlobalEAXProperty(GUID *guid, unsigned long ulProperty, void *pData, unsigned long ulDataSize);
int SEGAAPI_GetGlobalEAXProperty(GUID *guid, unsigned long ulProperty, void *pData, unsigned long ulDataSize);
int SEGAAPI_SetSPDIFOutChannelStatus(unsigned int dwChannelStatus, unsigned int dwExtChannelStatus);
int SEGAAPI_GetSPDIFOutChannelStatus(unsigned int *pdwChannelStatus, unsigned int *pdwExtChannelStatus);
int SEGAAPI_SetSPDIFOutSampleRate(SPDIFOutputSampleRate dwSamplingRate);
SPDIFOutputSampleRate SEGAAPI_GetSPDIFOutSampleRate(void);
int SEGAAPI_SetSPDIFOutChannelRouting(unsigned int dwChannel, Routing dwSource);
Routing SEGAAPI_GetSPDIFOutChannelRouting(unsigned int dwChannel);
int SEGAAPI_SetIOVolume(SoundBoardIO dwPhysIO, unsigned int dwVolume);
unsigned int SEGAAPI_GetIOVolume(SoundBoardIO dwPhysIO);
void SEGAAPI_SetLastStatus(int LastStatus);
int SEGAAPI_GetLastStatus(void);
int SEGAAPI_Reset(void);
int SEGAAPI_Init(void);
int SEGAAPI_Exit(void);

#endif /* __SEGAAPI_H */
