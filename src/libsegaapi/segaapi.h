/**
 * Copyright (C) 2004-2005  Creative Technology Ltd.  All rights reserved.
 *
 ****************************************************************************
 *  \file               segaapi.h
 *  \brief
 *  This file contains the definition of the interfaces that requested by SEGA
 *  for audio output support.
 *
 *
 * @author      Creative
 * 
 * $Date: 2006/01/03 06:54:39 $
 * 
 ****************************************************************************
 * Revision History:
 *
 * 0.5  
 * 1st release to SEGA for review.
 *
 * 0.51
 * Added other interfaces to control effects, volume, optical-out etc.
 * Cleanup and added more information on the wave playback interfaces.
 *
 * 0.52
 * Added correlation between HASYNTHPARAMSEXT enumerators with SoundFont parameters.
 *
 * 0.53
 * Change name to segaapi.h (was WaveOutSegaAPI.h) as the API covers more than
 * just wave out.
 * Add prefix SEGAAPI_ to each of the functions to avoid names conflict.
 * Change HAERR_ to SEGAERR_, HA_SUCCESS to SEGA_SUCCESS
 *
 * 0.54
 * Added more documentation.
 *
 * 0.80
 * Added SetLastStatus and GetLastStatus functions.  
 * Change version to 0.80 to indicate close to final. 
 *
 * 0.9
 * Made the header file both gcc and g++ compliance as per request. 
 * 
 * 0.91
 * Updated voice priority description.  
 *
 * 0.92
 * Added SEGAAPI_Reset, and updated CreateBuffer() to support synthesizer buffer.  
 * 
 * 0.93
 * Added SEGAAPI_GetSendRouting and SEGAAPI_GetSetLevel per request.  
 * 
 * 0.94
 * Changed default send levels in SEGAAPI_CreateBuffer to 0.  
 *
 * 1.00
 * Added SEGAAPI_Init() and SEGAAPI_Exit() per request.
 *
 * 1.01
 * Updated SEGAAPI_SetReleaseState() document. 
 *
 * 1.02
*  Added SEGAAPI_SetSynthParamMultiple(0 and SEGAAPI_GetSynthParamMultiple().
 * Updated SEGAAPI_CreateBuffer() for user-mode buffer support.
 *
 ****************************************************************************
 * Released under NDA. 
 *
 * This document has been reviewed by SEGA.
 ****************************************************************************
 */

#ifndef __SEGAAPI_H
#define __SEGAAPI_H

// INCLUDES
#include "segadef.h"
#include "segaerr.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * The following defines SEGA custom EAX40 properties.
 */

// {A7FEEC3F-2BFD-4a40-891F-7423E38BAC1F}
DEFINE_GUID(EAXPROPERTYID_EAX40_SEGA_Custom, 
0xa7feec3f, 0x2bfd, 0x4a40, 0x89, 0x1f, 0x74, 0x23, 0xe3, 0x8b, 0xac, 0x1f);

// SEGA custom EAX40 properties
/*
 * The only property now is to switch the FX returns for 
 * FXSlot2 and FXSlot3 when non-reverb is loaded to these slots.
 *
 * EAXSEGA_STEREO_RETURN
 *    ulDataSize = CTDWORD
 *      value = 0 denotes route to front L/R (default)
 *      value = 1 denotes route to Rear L/R 
 */
typedef enum
{
    EAXSEGA_STEREO_RETURN_FX2 = 0,   
    EAXSEGA_STEREO_RETURN_FX3 = 1  
} EAXSEGA_PROPERTY;

/**
 * The following defines all of the messages wave output clients
 * can receive as part of their callback routines.
 */
typedef enum {
    HAWOS_RESOURCE_STOLEN = 0,
    HAWOS_NOTIFY = 2
} HAWOSMESSAGETYPE;


/*
 * The Playback status.
 */
typedef enum {
    PLAYBACK_STATUS_STOP,                      /* The voice is stopped */
    PLAYBACK_STATUS_ACTIVE,                    /* The voice is playing */
    PLAYBACK_STATUS_PAUSE,                     /* The voice is paused */
    PLAYBACK_STATUS_INVALID = -1               /* Invalid state */    
} PlaybackStatus;


/*
 * dwFlags use in CreateBuffer.
 */
#define HABUF_SYNTH_BUFFER		0x00000001	// indiate to create a synth buffer
#define HABUF_ALLOC_USER_MEM    0x00000002	// indiate that caller allocate memory
#define HABUF_USE_MAPPED_MEM    0x00000003	// indiate that caller allocate memory

/*
 * The HAWOSEFORMAT structure is used to change the format of an output client.
 */

#ifndef __HAWAVE_H 
#define HASF_UNSIGNED_8PCM      0x0004      /* Unsigned (offset 128) 8-bit PCM */
#define HASF_SIGNED_16PCM       0x0020      /* Signed 16-bit PCM */ 
#endif
 
typedef struct {
    CTDWORD     dwSampleRate;           /* The sample rate the client desires (in Hz) */
    CTDWORD     dwSampleFormat;         /* The sample format the client will use */
    CTDWORD     byNumChans;             /* The number of samples in the sample
                                         *  frame (1 = mono, 2 = stereo).  */
} HAWOSEFORMAT;



/*
 * HAWOSEMAPDATA contains
 */
typedef struct {
    CTDWORD     dwSize;       /* Supply by caller. Size (in bytes) of the valid sample data */
    CTDWORD     dwOffset;     /* Return by driver. Offset of buffer where the the first valid sample should be written to */
    CTHANDLE    hBufferHdr;   /* Return by driver. Memory address that user-space application can access. */
} HAWOSEMAPDATA;


/*
 * The HAWOSEBUFFERCONFIG structure is used to describe how an input or
 * output buffer client wishes to configure the device when it opens it.
 */
typedef struct {
    CTDWORD     dwPriority;         /* The priority with which the voices
                                     *  should be allocated.  This is used
                                     *  when voices need to be ripped off. */
    CTDWORD     dwSampleRate;       /* The sample rate the voice desires */
    CTDWORD     dwSampleFormat;     /* The sample format the voice will use */
    CTDWORD     byNumChans;         /* The number of samples in the sample
                                     *  frame. (1 = mono, 2 = stereo). */
    CTDWORD     dwReserved;         /* Reserved field */
    CTHANDLE    hUserData;          /* User data */
    HAWOSEMAPDATA mapData;          /* The sample memory mapping for the buffer. */
} HAWOSEBUFFERCONFIG;



/**
 * Default values
 */
#define HAWOSEVOL_MAX      0xFFFFFFFF  /* Maximum volume; no attenuation */


