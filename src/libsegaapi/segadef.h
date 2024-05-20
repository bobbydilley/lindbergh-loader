/****************************************************************************
 * Copyright (C) 2004  Creative Technology Ltd.  All rights reserved.
 *
 ****************************************************************************
 *  File:		segadef.h
 *
 *  This file contains the types definitions for segaapi.
 *
 ****************************************************************************
 */

#ifndef __CTDEF_H

#ifndef __SEGAAPITYPES_H
#define __SEGAAPITYPES_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


/*  8 bit signed value     */
typedef char                CTCHAR,     *PCTCHAR,        **PPCTCHAR;
/*  8 bit unsigned value   */
typedef unsigned char       CTBYTE,     *PCTBYTE,        **PPCTBYTE;

typedef unsigned char       CTUCHAR,     *PCTUCHAR,      **PPCTUCHAR;

/* 16 bit signed value     */
typedef short               CTSHORT,     *PCTSHORT,      **PPCTSHORT;
/* 16 bit unsigned value   */
typedef unsigned short      CTWORD,     *PCTWORD,        **PPCTWORD;

typedef unsigned short      CTUSHORT,    *PCTUSHORT,      **PPCTUSHORT;

/* 32 bit signed value     */
typedef int                 CTLONG,     *PCTLONG,        **PPCTLONG;
/* 32 bit unsigned value   */
typedef unsigned int        CTDWORD,    *PCTDWORD,       **PPCTDWORD;
typedef unsigned int 		UINT32, HRESULT;

typedef unsigned long       CTULONG,     *PCTULONG,      **PPCTULONG;

typedef int                 CTBOOL,     *PCTBOOL,        **PPCTBOOL;

typedef void *              CTHANDLE;

 /* Define basic COM types */
#ifndef GUID_DEFINED
    #define GUID_DEFINED
    typedef struct _GUID
    {
        unsigned long Data1 = 0;
        unsigned short Data2 = 0;
        unsigned short Data3 = 0;
        unsigned char Data4[8] = {0};
    } GUID;
#endif // GUID_DEFINED

#ifndef DEFINE_GUID
    #ifndef INITGUID
        #define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
                extern const GUID /*FAR*/ name
    #else
        #define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
                extern const GUID name = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
    #endif // INITGUID
#endif // DEFINE_GUID


#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* __SEGAAPITYPES_H */

#endif /* __CTDEF_H */

