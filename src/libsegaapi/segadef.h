#ifndef __CTDEF_H

#ifndef __SEGAAPITYPES_H
#define __SEGAAPITYPES_H

/* Define basic COM types */
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

#endif /* __SEGAAPITYPES_H */

#endif /* __CTDEF_H */