/**
 * Since Tina has up to 64- voices, voice priorities typically ranging 
 * from 0 to 63, where 0 is lower priority (more likely to get ripped off)
 * than 63.
 *
 * Set voice priority to HAWOSEP_MAXIMUM if a voice must never get ripped
 * off under any circumstances.
 */
#define HAWOSEP_MINIMUM    0
#define HAWOSEP_MAXIMUM    0xFFFFFFFF


/** @brief Routing List
 * 
 * voice sends routing to speakers or effects ports.
 *
 */
#define HAWOSE_UNUSED_SEND 0xFFFF0001


typedef enum HAROUTING{
    HA_UNUSED_PORT=HAWOSE_UNUSED_SEND,

    // Dry multi-channel outputs
    HA_FRONT_LEFT_PORT =0,
    HA_FRONT_RIGHT_PORT=1,
    HA_FRONT_CENTER_PORT=2,
    HA_LFE_PORT=3,
    HA_REAR_LEFT_PORT=4,
    HA_REAR_RIGHT_PORT=5,

    // effect outputs
    HA_FXSLOT0_PORT=10,
    HA_FXSLOT1_PORT=11,
    HA_FXSLOT2_PORT=12,
    HA_FXSLOT3_PORT=13

} HAROUTING;  


/**
 * The following defines SPDIF-Out sampling rate.
 */
typedef enum {
    HASPDIFOUT_44_1KHZ=0,
    HASPDIFOUT_48KHZ,
    HASPDIFOUT_96KHZ
} HASPDIFOUTRATE;


/**
 * The following defines inputs and outputs of SEGA sound board.
 */
typedef enum HAPHYSICALIO {
    // analog outputs
    HA_OUT_FRONT_LEFT =0,
    HA_OUT_FRONT_RIGHT=1,
    HA_OUT_FRONT_CENTER=2,
    HA_OUT_LFE_PORT=3,
    HA_OUT_REAR_LEFT=4,
    HA_OUT_REAR_RIGHT=5,

    // optical Outputs
    HA_OUT_OPTICAL_LEFT=10,
    HA_OUT_OPTICAL_RIGHT=11,

    // Line In
    HA_IN_LINEIN_LEFT=20,
    HA_IN_LINEIN_RIGHT=21

}HAPHYSICALIO ;  


/** @brief Synth parameters enumeration list
 *
 * This table defines the most common (and hardware-supported)
 * control routings in Real World Unit.
 *
 * Refers to DLS spec or SoundFont spec for details of these Parameters,
 * their units and their ranges.
 */
typedef enum HASYNTHPARAMSEXT {
    HAVP_ATTENUATION,                           ///< 0,         0x00,  initialAttenuation
    HAVP_PITCH,                                 ///< 1,         0x01,  fineTune + coarseTune * 100
    HAVP_FILTER_CUTOFF,                         ///< 2,         0x02,  initialFilterFc
    HAVP_FILTER_Q,                              ///< 3,         0x03,  initialFilterQ
    HAVP_DELAY_VOL_ENV,                         ///< 4,         0x04,  delayVolEnv
    HAVP_ATTACK_VOL_ENV,                        ///< 5,         0x05,  attackVolEnv
    HAVP_HOLD_VOL_ENV,                          ///< 6,         0x06,  holdVolEnv
    HAVP_DECAY_VOL_ENV,                         ///< 7,         0x07,  decayVolEnv
    HAVP_SUSTAIN_VOL_ENV,                       ///< 8,         0x08,  sustainVolEnv
    HAVP_RELEASE_VOL_ENV,                       ///< 9,         0x09,  releaseVolEnv
    HAVP_DELAY_MOD_ENV,                         ///< 10,        0x0A,  delayModEnv
    HAVP_ATTACK_MOD_ENV,                        ///< 11,        0x0B,  attackModEnv
    HAVP_HOLD_MOD_ENV,                          ///< 12,        0x0C,  holdModEnv
    HAVP_DECAY_MOD_ENV,                         ///< 13,        0x0D,  decayModEnv
    HAVP_SUSTAIN_MOD_ENV,                       ///< 14,        0x0E,  sustainModEnv
    HAVP_RELEASE_MOD_ENV,                       ///< 15,        0x0F,  releaseModEnv
    HAVP_DELAY_MOD_LFO,                         ///< 16,        0x10,  delayModLFO
    HAVP_FREQ_MOD_LFO,                          ///< 17,        0x11,  freqModLFO
    HAVP_DELAY_VIB_LFO,                         ///< 18,        0x12,  delayVibLFO
    HAVP_FREQ_VIB_LFO,                          ///< 19,        0x13,  freqVibLFO
    HAVP_MOD_LFO_TO_PITCH,                      ///< 20,        0x14,  modLfoToPitch
    HAVP_VIB_LFO_TO_PITCH,                      ///< 21,        0x15,  vibLfoToPitch
    HAVP_MOD_LFO_TO_FILTER_CUTOFF,              ///< 22,        0x16,  modLfoToFilterFc
    HAVP_MOD_LFO_TO_ATTENUATION,                ///< 23,        0x17,  modLfoToVolume
    HAVP_MOD_ENV_TO_PITCH,                      ///< 24,        0x18,  modEnvToPitch
    HAVP_MOD_ENV_TO_FILTER_CUTOFF               ///< 25,        0x19,  modEnvToFilterFc

} HASYNTHPARAMSEXT;

