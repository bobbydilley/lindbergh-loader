/****************************************************************************
 * Copyright (C) 2004  Creative Technology Ltd.  All rights reserved.
 *
 ****************************************************************************
 *  File:		sapidef.h
 *
 *  This file contains the return codes definition for segaapi.
 *
 ****************************************************************************
 */


#ifndef __SEGAAPIERROR_H
#define __SEGAAPIERROR_H

typedef int   SEGASTATUS;

#define SEGA_SUCCEEDED(_x)          ((SEGASTATUS) (_x) >= 0)
#define SEGA_FAILED(_x)             ((SEGASTATUS) (_x) < 0)

#define SEGARESULT_SUCCESS(_x)      (_x)
#define SEGARESULT_FAILURE(_x)      ((1 << 31) | 0xA000 | (_x))

#define SEGA_SUCCESS                            0L


#define SEGAERR_FAIL                          SEGARESULT_FAILURE(0)
#define SEGAERR_BAD_POINTER                   SEGARESULT_FAILURE(3)
#define SEGAERR_UNSUPPORTED                   SEGARESULT_FAILURE(5)
#define SEGAERR_BAD_PARAM                     SEGARESULT_FAILURE(9)
#define SEGAERR_INVALID_CHANNEL               SEGARESULT_FAILURE(10)
#define SEGAERR_INVALID_SEND                  SEGARESULT_FAILURE(11)
#define SEGAERR_PLAYING                       SEGARESULT_FAILURE(12)
#define SEGAERR_NO_RESOURCES                  SEGARESULT_FAILURE(13)
#define SEGAERR_BAD_CONFIG                    SEGARESULT_FAILURE(14)
#define SEGAERR_BAD_HANDLE                    SEGARESULT_FAILURE(18)
#define SEGAERR_BAD_SAMPLERATE                SEGARESULT_FAILURE(28)
#define SEGAERR_OUT_OF_MEMORY                 SEGARESULT_FAILURE(31)
#define SEGAERR_INIT_FAILED                   SEGARESULT_FAILURE(39)


#endif /* __SEGAAPIERROR_H */
