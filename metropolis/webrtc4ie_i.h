

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0500 */
/* at Tue Jun 03 02:50:27 2014
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


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __webrtc4ie_i_h__
#define __webrtc4ie_i_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ISessionDescription_FWD_DEFINED__
#define __ISessionDescription_FWD_DEFINED__
typedef interface ISessionDescription ISessionDescription;
#endif 	/* __ISessionDescription_FWD_DEFINED__ */


#ifndef __IPeerConnection_FWD_DEFINED__
#define __IPeerConnection_FWD_DEFINED__
typedef interface IPeerConnection IPeerConnection;
#endif 	/* __IPeerConnection_FWD_DEFINED__ */


#ifndef __IVideoDisplay_FWD_DEFINED__
#define __IVideoDisplay_FWD_DEFINED__
typedef interface IVideoDisplay IVideoDisplay;
#endif 	/* __IVideoDisplay_FWD_DEFINED__ */


#ifndef __INetTransport_FWD_DEFINED__
#define __INetTransport_FWD_DEFINED__
typedef interface INetTransport INetTransport;
#endif 	/* __INetTransport_FWD_DEFINED__ */


#ifndef __ILooper_FWD_DEFINED__
#define __ILooper_FWD_DEFINED__
typedef interface ILooper ILooper;
#endif 	/* __ILooper_FWD_DEFINED__ */


#ifndef __IPluginInstance_FWD_DEFINED__
#define __IPluginInstance_FWD_DEFINED__
typedef interface IPluginInstance IPluginInstance;
#endif 	/* __IPluginInstance_FWD_DEFINED__ */


#ifndef __SessionDescription_FWD_DEFINED__
#define __SessionDescription_FWD_DEFINED__

#ifdef __cplusplus
typedef class SessionDescription SessionDescription;
#else
typedef struct SessionDescription SessionDescription;
#endif /* __cplusplus */

#endif 	/* __SessionDescription_FWD_DEFINED__ */


#ifndef ___IPeerConnectionEvents_FWD_DEFINED__
#define ___IPeerConnectionEvents_FWD_DEFINED__
typedef interface _IPeerConnectionEvents _IPeerConnectionEvents;
#endif 	/* ___IPeerConnectionEvents_FWD_DEFINED__ */


#ifndef __PeerConnection_FWD_DEFINED__
#define __PeerConnection_FWD_DEFINED__

#ifdef __cplusplus
typedef class PeerConnection PeerConnection;
#else
typedef struct PeerConnection PeerConnection;
#endif /* __cplusplus */

#endif 	/* __PeerConnection_FWD_DEFINED__ */


#ifndef ___IVideoDisplayEvents_FWD_DEFINED__
#define ___IVideoDisplayEvents_FWD_DEFINED__
typedef interface _IVideoDisplayEvents _IVideoDisplayEvents;
#endif 	/* ___IVideoDisplayEvents_FWD_DEFINED__ */


#ifndef __VideoDisplay_FWD_DEFINED__
#define __VideoDisplay_FWD_DEFINED__

#ifdef __cplusplus
typedef class VideoDisplay VideoDisplay;
#else
typedef struct VideoDisplay VideoDisplay;
#endif /* __cplusplus */

#endif 	/* __VideoDisplay_FWD_DEFINED__ */


#ifndef ___INetTransportEvents_FWD_DEFINED__
#define ___INetTransportEvents_FWD_DEFINED__
typedef interface _INetTransportEvents _INetTransportEvents;
#endif 	/* ___INetTransportEvents_FWD_DEFINED__ */


#ifndef __NetTransport_FWD_DEFINED__
#define __NetTransport_FWD_DEFINED__

#ifdef __cplusplus
typedef class NetTransport NetTransport;
#else
typedef struct NetTransport NetTransport;
#endif /* __cplusplus */

#endif 	/* __NetTransport_FWD_DEFINED__ */


#ifndef ___ILooperEvents_FWD_DEFINED__
#define ___ILooperEvents_FWD_DEFINED__
typedef interface _ILooperEvents _ILooperEvents;
#endif 	/* ___ILooperEvents_FWD_DEFINED__ */


#ifndef __Looper_FWD_DEFINED__
#define __Looper_FWD_DEFINED__

#ifdef __cplusplus
typedef class Looper Looper;
#else
typedef struct Looper Looper;
#endif /* __cplusplus */

#endif 	/* __Looper_FWD_DEFINED__ */


#ifndef __pluginInstance_FWD_DEFINED__
#define __pluginInstance_FWD_DEFINED__

#ifdef __cplusplus
typedef class pluginInstance pluginInstance;
#else
typedef struct pluginInstance pluginInstance;
#endif /* __cplusplus */

