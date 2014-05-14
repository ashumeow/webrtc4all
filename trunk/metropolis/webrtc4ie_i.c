

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 7.00.0500 */
/* at Wed May 14 19:22:59 2014
 */
/* Compiler settings for ..\ie\webrtc4ie.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#pragma warning( disable: 4049 )  /* more than 64k source lines */


#ifdef __cplusplus
extern "C"{
#endif 


#include <rpc.h>
#include <rpcndr.h>

#ifdef _MIDL_USE_GUIDDEF_

#ifndef INITGUID
#define INITGUID
#include <guiddef.h>
#undef INITGUID
#else
#include <guiddef.h>
#endif

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        DEFINE_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8)

#else // !_MIDL_USE_GUIDDEF_

#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        const type name = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}

#endif !_MIDL_USE_GUIDDEF_

MIDL_DEFINE_GUID(IID, IID_ISessionDescription,0x59A83878,0x17F1,0x4093,0xAA,0xFD,0xFC,0x5E,0xFD,0x15,0xDD,0xD3);


MIDL_DEFINE_GUID(IID, IID_IPeerConnection,0xAEC9C14A,0x6FB5,0x4159,0x9F,0x88,0x43,0x5B,0x92,0x85,0x49,0x37);


MIDL_DEFINE_GUID(IID, IID_IVideoDisplay,0xA3513050,0x9B95,0x4FA0,0xB3,0xA7,0x1C,0xAD,0x92,0xCC,0xE3,0xA4);


MIDL_DEFINE_GUID(IID, IID_INetTransport,0xFDD25C9A,0xC789,0x400C,0xB5,0x7C,0x88,0x60,0xDC,0xAA,0xC8,0x61);


MIDL_DEFINE_GUID(IID, IID_ILooper,0xBD91785F,0x1414,0x4AD0,0xBF,0x07,0xD5,0xEE,0x82,0xCF,0x85,0xE6);


MIDL_DEFINE_GUID(IID, LIBID_webrtc4ieLib,0xF40DFDA2,0x873B,0x449A,0xB8,0xB6,0x16,0x5D,0x57,0x29,0x1E,0x1B);


MIDL_DEFINE_GUID(CLSID, CLSID_SessionDescription,0xDBA9F8E2,0xF9FB,0x47CF,0x87,0x97,0x98,0x6A,0x69,0xA1,0xCA,0x9C);


MIDL_DEFINE_GUID(IID, DIID__IPeerConnectionEvents,0xB57ABD83,0x7530,0x4A0B,0xAC,0xF5,0x1F,0x4C,0x2A,0x8C,0x13,0x20);


MIDL_DEFINE_GUID(CLSID, CLSID_PeerConnection,0x56D10AD3,0x8F52,0x4AA4,0x85,0x4B,0x41,0xF4,0xD6,0xF9,0xCE,0xA3);


MIDL_DEFINE_GUID(IID, DIID__IVideoDisplayEvents,0xB5EEFDFB,0x4DDD,0x40A2,0x9A,0x71,0xA6,0x3B,0x56,0x69,0xC7,0x9C);


MIDL_DEFINE_GUID(CLSID, CLSID_VideoDisplay,0x5C2C407B,0x09D9,0x449B,0xBB,0x83,0xC3,0x9B,0x78,0x02,0xA6,0x84);


MIDL_DEFINE_GUID(IID, DIID__INetTransportEvents,0xDC4A212A,0x8573,0x4851,0xA1,0x45,0xF7,0xDC,0x83,0xC6,0x19,0xD4);


MIDL_DEFINE_GUID(CLSID, CLSID_NetTransport,0x5A7D84EC,0x382C,0x4844,0xAB,0x3A,0x98,0x25,0xDB,0xE3,0x0D,0xAE);


MIDL_DEFINE_GUID(IID, DIID__ILooperEvents,0x0238851B,0x1479,0x40B6,0xA1,0xEB,0xE2,0x1B,0x0F,0xDF,0x04,0xEC);


MIDL_DEFINE_GUID(CLSID, CLSID_Looper,0x7082C446,0x54A8,0x4280,0xA1,0x8D,0x54,0x14,0x38,0x46,0x21,0x1A);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