#ifndef __SYNTHPARAMSET_
#   define __SYNTHPARAMSET_
typedef struct SynthParamSetExt {
    HASYNTHPARAMSEXT param;
    CTLONG  lPARWValue;
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



/*
 * Interfaces expose.  These interfaces will be exposed in user mode.
 * 
 * Note:
 * 1. hHandle that passes into these functions is An opaque identifier 
 *    obtained from CreateBuffer.
 *
 * 2. A mono buffer uses one voice.  A stereo buffer uses two voices.
 *
 */


/***********************************************************
 * @section 
 * API for playback operation controls.
 * 
 */
  
/**
 * Starts sample playback of a buffer.
 * Playback position is not modified when Play is called and will 
 * start incrementing from its previous value at the sample
 * rate.
 *
 * CALL LEVELS: PASSIVE
 *
 * @param hHandle
 * An opaque identifier obtained from CreateBuffer. 
 *
 * @return 
 * Returns SEGA_SUCCESS if playback can start.  Otherwise, returns an appropriate 
 * error code.
 */
SEGASTATUS SEGAAPI_Play(CTHANDLE hHandle);


/**
 * Halts playback and freezes the current counter at its last
 * value.
 * 
 * CALL LEVELS: PASSIVE
 *
 * @param hHandle
 * An opaque identifier obtained from CreateBuffer. 
 *
 * @return 
 * Returns SEGA_SUCCESS if playback was successfully paused. 
 * Otherwise, returns an appropriate error code.
 */
SEGASTATUS SEGAAPI_Pause(CTHANDLE hHandle);


/**
 * Stops playback and resets the sample counter.
 *
 *
 * CALL LEVELS: PASSIVE
 *
 * @param hHandle
 * An opaque identifier obtained from CreateBuffer. 
 * 
 * @return 
 * Returns SEGA_SUCCESS if playback was successfully paused. 
 * Otherwise, returns an appropriate error code.
 *
 */
SEGASTATUS SEGAAPI_Stop(CTHANDLE hHandle);

SEGASTATUS SEGAAPI_PlayWithSetup(CTHANDLE hHandle);

/**
 * Returns a current playback status of a buffer.
 *
 * CALL LEVELS: DPC, PASSIVE
 *
 * @param hHandle
 * An opaque identifier obtained from CreateBuffer. 
 *
 * @return 
 * One of the playback status defined in the PlaybackStatus enumration type.
 * If the returned status is PLAYBACK_STATUS_INVALID, use GetLastStatus() to check the error code.
 */
PlaybackStatus SEGAAPI_GetPlaybackStatus(CTHANDLE hHandle);




/***********************************************************
 * @section 
 * API for playback format controls.
 * 
 */

/**
 * Changes the buffer's format to the values specified in
 * the new format pFormat.  In some cases, it is possible that
 * an attempt to change the configuration may fail, as the change may
 * require more resources than were previously allocated.
 *
 * The playback buffer configuration may not be changed while the buffer is
 * playing.
 *
 * CALL LEVELS: PASSIVE
 *
 * @param hHandle
 * An opaque identifier obtained from CreateBuffer. 
 * 
 * @param pFormat 
 * The new format to change to.
 * 
 * @return
 * Returns SEGA_SUCCESS if successful.  Otherwise, returns an appropriate 
 * error code.
 *
 * @retval SEGAERR_PLAYING if the buffer is currently playing
 * @retval SEGAERR_BAD_POINTER if pFormat is NULL
 * @retval SEGAERR_NO_RESOURCES if insufficient resources are available
 * @retval SEGAERR_BAD_CONFIG if something in the given configuration is
 * invalid.
 */
SEGASTATUS SEGAAPI_SetFormat(CTHANDLE hHandle, HAWOSEFORMAT *pFormat);


/**
 * Returns the current format of the buffer.
 * 
 * CALL LEVELS: PASSIVE
 *
 * @param hHandle
 * An opaque identifier obtained from CreateBuffer. 
 *
 * @param   pFormat
 * Pointer to an address where the current format to return to.
 *
 * @return
 * Returns SEGA_SUCCESS if successful.  Otherwise, returns an appropriate 
 * error code.
 */
SEGASTATUS SEGAAPI_GetFormat(CTHANDLE hHandle, HAWOSEFORMAT *pFormat);


/**
 * Changes the playback sample rate for the current client to the
 * value specified.  The new value only pertains to this buffer.
 * If hardware cannot support changing sample rates for individual
 * buffers, it can return an error in response to this routine.
 * 
 * CALL LEVELS: PASSIVE
 * 
 * @param hHandle
 * An opaque identifier obtained from CreateBuffer. 
 *
 * @param  dwSampleRate
 * The desired sample rate.
 * 
 * @return
 * Returns SEGA_SUCCESS if successful.  Otherwise, returns an appropriate 
 * error code.
 */
SEGASTATUS SEGAAPI_SetSampleRate(CTHANDLE hHandle, CTDWORD dwSampleRate);


/**
 * Returns the current sample rate.
 *
 * CALL LEVELS: PASSIVE
 *
 * @param hHandle
 * An opaque identifier obtained from CreateBuffer. 
 *
 * @return 
 * Returns the current sample rate.
 * If the returned value is 0, use GetLastStatus() to check the error code.
 */
CTDWORD SEGAAPI_GetSampleRate(CTHANDLE hHandle);



/***********************************************************
 * @section 
 * API for Voice priority management.
 *
 */

 /**
 * Changes the buffer's priority to the specified value.
 *
 * If all the voices are set to HAWOSEP_MAXIMUM (0xFFFFFFFF) priority, 
 * CreateBuffer() call will return failure when running out of voices.
 *
 * CALL LEVEL: DPC, PASSIVE
 * 
 * @param hHandle
 * An opaque identifier obtained from CreateBuffer. 
 *
 * @param dwPriority
 * The new priority for the buffer.
 *
 * @return
 * Returns SEGA_SUCCESS if successful.  Otherwise, returns an appropriate 
 * error code.
 */
SEGASTATUS SEGAAPI_SetPriority(CTHANDLE hHandle, CTDWORD dwPriority);


/**
 * Returns the buffer's current priority.
 *
 * CALL LEVEL: DPC, PASSIVE
 *
 * @param hHandle
 * An opaque identifier obtained from CreateBuffer. 
 *
 * @return
 * Returns the buffer's current priority.
 * Note that returned value is also set to 0 if error invoked this function.
 */
CTDWORD SEGAAPI_GetPriority(CTHANDLE hHandle);


/***********************************************************
 * @section 
 * API for storing an User-defined data.
 * 
 */

/**
 * Stores a handle to user-defined data defined by the client structure.
 * This allows caller to associate caller-specific data.  The caller is
 * responsible for managing this data area.
 *
 * Note that caller can specify an user-defined data in the CreateBuffer()
 * call.  Caller can use SetUserData() to update the user-defined data.
 *
 * CALL LEVELS: DPC, PASSIVE
 *
 * @param hHandle
 * An opaque identifier obtained from CreateBuffer. 
 *
 * @param  hUserData   
 * A handle to user-defined data.
 *
 * @return
 * Returns SEGA_SUCCESS if successful.  Otherwise, returns an appropriate 
 * error code.
 */
SEGASTATUS SEGAAPI_SetUserData(CTHANDLE hHandle, CTHANDLE hUserData);


/**
 * Returns the last user-defined data set by the caller.
 *
 * CALL LEVELS: DPC, PASSIVE
 *
 * @param hHandle
 * An opaque identifier obtained from CreateBuffer. 
 *
 * @return
 * Returns the user-defined data.
 * Note that returned value is set to 0 if error invoked this function.
 */
CTHANDLE SEGAAPI_GetUserData(CTHANDLE hHandle);


/***********************************************************
 * @section 
 * API for Send routing and Send Level controls.
 * 
 */

/**
 * Changes the destination to which a channel send is connected.
 * Each channel has the possibility of supporting multiple sends.
 * For Tina chip, each channel has seven sends.
 * Each of these sends can be connected to a destination.  
 *
 * Note that it is invalid to have more than one sends routed to
 * a same destination.  For example, if send 0 is previously routed
 * to front-left, send 0 will need to be disconnected before another
 * send can route to front-left. 
 *
 * CALL LEVELS: PASSIVE
 *
 * @param hHandle
 * An opaque identifier obtained from CreateBuffer. 
 *
 * @param   dwChannel
 *  The channel to reroute.
 *
 * @param   dwSend
 *  The send number being addressed.
 *
 * @param   dwDest
 *  The destination to which the send should be connected.
 *  If the send doesn't need to be connected to anything,
 *  specify HAWOSE_UNUSED_SEND.  See HAROUTING for the details
 *  of the destination enumeration list.
 *
 * @return
 * Returns SEGA_SUCCESS if successful.  Otherwise, returns an appropriate 
 * error code.
 *
 * @retval  SEGAERR_UNSUPPORTED if channels can't be rerouted.
 * @retval  SEGAERR_INVALID_CHANNEL if the specified channel isn't in use.
 * @retval  SEGAERR_INVALID_SEND if the specified send isn't supported.
 */
SEGASTATUS SEGAAPI_SetSendRouting(CTHANDLE hHandle, CTDWORD dwChannel, CTDWORD dwSend,
                           HAROUTING dwDest);

/**
 * Returns the destination of which a channel send is connected to.
 *
 * CALL LEVELS: PASSIVE
 * 
 * @param hHandle
 * An opaque identifier obtained from CreateBuffer. 
 *
 * @param   dwChannel
 *  The channel requested.
 *
 * @param   dwSend
 *  The send number being addressed.
 *
 * @return  
 * Returns the current destination.
 * Note that returned value is set to HA_UNUSED_PORT if error invoked this function.  
 */
HAROUTING SEGAAPI_GetSendRouting(CTHANDLE hHandle, CTDWORD dwChannel, CTDWORD dwSend); 
                           
/**
 * Sets the output level of a particular send on a channel to the specified
 * level.
 *
 * CALL LEVELS: PASSIVE
 *
 * @param hHandle
 * An opaque identifier obtained from CreateBuffer. 
 * 
 * @param dwChannel
 * The channel whose output level is to be set.
 *
 * @param dwSend
 * The send being addressed.
 *
 * @param dwLevel
 * The output level.  Output levels are specified
 * in linear values.  A value of 0xFFFFFFFF indicates
 * full on (0 attenuation).  A value of 0x0 indicates
 * infinite attenuation.
 *
 * @return
 * Returns SEGA_SUCCESS if successful.  Otherwise, returns an appropriate 
 * error code.
 * 
 * @retval   SEGAERR_UNSUPPORTED if the device doesn't support channel levels in
 * general.
 * @retval   SEGAERR_INVALID_CHANNEL if the specified channel isn't valid.
 * @retval   SEGAERR_INVALID_SEND if the specified send isn't valid.
 */
SEGASTATUS SEGAAPI_SetSendLevel(CTHANDLE hHandle, CTDWORD dwChannel, CTDWORD dwSend,
                       CTDWORD dwLevel);

/**
 * Returns the output level of a particular send on a channel.
 *
 * CALL LEVELS: PASSIVE
 *
 * @param hHandle
 * An opaque identifier obtained from CreateBuffer. 
 * 
 * @param   dwChannel
 *  The channel requested.
 *
 * @param   dwSend
 *  The send number being addressed.
 *
 * @return  
 * Returns the current send level.
 * Note that returned value is set to 0 if error invoked this function. 
 */
CTDWORD SEGAAPI_GetSendLevel(CTHANDLE hHandle, CTDWORD dwChannel, CTDWORD dwSend); 



/***********************************************************
 * @section 
 * API for volume level controls.
 * 
 */

/**
 * Sets the volume to a specific linear value.  Volumes are specified
 * as fractional fixed-point linear values between HAWOSEVOL_MAX (0xFFFFFFFF)
 * and 0x0. The dwVolume is specified in linear increments from 0 to 1 
 * (actually to 65535 divided by 65536).  A 0 value represents 96db of attenuation, 
 * while a 1 value represents full volume. Default is full volume.
 *
 * Volume is a global value and is applied pre-send.
 *
 * CALL LEVELS: PASSIVE
 * 
 * @param hHandle
 * An opaque identifier obtained from CreateBuffer. 
 *
 * @param dwChannel
 * The channel to change.
 *
 * @param dwVolume
 * The new volume level to change to.
 *
 * @return
 * Returns SEGA_SUCCESS if successful.  Otherwise, returns an appropriate 
 * error code.
 *
 * @retval   SEGAERR_UNSUPPORTED if the device can't change volume.
 * @retval   SEGAERR_INVALID_CHANNEL if the given send isn't valid.
 */
SEGASTATUS SEGAAPI_SetChannelVolume(CTHANDLE hHandle, CTDWORD dwChannel, CTDWORD dwVolume);

/**
 * Returns the current volume level for the requested channel.
 *
 * CALL LEVELS: PASSIVE
 * 
 * @param hHandle
 * An opaque identifier obtained from CreateBuffer. 
 *
 * @param   dwChannel
 * The channel requested.
 *
 * @return  
 * Returns the current volume.
 * Note that returned value is set to 0 if error invoked this function.
 */
CTDWORD SEGAAPI_GetChannelVolume(CTHANDLE hHandle, CTDWORD dwChannel);



/***********************************************************
 * @section * API for playback position controls.
 * 
 */

/**
 * Changes the buffer position pointer from which hardware
 * is fetching samples.   Changes take place immediately and
 * can be made while playback is occurring.
 *
 * CALL LEVELS: DPC, PASSIVE
 *
 * @param hHandle
 * An opaque identifier obtained from CreateBuffer. 
 *
 * @param  dwPlaybackPos
 * The buffer position (IN BYTES) where the playback
 * pointer should be moved.
 *
 * @return
 * Returns SEGA_SUCCESS if successful.  Otherwise, returns an appropriate 
 * error code.
 */
SEGASTATUS SEGAAPI_SetPlaybackPosition(CTHANDLE hHandle, CTDWORD dwPlaybackPos);


/**
 * Returns the position in the buffer (IN BYTES) where the
 * hardware is currently playing samples from.
 *
 * CALL LEVELS: DPC, PASSIVE
 *
 * @param hHandle
 * An opaque identifier obtained from CreateBuffer. 
 *
 * @return
 * Returns the current playback position.
 * Note that returned value is set to 0 if error invoked this function.
 */
CTDWORD SEGAAPI_GetPlaybackPosition(CTHANDLE hHandle);


/***********************************************************
 * @section 
 * API for buffer update, loop regions and notification controls.
 * 
 */

/**
 * This function sets the frequency at which a callback will be generated.
 * The callback will be invoked periodically at a fixed interval
 * specified by the dwFrameCount.  dwFrameCount is in the units of 
 * sample frames.
 *
 * This notification method is typically used for ring buffer that need 
 * periodic notification to update the ring buffer data.
 *
 * Note that callback execution is scheduled at the later time (DPC), not at   
 * the interrupt time.
 *
 *
 * CALL LEVELS: PASSIVE
 *
 * @param   hHandle
 * An opaque identifier obtained from CreateBuffer. 
 *
 * @param   dwFrameCount
 * The frequency, in sample frames, at which the the notification is invoked.
 * Specifying a value of zero cancels the callback.
 *
 * @return
 * Returns SEGA_SUCCESS if successful.  Otherwise, returns an appropriate 
 * error code.
 */
SEGASTATUS SEGAAPI_SetNotificationFrequency(CTHANDLE hHandle, CTDWORD dwFrameCount);

/**
 * This function can be used to set a notification point in the ring
 * buffer.  Whenever the play position passes over a notification point
 * the device will schedule a callback to the function indicated when
 * the ring buffer was created.
 *
 * Note that callback execution is scheduled at the later time (DPC), not at   
 * the interrupt time.
 *
 * CALL LEVELS: PASSIVE, DPC
 *
 * @param hHandle
 * An opaque identifier obtained from CreateBuffer. 
 *
 * @param   dwBufferOffset
 * The offset (in bytes) in the buffer where the notification
 * point is to be set.
 *
 * @return
 * Returns SEGA_SUCCESS if successful.  Otherwise, returns an appropriate 
 * error code.
 */
SEGASTATUS SEGAAPI_SetNotificationPoint(CTHANDLE hHandle, CTDWORD dwBufferOffset);

/**
 * Removes a previously set notification point.
 *
 * CALL LEVELS: PASSIVE, DPC
 *
 * @param hHandle
 * An opaque identifier obtained from CreateBuffer. 
 *
 * @param   dwBufferOffset
 * The offset (in bytes) of the notification point to
 * remove.
 *
 * @return
 * Returns SEGA_SUCCESS if successful.  Otherwise, returns an appropriate 
 * error code.
 */
SEGASTATUS SEGAAPI_ClearNotificationPoint(CTHANDLE hHandle, CTDWORD dwBufferOffset);


/**
 * Sets the start loop offset.  The start loop offset controls where
 * the play pointer will jump when it crosses the End Loop Offset.
 * There is no requirement that the start loop offset preceed the
 * End Loop Offset.  It is illegal to set Start Loop Offset and
 * End Loop Offset to the same value, however, and in general it is
 * a bad idea to have the difference between the two be less than
 * 16 samples.
 *
 * GetStartLoopOffset() just returns the current start loop offset
 * value and may be called at any interrupt level.
 *
 * CALL LEVELS: DPC, PASSIVE.
 *
 * @param hHandle
 * An opaque identifier obtained from CreateBuffer. 
 *
 * @param   dwOffset
 * The offset in bytes from the beginning of the buffer of
 * the loop start point.
 *
 * @return
 * Returns SEGA_SUCCESS if the loop offset is changed successful.  
 * Otherwise, returns an appropriate error code.
 */
SEGASTATUS SEGAAPI_SetStartLoopOffset(CTHANDLE hHandle, CTDWORD dwOffset);


/**
 * Returns the current start loop offest.
 *
 * CALL LEVELS: DPC, PASSIVE.
 *
 * @param hHandle
 * An opaque identifier obtained from CreateBuffer. 
 *
 * @return
 * Returns the current start loop offset.
 * Note that returned value is set to 0 if error invoked this function.
 */
CTDWORD SEGAAPI_GetStartLoopOffset(CTHANDLE hHandle);


/**
 * Sets the End Loop Offset position.  When the play pointer crosses
 * the End Loop Offset it will jump to the Start Loop Offset
 * position if buffer is in looping state (bDoContinuousLooping
 * is set to TRUE).
 *
 * GetEndLoopOffset() just returns the current value and may be called
 * at any interrupt level.
 *
 * Note that EndLoopOffset must not be larger than EndOffset. 
 *
 * CALL LEVELS: DPC, PASSIVE.
 *
 * @param hHandle
 * An opaque identifier obtained from CreateBuffer. 
 *
 * @param  dwOffset
 * An offset in bytes from the beginning of the buffer,
 * dwOffset specifies the location for the End Loop point.
 *
 * @return
 * Returns SEGA_SUCCESS if successful.  Otherwise, returns an appropriate 
 * error code.
 */
SEGASTATUS SEGAAPI_SetEndLoopOffset(CTHANDLE hHandle, CTDWORD dwOffset);


/**
 * Returns the current end loop offest.
 *
 * CALL LEVELS: DPC, PASSIVE.
 *
 * @param hHandle
 * An opaque identifier obtained from CreateBuffer. 
 *
 * @return 
 * Returns the current end loop offest.
 * Note that returned value is set to 0 if error invoked this function.
 */
CTDWORD SEGAAPI_GetEndLoopOffset(CTHANDLE hHandle);


/**
 * Sets the End Offset position.  When the play pointer crosses
 * the End Offset (assuming the buffer isn't currently looping)
 * the buffer will halt.
 *
 * Only change the End offset position when buffer is not at
 * PLAYBACK_STATUS_ACTIVE state.  End Offset must be sample frame aligned.  
 * For example, 16-bit 1 channel is WORD aligned, 16-bit 2 channel 
 * is DWORD aligned.
 *
 * Note that EndOffset must not be larger than pConfig->mapdata.dwSize
 * specified in the CreateBuffer(). 
 *
 * CALL LEVELS: DPC, PASSIVE.
 *
 * @param hHandle
 * An opaque identifier obtained from CreateBuffer. 
 *
 * @param   dwOffset
 * An offset in bytes from the beginning of the buffer,
 * dwOffset specifies the location for the End point.
 *
 * @return
 * Returns SEGA_SUCCESS if successful.  Otherwise, returns an appropriate 
 * error code.
 */
SEGASTATUS SEGAAPI_SetEndOffset(CTHANDLE hHandle, CTDWORD dwOffset);

/**
 * Returns the current end offest.
 *
 * CALL LEVELS: DPC, PASSIVE.
 *
 * @param hHandle
 * An opaque identifier obtained from CreateBuffer. 
 *
 * @return
 * Returns the current end offest.
 * Note that returned value is set to 0 if error invoked this function.
 */
CTDWORD SEGAAPI_GetEndOffset(CTHANDLE hHandle);

/**
 * Allows the user to control whether the voice loops back to the
 * Start Loop Offset when it crosses the End Loop Point or whether
 * it goes into the release phase (i.e. post end loop).  Note that
 * setting the loop state doesn't actually cause the device
 * to transition to the stopped state.
 * 
 *
 * CALL LEVELS: DPC, PASSIVE.
 *
 * @param hHandle
 * An opaque identifier obtained from CreateBuffer. 
 *
 * @param   bDoContinuousLooping
 * If TRUE, the buffer will loop from start-loop to end
 * loop and back again. If FALSE, loop points are ignored
 * and buffer will play until the sample end, as programmed
 * with SetEndOffset. This may be programmed when as the buffer
 * is playing.
 *
 * @return
 * Returns SEGA_SUCCESS if successful.  Otherwise, returns an appropriate 
 * error code.
 */
SEGASTATUS SEGAAPI_SetLoopState(CTHANDLE hHandle, CTBOOL bDoContinuousLooping);

/**
 * Returns the current loop status. 
 *
 * CALL LEVELS: DPC, PASSIVE.
 *
 * @param hHandle
 * An opaque identifier obtained from CreateBuffer. 
 *
 * @return
 * Returns TRUE if it is in loop state. Otherwise, returns FALSE. 
 * Note that returned value is set to FALSE if error invoked this function.
 */
CTBOOL SEGAAPI_GetLoopState(CTHANDLE hHandle);


/**
 * Advises the driver code that some portion of the buffer
 * has been written with new data.  This method is required for devices
 * which don't support a memory-mapped ring buffer and allows the
 * underlying software to perform any necessary copying or format
 * conversion.
 *
 * The caller should call this function *after* they have filled
 * the data into the ring buffer that they passed
 * to the driver in the CreateBuffer() call.  
 *
 * Although this routine takes it values in bytes, the caller
 * is responsible for insuring that the starting offset and
 * length are a integer multiple of the sample size.
 *
 * CALL LEVELS: DPC, PASSIVE.
 * 
 * @param hHandle
 * An opaque identifier obtained from CreateBuffer. 
 *
 * @param  dwStartOffset
 *  The offset of the first byte in the buffer which
 *  has changed (between 0 and bufferSize-1)
 * @param  dwLength
 *  The number of bytes which have changed.
 *
 * @return
 * Returns SEGA_SUCCESS if successful.  Otherwise, returns an appropriate 
 * error code.
 */
SEGASTATUS SEGAAPI_UpdateBuffer(CTHANDLE hHandle, CTDWORD dwStartOffset, CTDWORD dwLength);


/***********************************************************
 * @section 
 * Low level API to control Synth buffer parameters
 *
 */

/**
 * Sets and stores a synthesizer parameter, in Perceptually-Additive Real-World (PARW) units
 *
 * The parameter is applied for mono buffer only.
 *
 * CALL LEVELS: DPC, PASSIVE.
 * 
 * @param hHandle
 * An opaque identifier obtained from CreateBuffer. 
 *
 * @param param
 * The parameter to apply
 *
 * @param lPARWValue
 * The value in PARW units
 *
 * @return
 * Returns SEGA_SUCCESS if successful.  Otherwise, returns an appropriate 
 * error code.
 */
SEGASTATUS SEGAAPI_SetSynthParam(CTHANDLE hHandle, HASYNTHPARAMSEXT param, CTLONG lPARWValue);


/**
 * Returns the most recent call to SetSynthParam() in PARW units.  This is the cache value
 * of the most recent PARW value set by SetSynthParam(). If the parameter has not been set 
 * before, this function will return 0.  
 *
 * The parameter is applied for mono buffer only.
 *
 * CALL LEVELS: DPC, PASSIVE.
 *
 * @param hHandle
 * An opaque identifier obtained from CreateBuffer. 
 *
 * @param param
 * The parameter to retrieve
 *
 * @return
 * The returned value in PARW units.
 * Note that returned value is set to -1 if error invoked this function.
 */
CTLONG SEGAAPI_GetSynthParam(CTHANDLE hHandle, HASYNTHPARAMSEXT param);


/**
 * Sets and stores an array of synthesizer parameters, in Perceptually-Additive Real-World (PARW) units.
 *
 * The parameter is applied for mono buffer only.
 *
 * CALL LEVELS: DPC, PASSIVE.
 * 
 * @param hHandle
 * An opaque identifier obtained from CreateBuffer. 
 *
 * @param dwNumParams
 * Number of parameters to apply
 *
 * @param pSynthParams
 * Pointer to the Synth Parameters array to apply.
 *
 * @return
 * Returns SEGA_SUCCESS if successful.  Otherwise, returns an appropriate 
 * error code.
 */
SEGASTATUS SEGAAPI_SetSynthParamMultiple(CTHANDLE hHandle, CTDWORD dwNumParams, SynthParamSet *pSynthParams);


/**
 * Retrieves an array of synthesizer parameters, in Perceptually-Additive Real-World (PARW) units.
 *
 * The parameter is applied for mono buffer only.
 *
 * CALL LEVELS: DPC, PASSIVE.
 * 
 * @param hHandle
 * An opaque identifier obtained from CreateBuffer. 
 *
 * @param dwNumParams
 * Number of parameters to retrieve.
 *
 * @param pSynthParams
 * Pointer to the Synth Parameters array to retrieve.
 *
 * @return
 * Returns SEGA_SUCCESS if successful.  Otherwise, returns an appropriate 
 * error code.
 */
SEGASTATUS SEGAAPI_GetSynthParamMultiple(CTHANDLE hHandle, CTDWORD dwNumParams, SynthParamSet *pSynthParams);


/**
 * Set the voice into the release phase of the volume envelope engines when set to TRUE.  
 * This will automatically stop the voice when the voice reaches end of release phase.
 *
 * CALL LEVELS: DPC, PASSIVE.
 *
 * @param hHandle
 * An opaque identifier obtained from CreateBuffer. 
 *
 * @param bSet
 * TRUE for enter releaes phase
 *
 * @return
 * Returns SEGA_SUCCESS if successful.  Otherwise, returns an appropriate 
 * error code.
 */
SEGASTATUS SEGAAPI_SetReleaseState(CTHANDLE hHandle, CTBOOL bSet);



/***********************************************************
 * @section 
 * Playback buffers (voices) callback notification function.  
 * 
 */

/**
 * A callback function of this type is passed into the
 * CreateBuffer() function and gets invoked when a WaveOutBuffer
 * client needs to notify the OS of some event.  Currently, the following event
 * types are defined:
 *
 *  HAWOS_RESOURCE_STOLEN -- Indicates the resources used to
 *  play the audio have been stolen.
 *
 *  HAWOS_NOTIFY -- Indicates that the current play position
 *  has passed over one of the notification points set
 *  with SetNotificationPoint.
 * 
 * CALL LEVELS: The callback is invoked at DPC level.
 *
 * @param hHandle
 * An opaque identifier obtained from CreateBuffer. 
 *
 * @param   HAWOSMESSAGETYPE message
 * The callback message.
 */
typedef void (*HAWOSEGABUFFERCALLBACK)(CTHANDLE hHandle,
   HAWOSMESSAGETYPE message);


/***********************************************************
 * @section 
 * API to create playback buffers (voices).  
 * 
 * This is the entry call to obtain the voice handle
 * and perform further voice operations.
 */

/**
 * Creates a new buffer (voice) of the output device using 
 * the configuration specified in pConfig.
 *
 * There are two types of buffers: normal buffer and synthesizer buffer.  
 * Typically, a buffer is created as normal buffer.  Synthesizer buffer 
 * is used if a buffer requires synthesizer type of parameter controls.
 *
 * Caller specifies the configuration to create in pConfig structure.
 * Driver will allocate memory and hardware resources needed for this 
 * voice creation.  If successful, the pConfig->mapdata.hBufferHdr contains  
 * the memory address that user-space application can access.  Caller 
 * should write the valid sound data into this memory buffer.
 *
 * Caller can indicate to the sound driver to use the caller allocated sound 
 * data memory buffer for efficiency purpose if the same buffer is re-used frequently.
 * In such a case, caller needs to provide the sound buffer address in the  
 * pConfig->mapdata.hBufferHdr and sets the HABUF_ALLOC_USER_MEM bit
 * of dwFlags parameter.  Caller needs to ensure that the sound buffer
 * memory is page-aligned and locked. 
 *
 * pConfig->mapdata.dwSize must be sample frame aligned.  For example, 
 * 16-bit 1 channel is WORD aligned, 16-bit 2 channel is DWORD aligned.
 * For more efficient memory management, pConfig->mapdata.dwSize is 
 * recomended to be page aligned.
 *
 * The size of the ring buffer is fixed throughout the lifetime of the 
 * buffer until it is destroyed.  During the lifetime of the buffer, 
 * caller can periodically update the data of the buffer if necessary or 
 * modulate the buffer with SetSynthParam or SetSynthParamMultiple functions 
 * if the buffer is created as synthesizer buffer. 
 *
 * If all the voices are currently in use, CreateBuffer will perform voice-stealing 
 * to fulfill the request. Note that voice stealing may fail if all voices that are 
 * currently in use are set to HAWOSEP_MAXIMUM priority.
 *
 * The followings are default values for a newly created buffer:
 *   - Send Routing 
 *      - for 1 channel buffer, channel is routed to Front-Left and Front-Right.
 *      - for 2 channel buffer, channel 0 is routed Front-Left, channel 1 is routed Front-Right  
 *   - Send Levels are set to 0 (infinite attenuation)
 *   - Channel Volume is set to 0xFFFFFFFF (no attenuation)
 *   - No notification.
 *   - StartLoopOffset is set to 0.
 *   - EndLoopOffset and EndOffset are set to pConfig->mapdata.dwSize.
 *   - No loop.
 *   - Buffer is in the stop state. 
 *   - Play position is set to 0.
 *
 * CALL LEVELS: PASSIVE.
 * 
 * @param   pConfig
 * A pointers to configuration structures containing
 * information about how the clients should be opened.
 *
 * @param   pCallback
 * A pointer to the callback function.
 *
 * @param   dwFlags 
 * HABUF_SYNTH_BUFFER bit when set indicates synthesizer buffer. 
 * HABUF_ALLOC_USER_MEM bit when set indicates caller allocate sound data memory buffer.  
 * 
 * @param   phHandle
 * A pointer to a memory address where the token of new client identifier
 * should be placed.
 *
 * @return
 * Returns SEGA_SUCCESS if successful.  Otherwise, returns an appropriate 
 * error code.
 * 
 * @retval  SEGAERR_BAD_POINTER if either the pConfig or phHandle pointers are
 * NULL.
 * @retval  SEGAERR_OUT_OF_MEMORY if buffer size requested in pConfig cannot be
 * allocated. 
 * @retval  SEGAERR_BAD_CONFIG if the device can't support the configuration
 * requested.
 * @retval  SEGAERR_NO_RESOURCES if no resources are available for creating
 * the device.  Generally, increasing the client's priority
 * will allow a subsequent creation request to succeed.
 */
SEGASTATUS SEGAAPI_CreateBuffer(HAWOSEBUFFERCONFIG * pConfig,
  HAWOSEGABUFFERCALLBACK pCallback,
  CTDWORD dwFlags,
  CTHANDLE *phHandle);


/**
 * Destroys the buffer previously created with CreateBuffer().
 * This will free all the resources previously allocated to this buffer.
 *
 * This function will stop the buffer if it is not at the stop state before
 * freeing all the resources.
 *
 * CALL LEVELS: PASSIVE.
 *
 * @param hHandle
 * An opaque identifier obtained from CreateBuffer. 
 *
 * @return
 * Returns SEGA_SUCCESS if successful.  Otherwise, returns an appropriate 
 * error code.
 */
SEGASTATUS SEGAAPI_DestroyBuffer(CTHANDLE hHandle);





/***********************************************************
 * @section 
 * API to control effect slots and effects parameters.
 * 
 * Refers to the EAX4 Programmer's Guide for the details of
 * controlling FX Slots and controlling Effect parameters.
 * 
 * Only the EAX4 FX Slots and Effect Parameters property controls
 * will be implemented for this project.
 *
 * Need to add property to switch the FX returns for 
 * FXSlot2 and FXSlot3 when non-reverb is loaded to these slots
 * as per SEGA request.
 */
 
/**
 * Sets global EAX property.
 *
 * This function sets the EAX4 FX Slots and Effect Parameters property 
 * controls as defined in EAX4 EAX4 Programmer's Guide.
 *
 * @param guid
 * EAX Object GUID
 *
 * @param ulProperty
 * Property enumeration value of each object
 *
 * @param pData
 * A pointer to a memory address where the data will be accessed 
 *
 * @param ulDataSize
 * An unsigned integer indicating the size of the data pointed to by the pData.
 *
 * @return
 * Returns TRUE if successful.  Otherwise, returns FALSE.
 */ 
CTBOOL SEGAAPI_SetGlobalEAXProperty(GUID *guid, unsigned long ulProperty, void *pData, unsigned long ulDataSize);

/**
 * Gets global EAX property.
 *
 * This function gets the EAX4 FX Slots and Effect Parameters property 
 * controls as defined in EAX4 EAX4 Programmer's Guide.
 *
 * @param guid
 * EAX Object GUID
 *
 * @param ulProperty
 * Property enumeration value of each object
 *
 * @param pData
 * A pointer to a memory address where the data will be accessed 
 *
 * @param ulDataSize
 * An unsigned integer indicating the size of the data pointed to by the pData.
 *
 * @return
 * Returns TRUE if successful.  Otherwise, returns FALSE.
 */ 
CTBOOL SEGAAPI_GetGlobalEAXProperty(GUID *guid, unsigned long ulProperty, void *pData, unsigned long ulDataSize);


/***********************************************************
 * @section 
 * API to control SPDIF Output channel status, sampling rate
 * and output routing matrix.
 * 
 */

/**
 * Sets SPDIF Out channel status.
 *
 * @param dwChannelStatus
 * Channel Status
 *
 * @param dwExtChannelStatus
 * Extended Channel Status
 *
 * @return
 * Returns SEGA_SUCCESS if successful.  Otherwise, returns an appropriate 
 * error code.
 */
SEGASTATUS SEGAAPI_SetSPDIFOutChannelStatus(
 CTDWORD dwChannelStatus,
 CTDWORD dwExtChannelStatus);

/**
 * Gets SPDIF Out channel status.
 *
 * @param pdwChannelStatus
 * Pointer to address where Channel Status is returned to.
 *
 * @param pdwExtChannelStatus
 * Pointer to address where Extended Channel Status is returned to.
 *
 * @return
 * Returns SEGA_SUCCESS if successful.  Otherwise, returns an appropriate 
 * error code.
 */
SEGASTATUS SEGAAPI_GetSPDIFOutChannelStatus(
 CTDWORD *pdwChannelStatus,
 CTDWORD *pdwExtChannelStatus);


/**
 * Sets the SPDIF Out sampling rate.  This function also updates the
 * SPDIF-Out channel status to reflect the correct sampling rate.
 * The default SPDIF Out sampling rate is 48KHz.
 *
 * @param dwSamplingRate
 * Sampling rate enumeration type
 *
 * @return
 * Returns SEGA_SUCCESS if successful.  Otherwise, returns an appropriate 
 * error code.
 */
SEGASTATUS SEGAAPI_SetSPDIFOutSampleRate(HASPDIFOUTRATE dwSamplingRate);

/**
 * Gets SPDIF Out sampling rate
 *
 * @return 
 * Returns sampling rate enum.
 * Note that returned value is set to HASPDIFOUT_48KHZ if error invoked this function. 
 */
HASPDIFOUTRATE SEGAAPI_GetSPDIFOutSampleRate(void);


/**
 * Sets SPDIF Out channel routing.
 *
 * @param   dwChannel
 *  The channel to route.  
 *  0 for Left channel, 1 for right channel.
 *
 * @param   dwSource
 *  The source to which the channel should be received signal from.
 *  If the channel doesn't need to be connected to anything,
 *  specify HA_UNUSED_PORT. 
 *  HA_FXSLOTx_PORT is not a valid source.  Routes from these ports 
 *  will return failure.
 *
 *  See HAROUTING for the details of the source enumeration list.
 *
 * @return
 * Returns SEGA_SUCCESS if successful.  Otherwise, returns an appropriate 
 * error code.
 */
SEGASTATUS SEGAAPI_SetSPDIFOutChannelRouting(
  CTDWORD dwChannel,
  HAROUTING dwSource);

 /**
 * Gets SPDIF Out channel routing
 *
 * @param   dwChannel
 *  The channel to route.  
 *  0 for Left channel, 1 for right channel.
 *
 * @return  
 *  The source to which the channel is received signal from.
  * Note that returned value is set to HA_UNUSED_PORT if error invoked this function.  
 *
 *  See HAROUTING for the details of the source enumeration list.
 */
HAROUTING SEGAAPI_GetSPDIFOutChannelRouting(CTDWORD dwChannel);



/***********************************************************
 * @section 
 * API to control global inputs and outputs volume.
 * For outputs, these volume controls are post-routing.
 */

/**
 * Sets the volume to a specific linear value.  Volumes are specified
 * as fractional fixed-point linear values between HAWOSEVOL_MAX (0xFFFFFFFF)
 * and 0x0.  Note that only the upper word is effective.
 *
 * @param dwPhysIO
 * The Physical IO being addressed.
 *
 * @param dwVolume
 * The new volume level.
 *
 * @return
 * Returns SEGA_SUCCESS if successful.  Otherwise, returns an appropriate 
 * error code.
 */
SEGASTATUS SEGAAPI_SetIOVolume(HAPHYSICALIO dwPhysIO, CTDWORD dwVolume);

/**
 * Returns the current volume level for the requested physical IO.
 *
 * @param dwPhysIO
 * The Physical IO being addressed.
 *
 * @return  
 * The current volume.
 * Note that returned value is set to 0xffffffff if error invoked this function.  
 */
CTDWORD SEGAAPI_GetIOVolume(HAPHYSICALIO dwPhysIO);

/**
 * Sets the last status code manually.
 *
 * Typically, this is use to reset the last status code.
 * Note that the last status code will be reset whenever a function is invoked.
 *
 * @param LastStatus
 * The last status code to change to.
 *
 * @return  
 * None.  
 */
void SEGAAPI_SetLastStatus(SEGASTATUS LastStatus);


/**
 * Returns the last status code for the function that just invoked.
 * The last status code will be reset whenever a function is invoked.
 * Therefore, The last status code should be checked immediately after a function
 * is invoked.  
 *
 * For functions that return SEGASTATUS, caller can check the return code 
 * immediately without needing to call GetLastStatus function.   
 *
 * @return  
 * The SEGASTATUS code.  
 */
SEGASTATUS SEGAAPI_GetLastStatus(void);


/**
 * Resets the driver to its default states.
 * 
 * This includes but not limited to the followings:
 *  - Stop and destroy all the currently playing buffers. All previous buffer 
 *    handles are no longer valid after returning from this call.
 *  - Resets all volume levels to its default.
 *  - Resets EAX property values to their defaults.
 *  - Resets SPDIF Out sampling rate and routing to its default. 
 *   
 *
 * @return  
 * The SEGASTATUS code.  
 */
SEGASTATUS SEGAAPI_Reset(void);


/**
 * Initializes the SEGAAPI Library.
 * 
 * This must be the first function to call before using any of the SEGAAPI functions.
 *   
 *
 * @return  
 * The SEGASTATUS code.  
 */
SEGASTATUS SEGAAPI_Init(void);


/**
 * Exits from the SEGAAPI Library.
 * 
 * This function performs cleanup on the SEGAAPI Library.  
 * It must be the last function to call.  
 *   
 *
 * @return  
 * The SEGASTATUS code.  
 */
SEGASTATUS SEGAAPI_Exit(void);


#ifdef __cplusplus
}
#endif

#endif  /* __SEGAAPI_H */