#endif 	/* __pluginInstance_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __ISessionDescription_INTERFACE_DEFINED__
#define __ISessionDescription_INTERFACE_DEFINED__

/* interface ISessionDescription */
/* [unique][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_ISessionDescription;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("59A83878-17F1-4093-AAFD-FC5EFD15DDD3")
    ISessionDescription : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE toSdp( 
            /* [retval][out] */ BSTR *sdp) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Init( 
            BSTR sdp) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE addCandidate( 
            /* [in] */ BSTR media,
            /* [in] */ BSTR candidate) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISessionDescriptionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISessionDescription * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISessionDescription * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISessionDescription * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISessionDescription * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISessionDescription * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISessionDescription * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISessionDescription * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *toSdp )( 
            ISessionDescription * This,
            /* [retval][out] */ BSTR *sdp);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Init )( 
            ISessionDescription * This,
            BSTR sdp);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *addCandidate )( 
            ISessionDescription * This,
            /* [in] */ BSTR media,
            /* [in] */ BSTR candidate);
        
        END_INTERFACE
    } ISessionDescriptionVtbl;

    interface ISessionDescription
    {
        CONST_VTBL struct ISessionDescriptionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISessionDescription_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ISessionDescription_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ISessionDescription_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ISessionDescription_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define ISessionDescription_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define ISessionDescription_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define ISessionDescription_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define ISessionDescription_toSdp(This,sdp)	\
    ( (This)->lpVtbl -> toSdp(This,sdp) ) 

#define ISessionDescription_Init(This,sdp)	\
    ( (This)->lpVtbl -> Init(This,sdp) ) 

#define ISessionDescription_addCandidate(This,media,candidate)	\
    ( (This)->lpVtbl -> addCandidate(This,media,candidate) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ISessionDescription_INTERFACE_DEFINED__ */


#ifndef __IPeerConnection_INTERFACE_DEFINED__
#define __IPeerConnection_INTERFACE_DEFINED__

/* interface IPeerConnection */
/* [unique][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IPeerConnection;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AEC9C14A-6FB5-4159-9F88-435B92854937")
    IPeerConnection : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE close( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE createOffer( 
            /* [in] */ VARIANT_BOOL has_audio,
            /* [in] */ VARIANT_BOOL has_video,
            /* [retval][out] */ BSTR *sdp) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE createAnswer( 
            /* [in] */ VARIANT_BOOL has_audio,
            /* [in] */ VARIANT_BOOL has_video,
            /* [retval][out] */ BSTR *sdp) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE createOfferEx( 
            /* [in] */ VARIANT_BOOL has_audio,
            /* [in] */ VARIANT_BOOL has_video,
            /* [in] */ VARIANT_BOOL has_bfcpvideo,
            /* [retval][out] */ BSTR *sdp) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE createAnswerEx( 
            /* [in] */ VARIANT_BOOL has_audio,
            /* [in] */ VARIANT_BOOL has_video,
            /* [in] */ VARIANT_BOOL has_bfcpvideo,
            /* [retval][out] */ BSTR *sdp) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE startIce( 
            /* [in] */ SHORT IceOptions,
            /* [in] */ LONGLONG looper) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE startMedia( void) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_localDescription( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_remoteDescription( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE setLocalDescription( 
            /* [in] */ USHORT action,
            /* [in] */ BSTR desc) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE setRemoteDescription( 
            /* [in] */ USHORT action,
            /* [in] */ BSTR desc) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE processContent( 
            /* [in] */ BSTR req_name,
            /* [in] */ BSTR content_type,
            /* [in] */ BSTR content_ptr,
            /* [in] */ INT content_size) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE sendDTMF( 
            /* [in] */ USHORT digit) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_readyState( 
            /* [retval][out] */ USHORT *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_iceState( 
            /* [retval][out] */ USHORT *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_remoteVideo( 
            /* [retval][out] */ LONGLONG *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_remoteVideo( 
            /* [in] */ LONGLONG newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_localVideo( 
            /* [retval][out] */ LONGLONG *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_localVideo( 
            /* [in] */ LONGLONG newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_localScreencast( 
            /* [retval][out] */ LONGLONG *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_localScreencast( 
            /* [in] */ LONGLONG newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_srcScreencast( 
            /* [retval][out] */ LONGLONG *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_srcScreencast( 
            /* [in] */ LONGLONG newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_version( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_fullScreen( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_fullScreen( 
            /* [in] */ VARIANT_BOOL newVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Init( 
            /* [in] */ BSTR desc) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE StartDebug( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE StopDebug( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPeerConnectionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPeerConnection * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPeerConnection * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPeerConnection * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPeerConnection * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPeerConnection * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPeerConnection * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPeerConnection * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *close )( 
            IPeerConnection * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *createOffer )( 
            IPeerConnection * This,
            /* [in] */ VARIANT_BOOL has_audio,
            /* [in] */ VARIANT_BOOL has_video,
            /* [retval][out] */ BSTR *sdp);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *createAnswer )( 
            IPeerConnection * This,
            /* [in] */ VARIANT_BOOL has_audio,
            /* [in] */ VARIANT_BOOL has_video,
            /* [retval][out] */ BSTR *sdp);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *createOfferEx )( 
            IPeerConnection * This,
            /* [in] */ VARIANT_BOOL has_audio,
            /* [in] */ VARIANT_BOOL has_video,
            /* [in] */ VARIANT_BOOL has_bfcpvideo,
            /* [retval][out] */ BSTR *sdp);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *createAnswerEx )( 
            IPeerConnection * This,
            /* [in] */ VARIANT_BOOL has_audio,
            /* [in] */ VARIANT_BOOL has_video,
            /* [in] */ VARIANT_BOOL has_bfcpvideo,
            /* [retval][out] */ BSTR *sdp);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *startIce )( 
            IPeerConnection * This,
            /* [in] */ SHORT IceOptions,
            /* [in] */ LONGLONG looper);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *startMedia )( 
            IPeerConnection * This);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_localDescription )( 
            IPeerConnection * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_remoteDescription )( 
            IPeerConnection * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *setLocalDescription )( 
            IPeerConnection * This,
            /* [in] */ USHORT action,
            /* [in] */ BSTR desc);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *setRemoteDescription )( 
            IPeerConnection * This,
            /* [in] */ USHORT action,
            /* [in] */ BSTR desc);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *processContent )( 
            IPeerConnection * This,
            /* [in] */ BSTR req_name,
            /* [in] */ BSTR content_type,
            /* [in] */ BSTR content_ptr,
            /* [in] */ INT content_size);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *sendDTMF )( 
            IPeerConnection * This,
            /* [in] */ USHORT digit);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_readyState )( 
            IPeerConnection * This,
            /* [retval][out] */ USHORT *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_iceState )( 
            IPeerConnection * This,
            /* [retval][out] */ USHORT *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_remoteVideo )( 
            IPeerConnection * This,
            /* [retval][out] */ LONGLONG *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_remoteVideo )( 
            IPeerConnection * This,
            /* [in] */ LONGLONG newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_localVideo )( 
            IPeerConnection * This,
            /* [retval][out] */ LONGLONG *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_localVideo )( 
            IPeerConnection * This,
            /* [in] */ LONGLONG newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_localScreencast )( 
            IPeerConnection * This,
            /* [retval][out] */ LONGLONG *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_localScreencast )( 
            IPeerConnection * This,
            /* [in] */ LONGLONG newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_srcScreencast )( 
            IPeerConnection * This,
            /* [retval][out] */ LONGLONG *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_srcScreencast )( 
            IPeerConnection * This,
            /* [in] */ LONGLONG newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_version )( 
            IPeerConnection * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_fullScreen )( 
            IPeerConnection * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_fullScreen )( 
            IPeerConnection * This,
            /* [in] */ VARIANT_BOOL newVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Init )( 
            IPeerConnection * This,
            /* [in] */ BSTR desc);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *StartDebug )( 
            IPeerConnection * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *StopDebug )( 
            IPeerConnection * This);
        
        END_INTERFACE
    } IPeerConnectionVtbl;

    interface IPeerConnection
    {
        CONST_VTBL struct IPeerConnectionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPeerConnection_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IPeerConnection_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IPeerConnection_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IPeerConnection_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IPeerConnection_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IPeerConnection_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IPeerConnection_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IPeerConnection_close(This)	\
    ( (This)->lpVtbl -> close(This) ) 

#define IPeerConnection_createOffer(This,has_audio,has_video,sdp)	\
    ( (This)->lpVtbl -> createOffer(This,has_audio,has_video,sdp) ) 

#define IPeerConnection_createAnswer(This,has_audio,has_video,sdp)	\
    ( (This)->lpVtbl -> createAnswer(This,has_audio,has_video,sdp) ) 

#define IPeerConnection_createOfferEx(This,has_audio,has_video,has_bfcpvideo,sdp)	\
    ( (This)->lpVtbl -> createOfferEx(This,has_audio,has_video,has_bfcpvideo,sdp) ) 

#define IPeerConnection_createAnswerEx(This,has_audio,has_video,has_bfcpvideo,sdp)	\
    ( (This)->lpVtbl -> createAnswerEx(This,has_audio,has_video,has_bfcpvideo,sdp) ) 

#define IPeerConnection_startIce(This,IceOptions,looper)	\
    ( (This)->lpVtbl -> startIce(This,IceOptions,looper) ) 

#define IPeerConnection_startMedia(This)	\
    ( (This)->lpVtbl -> startMedia(This) ) 

#define IPeerConnection_get_localDescription(This,pVal)	\
    ( (This)->lpVtbl -> get_localDescription(This,pVal) ) 

#define IPeerConnection_get_remoteDescription(This,pVal)	\
    ( (This)->lpVtbl -> get_remoteDescription(This,pVal) ) 

#define IPeerConnection_setLocalDescription(This,action,desc)	\
    ( (This)->lpVtbl -> setLocalDescription(This,action,desc) ) 

#define IPeerConnection_setRemoteDescription(This,action,desc)	\
    ( (This)->lpVtbl -> setRemoteDescription(This,action,desc) ) 

#define IPeerConnection_processContent(This,req_name,content_type,content_ptr,content_size)	\
    ( (This)->lpVtbl -> processContent(This,req_name,content_type,content_ptr,content_size) ) 

#define IPeerConnection_sendDTMF(This,digit)	\
    ( (This)->lpVtbl -> sendDTMF(This,digit) ) 

#define IPeerConnection_get_readyState(This,pVal)	\
    ( (This)->lpVtbl -> get_readyState(This,pVal) ) 

#define IPeerConnection_get_iceState(This,pVal)	\
    ( (This)->lpVtbl -> get_iceState(This,pVal) ) 

#define IPeerConnection_get_remoteVideo(This,pVal)	\
    ( (This)->lpVtbl -> get_remoteVideo(This,pVal) ) 

#define IPeerConnection_put_remoteVideo(This,newVal)	\
    ( (This)->lpVtbl -> put_remoteVideo(This,newVal) ) 

#define IPeerConnection_get_localVideo(This,pVal)	\
    ( (This)->lpVtbl -> get_localVideo(This,pVal) ) 

#define IPeerConnection_put_localVideo(This,newVal)	\
    ( (This)->lpVtbl -> put_localVideo(This,newVal) ) 

#define IPeerConnection_get_localScreencast(This,pVal)	\
    ( (This)->lpVtbl -> get_localScreencast(This,pVal) ) 

#define IPeerConnection_put_localScreencast(This,newVal)	\
    ( (This)->lpVtbl -> put_localScreencast(This,newVal) ) 

#define IPeerConnection_get_srcScreencast(This,pVal)	\
    ( (This)->lpVtbl -> get_srcScreencast(This,pVal) ) 

#define IPeerConnection_put_srcScreencast(This,newVal)	\
    ( (This)->lpVtbl -> put_srcScreencast(This,newVal) ) 

#define IPeerConnection_get_version(This,pVal)	\
    ( (This)->lpVtbl -> get_version(This,pVal) ) 

#define IPeerConnection_get_fullScreen(This,pVal)	\
    ( (This)->lpVtbl -> get_fullScreen(This,pVal) ) 

#define IPeerConnection_put_fullScreen(This,newVal)	\
    ( (This)->lpVtbl -> put_fullScreen(This,newVal) ) 

#define IPeerConnection_Init(This,desc)	\
    ( (This)->lpVtbl -> Init(This,desc) ) 

#define IPeerConnection_StartDebug(This)	\
    ( (This)->lpVtbl -> StartDebug(This) ) 

#define IPeerConnection_StopDebug(This)	\
    ( (This)->lpVtbl -> StopDebug(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IPeerConnection_INTERFACE_DEFINED__ */


#ifndef __IVideoDisplay_INTERFACE_DEFINED__
#define __IVideoDisplay_INTERFACE_DEFINED__

/* interface IVideoDisplay */
/* [unique][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IVideoDisplay;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A3513050-9B95-4FA0-B3A7-1CAD92CCE3A4")
    IVideoDisplay : public IDispatch
    {
    public:
        virtual /* [id][requestedit][bindable][propput] */ HRESULT STDMETHODCALLTYPE put_AutoSize( 
            /* [in] */ VARIANT_BOOL vbool) = 0;
        
        virtual /* [id][requestedit][bindable][propget] */ HRESULT STDMETHODCALLTYPE get_AutoSize( 
            /* [retval][out] */ VARIANT_BOOL *pbool) = 0;
        
        virtual /* [id][requestedit][bindable][propput] */ HRESULT STDMETHODCALLTYPE put_BorderWidth( 
            /* [in] */ long width) = 0;
        
        virtual /* [id][requestedit][bindable][propget] */ HRESULT STDMETHODCALLTYPE get_BorderWidth( 
            /* [retval][out] */ long *width) = 0;
        
        virtual /* [id][requestedit][bindable][propget] */ HRESULT STDMETHODCALLTYPE get_HWND( 
            /* [retval][out] */ LONG_PTR *pHWND) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IVideoDisplayVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IVideoDisplay * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IVideoDisplay * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IVideoDisplay * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IVideoDisplay * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IVideoDisplay * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IVideoDisplay * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IVideoDisplay * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][requestedit][bindable][propput] */ HRESULT ( STDMETHODCALLTYPE *put_AutoSize )( 
            IVideoDisplay * This,
            /* [in] */ VARIANT_BOOL vbool);
        
        /* [id][requestedit][bindable][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AutoSize )( 
            IVideoDisplay * This,
            /* [retval][out] */ VARIANT_BOOL *pbool);
        
        /* [id][requestedit][bindable][propput] */ HRESULT ( STDMETHODCALLTYPE *put_BorderWidth )( 
            IVideoDisplay * This,
            /* [in] */ long width);
        
        /* [id][requestedit][bindable][propget] */ HRESULT ( STDMETHODCALLTYPE *get_BorderWidth )( 
            IVideoDisplay * This,
            /* [retval][out] */ long *width);
        
        /* [id][requestedit][bindable][propget] */ HRESULT ( STDMETHODCALLTYPE *get_HWND )( 
            IVideoDisplay * This,
            /* [retval][out] */ LONG_PTR *pHWND);
        
        END_INTERFACE
    } IVideoDisplayVtbl;

    interface IVideoDisplay
    {
        CONST_VTBL struct IVideoDisplayVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IVideoDisplay_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IVideoDisplay_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IVideoDisplay_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IVideoDisplay_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IVideoDisplay_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IVideoDisplay_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IVideoDisplay_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IVideoDisplay_put_AutoSize(This,vbool)	\
    ( (This)->lpVtbl -> put_AutoSize(This,vbool) ) 

#define IVideoDisplay_get_AutoSize(This,pbool)	\
    ( (This)->lpVtbl -> get_AutoSize(This,pbool) ) 

#define IVideoDisplay_put_BorderWidth(This,width)	\
    ( (This)->lpVtbl -> put_BorderWidth(This,width) ) 

#define IVideoDisplay_get_BorderWidth(This,width)	\
    ( (This)->lpVtbl -> get_BorderWidth(This,width) ) 

#define IVideoDisplay_get_HWND(This,pHWND)	\
    ( (This)->lpVtbl -> get_HWND(This,pHWND) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IVideoDisplay_INTERFACE_DEFINED__ */


#ifndef __INetTransport_INTERFACE_DEFINED__
#define __INetTransport_INTERFACE_DEFINED__

/* interface INetTransport */
/* [unique][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_INetTransport;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FDD25C9A-C789-400C-B57C-8860DCAAC861")
    INetTransport : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetDomain( 
            /* [in] */ BSTR domain) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Start( 
            /* [in] */ LONGLONG looper) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Stop( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SendTo( 
            /* [in] */ BSTR msg,
            /* [in] */ BSTR addr,
            /* [in] */ USHORT port) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_localIP( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_localPort( 
            /* [retval][out] */ USHORT *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_defaultDestAddr( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_defaultDestPort( 
            /* [retval][out] */ USHORT *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_version( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE StartDebug( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE StopDebug( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INetTransportVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            INetTransport * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            INetTransport * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            INetTransport * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            INetTransport * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            INetTransport * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            INetTransport * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            INetTransport * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SetDomain )( 
            INetTransport * This,
            /* [in] */ BSTR domain);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Start )( 
            INetTransport * This,
            /* [in] */ LONGLONG looper);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Stop )( 
            INetTransport * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SendTo )( 
            INetTransport * This,
            /* [in] */ BSTR msg,
            /* [in] */ BSTR addr,
            /* [in] */ USHORT port);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_localIP )( 
            INetTransport * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_localPort )( 
            INetTransport * This,
            /* [retval][out] */ USHORT *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_defaultDestAddr )( 
            INetTransport * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_defaultDestPort )( 
            INetTransport * This,
            /* [retval][out] */ USHORT *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_version )( 
            INetTransport * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *StartDebug )( 
            INetTransport * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *StopDebug )( 
            INetTransport * This);
        
        END_INTERFACE
    } INetTransportVtbl;

    interface INetTransport
    {
        CONST_VTBL struct INetTransportVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INetTransport_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define INetTransport_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define INetTransport_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define INetTransport_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define INetTransport_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define INetTransport_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define INetTransport_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define INetTransport_SetDomain(This,domain)	\
    ( (This)->lpVtbl -> SetDomain(This,domain) ) 

#define INetTransport_Start(This,looper)	\
    ( (This)->lpVtbl -> Start(This,looper) ) 

#define INetTransport_Stop(This)	\
    ( (This)->lpVtbl -> Stop(This) ) 

#define INetTransport_SendTo(This,msg,addr,port)	\
    ( (This)->lpVtbl -> SendTo(This,msg,addr,port) ) 

#define INetTransport_get_localIP(This,pVal)	\
    ( (This)->lpVtbl -> get_localIP(This,pVal) ) 

#define INetTransport_get_localPort(This,pVal)	\
    ( (This)->lpVtbl -> get_localPort(This,pVal) ) 

#define INetTransport_get_defaultDestAddr(This,pVal)	\
    ( (This)->lpVtbl -> get_defaultDestAddr(This,pVal) ) 

#define INetTransport_get_defaultDestPort(This,pVal)	\
    ( (This)->lpVtbl -> get_defaultDestPort(This,pVal) ) 

#define INetTransport_get_version(This,pVal)	\
    ( (This)->lpVtbl -> get_version(This,pVal) ) 

#define INetTransport_StartDebug(This)	\
    ( (This)->lpVtbl -> StartDebug(This) ) 

#define INetTransport_StopDebug(This)	\
    ( (This)->lpVtbl -> StopDebug(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __INetTransport_INTERFACE_DEFINED__ */


#ifndef __ILooper_INTERFACE_DEFINED__
#define __ILooper_INTERFACE_DEFINED__

/* interface ILooper */
/* [unique][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_ILooper;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("BD91785F-1414-4AD0-BF07-D5EE82CF85E6")
    ILooper : public IDispatch
    {
    public:
        virtual /* [id][requestedit][bindable][propget] */ HRESULT STDMETHODCALLTYPE get_HWND( 
            /* [retval][out] */ LONG_PTR *pHWND) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ILooperVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ILooper * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ILooper * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ILooper * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ILooper * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ILooper * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ILooper * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ILooper * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][requestedit][bindable][propget] */ HRESULT ( STDMETHODCALLTYPE *get_HWND )( 
            ILooper * This,
            /* [retval][out] */ LONG_PTR *pHWND);
        
        END_INTERFACE
    } ILooperVtbl;

    interface ILooper
    {
        CONST_VTBL struct ILooperVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ILooper_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ILooper_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ILooper_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ILooper_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define ILooper_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define ILooper_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define ILooper_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define ILooper_get_HWND(This,pHWND)	\
    ( (This)->lpVtbl -> get_HWND(This,pHWND) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ILooper_INTERFACE_DEFINED__ */


#ifndef __IPluginInstance_INTERFACE_DEFINED__
#define __IPluginInstance_INTERFACE_DEFINED__

/* interface IPluginInstance */
/* [unique][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IPluginInstance;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("ACE5665C-BA38-4935-8D73-2E3693ED6899")
    IPluginInstance : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_version( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_supportsPeerConnection( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_supportsSessionDescription( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_supportsNetTransport( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_windowHandle( 
            /* [retval][out] */ LONGLONG *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_fps( 
            /* [retval][out] */ LONG *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_fps( 
            /* [in] */ LONG newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_maxVideoSize( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_maxVideoSize( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_maxBandwidthUp( 
            /* [retval][out] */ LONG *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_maxBandwidthUp( 
            /* [in] */ LONG newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_maxBandwidthDown( 
            /* [retval][out] */ LONG *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_maxBandwidthDown( 
            /* [in] */ LONG newVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE createPeerConnection( 
            /* [retval][out] */ IDispatch **ppPeerConnection) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE createSessionDescription( 
            /* [retval][out] */ IDispatch **ppSessionDescription) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE createNetTransport( 
            /* [retval][out] */ IDispatch **ppNetTransport) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE runningApps( 
            /* [retval][out] */ BSTR *pApps) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPluginInstanceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPluginInstance * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPluginInstance * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPluginInstance * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPluginInstance * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPluginInstance * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPluginInstance * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPluginInstance * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_version )( 
            IPluginInstance * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_supportsPeerConnection )( 
            IPluginInstance * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_supportsSessionDescription )( 
            IPluginInstance * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_supportsNetTransport )( 
            IPluginInstance * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_windowHandle )( 
            IPluginInstance * This,
            /* [retval][out] */ LONGLONG *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_fps )( 
            IPluginInstance * This,
            /* [retval][out] */ LONG *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_fps )( 
            IPluginInstance * This,
            /* [in] */ LONG newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_maxVideoSize )( 
            IPluginInstance * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_maxVideoSize )( 
            IPluginInstance * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_maxBandwidthUp )( 
            IPluginInstance * This,
            /* [retval][out] */ LONG *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_maxBandwidthUp )( 
            IPluginInstance * This,
            /* [in] */ LONG newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_maxBandwidthDown )( 
            IPluginInstance * This,
            /* [retval][out] */ LONG *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_maxBandwidthDown )( 
            IPluginInstance * This,
            /* [in] */ LONG newVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *createPeerConnection )( 
            IPluginInstance * This,
            /* [retval][out] */ IDispatch **ppPeerConnection);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *createSessionDescription )( 
            IPluginInstance * This,
            /* [retval][out] */ IDispatch **ppSessionDescription);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *createNetTransport )( 
            IPluginInstance * This,
            /* [retval][out] */ IDispatch **ppNetTransport);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *runningApps )( 
            IPluginInstance * This,
            /* [retval][out] */ BSTR *pApps);
        
        END_INTERFACE
    } IPluginInstanceVtbl;

    interface IPluginInstance
    {
        CONST_VTBL struct IPluginInstanceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPluginInstance_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IPluginInstance_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IPluginInstance_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IPluginInstance_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IPluginInstance_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IPluginInstance_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IPluginInstance_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IPluginInstance_get_version(This,pVal)	\
    ( (This)->lpVtbl -> get_version(This,pVal) ) 

#define IPluginInstance_get_supportsPeerConnection(This,pVal)	\
    ( (This)->lpVtbl -> get_supportsPeerConnection(This,pVal) ) 

#define IPluginInstance_get_supportsSessionDescription(This,pVal)	\
    ( (This)->lpVtbl -> get_supportsSessionDescription(This,pVal) ) 

#define IPluginInstance_get_supportsNetTransport(This,pVal)	\
    ( (This)->lpVtbl -> get_supportsNetTransport(This,pVal) ) 

#define IPluginInstance_get_windowHandle(This,pVal)	\
    ( (This)->lpVtbl -> get_windowHandle(This,pVal) ) 

#define IPluginInstance_get_fps(This,pVal)	\
    ( (This)->lpVtbl -> get_fps(This,pVal) ) 

#define IPluginInstance_put_fps(This,newVal)	\
    ( (This)->lpVtbl -> put_fps(This,newVal) ) 

#define IPluginInstance_get_maxVideoSize(This,pVal)	\
    ( (This)->lpVtbl -> get_maxVideoSize(This,pVal) ) 

#define IPluginInstance_put_maxVideoSize(This,newVal)	\
    ( (This)->lpVtbl -> put_maxVideoSize(This,newVal) ) 

#define IPluginInstance_get_maxBandwidthUp(This,pVal)	\
    ( (This)->lpVtbl -> get_maxBandwidthUp(This,pVal) ) 

#define IPluginInstance_put_maxBandwidthUp(This,newVal)	\
    ( (This)->lpVtbl -> put_maxBandwidthUp(This,newVal) ) 

#define IPluginInstance_get_maxBandwidthDown(This,pVal)	\
    ( (This)->lpVtbl -> get_maxBandwidthDown(This,pVal) ) 

#define IPluginInstance_put_maxBandwidthDown(This,newVal)	\
    ( (This)->lpVtbl -> put_maxBandwidthDown(This,newVal) ) 

#define IPluginInstance_createPeerConnection(This,ppPeerConnection)	\
    ( (This)->lpVtbl -> createPeerConnection(This,ppPeerConnection) ) 

#define IPluginInstance_createSessionDescription(This,ppSessionDescription)	\
    ( (This)->lpVtbl -> createSessionDescription(This,ppSessionDescription) ) 

#define IPluginInstance_createNetTransport(This,ppNetTransport)	\
    ( (This)->lpVtbl -> createNetTransport(This,ppNetTransport) ) 

#define IPluginInstance_runningApps(This,pApps)	\
    ( (This)->lpVtbl -> runningApps(This,pApps) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IPluginInstance_INTERFACE_DEFINED__ */



#ifndef __webrtc4ieLib_LIBRARY_DEFINED__
#define __webrtc4ieLib_LIBRARY_DEFINED__

/* library webrtc4ieLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_webrtc4ieLib;

EXTERN_C const CLSID CLSID_SessionDescription;

#ifdef __cplusplus

class DECLSPEC_UUID("DBA9F8E2-F9FB-47CF-8797-986A69A1CA9C")
SessionDescription;
#endif

#ifndef ___IPeerConnectionEvents_DISPINTERFACE_DEFINED__
#define ___IPeerConnectionEvents_DISPINTERFACE_DEFINED__

/* dispinterface _IPeerConnectionEvents */
/* [helpstring][uuid] */ 


EXTERN_C const IID DIID__IPeerConnectionEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("B57ABD83-7530-4A0B-ACF5-1F4C2A8C1320")
    _IPeerConnectionEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct _IPeerConnectionEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            _IPeerConnectionEvents * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            _IPeerConnectionEvents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            _IPeerConnectionEvents * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            _IPeerConnectionEvents * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            _IPeerConnectionEvents * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            _IPeerConnectionEvents * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            _IPeerConnectionEvents * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } _IPeerConnectionEventsVtbl;

    interface _IPeerConnectionEvents
    {
        CONST_VTBL struct _IPeerConnectionEventsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _IPeerConnectionEvents_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define _IPeerConnectionEvents_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define _IPeerConnectionEvents_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define _IPeerConnectionEvents_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define _IPeerConnectionEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define _IPeerConnectionEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define _IPeerConnectionEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* ___IPeerConnectionEvents_DISPINTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_PeerConnection;

#ifdef __cplusplus

class DECLSPEC_UUID("56D10AD3-8F52-4AA4-854B-41F4D6F9CEA3")
PeerConnection;
#endif

#ifndef ___IVideoDisplayEvents_DISPINTERFACE_DEFINED__
#define ___IVideoDisplayEvents_DISPINTERFACE_DEFINED__

/* dispinterface _IVideoDisplayEvents */
/* [helpstring][uuid] */ 


EXTERN_C const IID DIID__IVideoDisplayEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("B5EEFDFB-4DDD-40A2-9A71-A63B5669C79C")
    _IVideoDisplayEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct _IVideoDisplayEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            _IVideoDisplayEvents * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            _IVideoDisplayEvents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            _IVideoDisplayEvents * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            _IVideoDisplayEvents * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            _IVideoDisplayEvents * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            _IVideoDisplayEvents * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            _IVideoDisplayEvents * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } _IVideoDisplayEventsVtbl;

    interface _IVideoDisplayEvents
    {
        CONST_VTBL struct _IVideoDisplayEventsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _IVideoDisplayEvents_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define _IVideoDisplayEvents_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define _IVideoDisplayEvents_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define _IVideoDisplayEvents_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define _IVideoDisplayEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define _IVideoDisplayEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define _IVideoDisplayEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* ___IVideoDisplayEvents_DISPINTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_VideoDisplay;

#ifdef __cplusplus

class DECLSPEC_UUID("5C2C407B-09D9-449B-BB83-C39B7802A684")
VideoDisplay;
#endif

#ifndef ___INetTransportEvents_DISPINTERFACE_DEFINED__
#define ___INetTransportEvents_DISPINTERFACE_DEFINED__

/* dispinterface _INetTransportEvents */
/* [helpstring][uuid] */ 


EXTERN_C const IID DIID__INetTransportEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("DC4A212A-8573-4851-A145-F7DC83C619D4")
    _INetTransportEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct _INetTransportEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            _INetTransportEvents * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            _INetTransportEvents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            _INetTransportEvents * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            _INetTransportEvents * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            _INetTransportEvents * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            _INetTransportEvents * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            _INetTransportEvents * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } _INetTransportEventsVtbl;

    interface _INetTransportEvents
    {
        CONST_VTBL struct _INetTransportEventsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _INetTransportEvents_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define _INetTransportEvents_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define _INetTransportEvents_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define _INetTransportEvents_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define _INetTransportEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define _INetTransportEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define _INetTransportEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* ___INetTransportEvents_DISPINTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_NetTransport;

#ifdef __cplusplus

class DECLSPEC_UUID("5A7D84EC-382C-4844-AB3A-9825DBE30DAE")
NetTransport;
#endif

#ifndef ___ILooperEvents_DISPINTERFACE_DEFINED__
#define ___ILooperEvents_DISPINTERFACE_DEFINED__

/* dispinterface _ILooperEvents */
/* [helpstring][uuid] */ 


EXTERN_C const IID DIID__ILooperEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("0238851B-1479-40B6-A1EB-E21B0FDF04EC")
    _ILooperEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct _ILooperEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            _ILooperEvents * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            _ILooperEvents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            _ILooperEvents * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            _ILooperEvents * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            _ILooperEvents * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            _ILooperEvents * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            _ILooperEvents * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } _ILooperEventsVtbl;

    interface _ILooperEvents
    {
        CONST_VTBL struct _ILooperEventsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _ILooperEvents_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define _ILooperEvents_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define _ILooperEvents_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define _ILooperEvents_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define _ILooperEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define _ILooperEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define _ILooperEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* ___ILooperEvents_DISPINTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_Looper;

#ifdef __cplusplus

class DECLSPEC_UUID("7082C446-54A8-4280-A18D-54143846211A")
Looper;
#endif

EXTERN_C const CLSID CLSID_pluginInstance;

#ifdef __cplusplus

class DECLSPEC_UUID("69E4A9D1-824C-40DA-9680-C7424A27B6A0")
pluginInstance;
#endif
#endif /* __webrtc4ieLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


