#pragma once

#include <windows.h>
#include <mfidl.h>
#include <mfapi.h>
#include <mferror.h>
#include <atlbase.h>
#include <string>

#include <assert.h>

#include "GothamAttributes.h"
#include "GothamSinkUtils.h"
#include "GothamSessionCall.h"
#include "GothamWebRTC.h"

class CGmStreamSink;

enum GmFlushState
{
	DropSamples = 0,
	WriteSamples
};

// IGmMarker:
// Custom interface for handling IMFStreamSink::PlaceMarker calls asynchronously.

// A marker consists of a marker type, marker data, and context data.
// By defining this interface, we can store the marker data inside an IUnknown object
// and keep that object on the same queue that holds the media samples. This is
// useful because samples and markers must be serialized. That is, we cannot respond
// to a marker until we have processed all of the samples that came before it.

// Note that IMarker is not a standard Media Foundation interface.
MIDL_INTERFACE("3AC82233-933C-43a9-AF3D-ADC94EABF406")
IGmMarker : public IUnknown
{
	virtual STDMETHODIMP GetMarkerType(MFSTREAMSINK_MARKER_TYPE *pType) = 0;
	virtual STDMETHODIMP GetMarkerValue(PROPVARIANT *pvar) = 0;
	virtual STDMETHODIMP GetContext(PROPVARIANT *pvar) = 0;
};

class CGmSink 
	: public IMFFinalizableMediaSink
	, public IMFClockStateSink
	, public CGmAttributes<>
	, public IGmSessionWebRTC
{

	friend class CGmStreamSink;

public:
	// Static method to create the object.
	static HRESULT CreateInstance(REFIID iid, void **ppSink);

	// IUnknown
	STDMETHODIMP QueryInterface(REFIID iid, void** ppv);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	// IMFMediaSink methods
	STDMETHODIMP GetCharacteristics(DWORD *pdwCharacteristics);

	STDMETHODIMP AddStreamSink(
		/* [in] */ DWORD dwStreamSinkIdentifier,
		/* [in] */ IMFMediaType *pMediaType,
		/* [out] */ IMFStreamSink **ppStreamSink);

	STDMETHODIMP RemoveStreamSink(DWORD dwStreamSinkIdentifier);
	STDMETHODIMP GetStreamSinkCount(DWORD *pcStreamSinkCount);
	STDMETHODIMP GetStreamSinkByIndex(DWORD dwIndex, IMFStreamSink **ppStreamSink);
	STDMETHODIMP GetStreamSinkById(DWORD dwIdentifier, IMFStreamSink **ppStreamSink);
	STDMETHODIMP SetPresentationClock(IMFPresentationClock *pPresentationClock);
	STDMETHODIMP GetPresentationClock(IMFPresentationClock **ppPresentationClock);
	STDMETHODIMP Shutdown();

	// IMFFinalizableMediaSink methods
	STDMETHODIMP BeginFinalize(IMFAsyncCallback *pCallback, IUnknown *punkState);
	STDMETHODIMP EndFinalize(IMFAsyncResult *pResult);

	// IMFClockStateSink methods
	STDMETHODIMP OnClockStart(MFTIME hnsSystemTime, LONGLONG llClockStartOffset);
	STDMETHODIMP OnClockStop(MFTIME hnsSystemTime);
	STDMETHODIMP OnClockPause(MFTIME hnsSystemTime);
	STDMETHODIMP OnClockRestart(MFTIME hnsSystemTime);
	STDMETHODIMP OnClockSetRate(MFTIME hnsSystemTime, float flRate);

	// CGmAttributes methods
	STDMETHODIMP GetUINT32(REFGUID guidKey, UINT32* punValue);
	STDMETHODIMP SetUINT32(REFGUID guidKey, UINT32 unValue);
	STDMETHODIMP GetString(REFGUID guidKey, LPWSTR pwszValue, UINT32 cchBufSize, UINT32* pcchLength);
	STDMETHODIMP SetString(REFGUID guidKey, LPCWSTR wszValue);

	// IGmSessionWebRTC methods
	STDMETHODIMP Test();

private:

	CGmSink();
	virtual ~CGmSink();

	HRESULT Initialize();
	
	HRESULT CheckShutdown() const
	{
		if (m_IsShutdown)
		{
			return MF_E_SHUTDOWN;
		}
		else
		{
			return S_OK;
		}
	}


	long                        m_nRefCount;                // reference count
	CRITICAL_SECTION            m_critSec;                  // critical section for thread safety

	BOOL                        m_IsShutdown;               // Flag to indicate if Shutdown() method was called.

	CGmStreamSink               *m_pStream;                 // Byte stream
	IMFPresentationClock        *m_pClock;                  // Presentation clock.
	CComPtr<GmSessionCall>      m_objCall;						// Doubango call session
};

//////////////////////////////////////////////////////////////////////////
//  AsyncCallback [template]
//
//  Description:
//  Helper class that routes IMFAsyncCallback::Invoke calls to a class
//  method on the parent class.
//
//  Usage:
//  Add this class as a member variable. In the parent class constructor,
//  initialize the AsyncCallback class like this:
//      m_cb(this, &CYourClass::OnInvoke)
//  where
//      m_cb       = AsyncCallback object
//      CYourClass = parent class
//      OnInvoke   = Method in the parent class to receive Invoke calls.
//
//  The parent's OnInvoke method (you can name it anything you like) must
//  have a signature that matches the InvokeFn typedef below.
//////////////////////////////////////////////////////////////////////////

// T: Type of the parent object
template<class T>
class GmAsyncCallback : public IMFAsyncCallback
{
public:
	typedef HRESULT(T::*InvokeFn)(IMFAsyncResult *pAsyncResult);

	GmAsyncCallback(T *pParent, InvokeFn fn) : m_pParent(pParent), m_pInvokeFn(fn)
	{
	}

	// IUnknown
	STDMETHODIMP QueryInterface(REFIID iid, void** ppv)
	{
		if (!ppv)
		{
			return E_POINTER;
		}
		if (iid == __uuidof(IUnknown))
		{
			*ppv = static_cast<IUnknown*>(static_cast<IMFAsyncCallback*>(this));
		}
		else if (iid == __uuidof(IMFAsyncCallback))
		{
			*ppv = static_cast<IMFAsyncCallback*>(this);
		}
		else
		{
			*ppv = NULL;
			return E_NOINTERFACE;
		}
		AddRef();
		return S_OK;
	}
	STDMETHODIMP_(ULONG) AddRef() {
		// Delegate to parent class.
		return m_pParent->AddRef();
	}
	STDMETHODIMP_(ULONG) Release() {
		// Delegate to parent class.
		return m_pParent->Release();
	}


	// IMFAsyncCallback methods
	STDMETHODIMP GetParameters(DWORD*, DWORD*)
	{
		// Implementation of this method is optional.
		return E_NOTIMPL;
	}

	STDMETHODIMP Invoke(IMFAsyncResult* pAsyncResult)
	{
		return (m_pParent->*m_pInvokeFn)(pAsyncResult);
	}

	T *m_pParent;
	InvokeFn m_pInvokeFn;
};

class CGmStreamSink 
	: public IMFStreamSink
	, public IMFMediaTypeHandler
	, public IGmStreamWebRTC
	, public CGmAttributes<>
{
	friend class CGmSink;

public:
	// IUnknown
	STDMETHODIMP QueryInterface(REFIID iid, void** ppv);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	// IMFMediaEventGenerator
	STDMETHODIMP BeginGetEvent(IMFAsyncCallback* pCallback, IUnknown* punkState);
	STDMETHODIMP EndGetEvent(IMFAsyncResult* pResult, IMFMediaEvent** ppEvent);
	STDMETHODIMP GetEvent(DWORD dwFlags, IMFMediaEvent** ppEvent);
	STDMETHODIMP QueueEvent(MediaEventType met, REFGUID guidExtendedType, HRESULT hrStatus, const PROPVARIANT* pvValue);

	// IMFStreamSink
	STDMETHODIMP GetMediaSink(IMFMediaSink **ppMediaSink);
	STDMETHODIMP GetIdentifier(DWORD *pdwIdentifier);
	STDMETHODIMP GetMediaTypeHandler(IMFMediaTypeHandler **ppHandler);
	STDMETHODIMP ProcessSample(IMFSample *pSample);

	STDMETHODIMP PlaceMarker(
		/* [in] */ MFSTREAMSINK_MARKER_TYPE eMarkerType,
		/* [in] */ const PROPVARIANT *pvarMarkerValue,
		/* [in] */ const PROPVARIANT *pvarContextValue);

	STDMETHODIMP Flush();

	// IMFMediaTypeHandler
	STDMETHODIMP IsMediaTypeSupported(IMFMediaType *pMediaType, IMFMediaType **ppMediaType);
	STDMETHODIMP GetMediaTypeCount(DWORD *pdwTypeCount);
	STDMETHODIMP GetMediaTypeByIndex(DWORD dwIndex, IMFMediaType **ppType);
	STDMETHODIMP SetCurrentMediaType(IMFMediaType *pMediaType);
	STDMETHODIMP GetCurrentMediaType(IMFMediaType **ppMediaType);
	STDMETHODIMP GetMajorType(GUID *pguidMajorType);

	// CGmAttributes methods
	STDMETHODIMP SetString(REFGUID guidKey, LPCWSTR wszValue);

	// IGmStreamWebRTC methods
	STDMETHODIMP Test();


private:

	// State enum: Defines the current state of the stream.
	enum GmState
	{
		State_TypeNotSet = 0,    // No media type is set
		State_Ready,             // Media type is set, Start has never been called.
		State_Started,
		State_Stopped,
		State_Paused,
		State_Finalized,

		State_Count = State_Finalized + 1    // Number of states
	};

	// StreamOperation: Defines various operations that can be performed on the stream.
	enum GmStreamOperation
	{
		OpSetMediaType = 0,
		OpStart,
		OpRestart,
		OpPause,
		OpStop,
		OpProcessSample,
		OpPlaceMarker,
		OpFinalize,

		Op_Count = OpFinalize + 1  // Number of operations
	};

	// CAsyncOperation:
	// Used to queue asynchronous operations. When we call MFPutWorkItem, we use this
	// object for the callback state (pState). Then, when the callback is invoked,
	// we can use the object to determine which asynchronous operation to perform.

	class CGmAsyncOperation : public IUnknown
	{
	public:
		CGmAsyncOperation(GmStreamOperation op);

		GmStreamOperation m_op;   // The operation to perform.

		// IUnknown methods.
		STDMETHODIMP QueryInterface(REFIID iid, void** ppv);
		STDMETHODIMP_(ULONG) AddRef();
		STDMETHODIMP_(ULONG) Release();

	private:
		long    m_nRefCount;
		virtual ~CGmAsyncOperation();
	};


	// ValidStateMatrix: Defines a look-up table that says which operations
	// are valid from which states.
	static BOOL ValidStateMatrix[State_Count][Op_Count];


	CGmStreamSink();
	virtual ~CGmStreamSink();

	HRESULT Initialize(CGmSink *pParent);

	HRESULT CheckShutdown() const
	{
		if (m_IsShutdown)
		{
			return MF_E_SHUTDOWN;
		}
		else
		{
			return S_OK;
		}
	}


	HRESULT     Start(MFTIME start);
	HRESULT     Restart();
	HRESULT     Stop();
	HRESULT     Pause();
	HRESULT     Finalize(IMFAsyncCallback *pCallback, IUnknown *punkState);
	HRESULT     Shutdown();

	HRESULT     ValidateOperation(GmStreamOperation op);

	HRESULT     QueueAsyncOperation(GmStreamOperation op);

	HRESULT     OnDispatchWorkItem(IMFAsyncResult* pAsyncResult);
	HRESULT     DispatchProcessSample(CGmAsyncOperation* pOp);
	HRESULT     DispatchFinalize(CGmAsyncOperation* pOp);

	HRESULT     ProcessSamplesFromQueue(GmFlushState bFlushData);
	HRESULT     SendSampleOverNetwork(IMFSample *pSample);
	HRESULT     SendMarkerEvent(IGmMarker *pMarker, GmFlushState bFlushData);


	bool						m_bVideo;					// whether it's a video stream (otherwise it's audio)
	long                        m_nRefCount;                // reference count
	CRITICAL_SECTION            m_critSec;                  // critical section for thread safety

	GmState                      m_state;
	BOOL                        m_IsShutdown;               // Flag to indicate if Shutdown() method was called.

	DWORD                       m_WorkQueueId;              // ID of the work queue for asynchronous operations.
	GmAsyncCallback<CGmStreamSink>   m_WorkQueueCB;              // Callback for the work queue.

	MFTIME                      m_StartTime;                // Presentation time when the clock started.
	DWORD                       m_cbDataWritten;            // How many bytes we have written so far. (FIXME: remove)

	CGmSink                    *m_pSink;                   // Parent media sink

	IMFMediaEventQueue          *m_pEventQueue;             // Event queue
	IMFMediaType                *m_pCurrentType;

	ComPtrList<IUnknown>        m_SampleQueue;              // Queue to hold samples and markers.
	// Applies to: ProcessSample, PlaceMarker, BeginFinalize

	IMFAsyncResult              *m_pFinalizeResult;         // Result object for Finalize operation.
};


// Holds marker information for IMFStreamSink::PlaceMarker

class CGmMarker : public IGmMarker
{
public:
	static HRESULT Create(
		MFSTREAMSINK_MARKER_TYPE eMarkerType,
		const PROPVARIANT* pvarMarkerValue,
		const PROPVARIANT* pvarContextValue,
		IGmMarker **ppMarker
		);

	// IUnknown methods.
	STDMETHODIMP QueryInterface(REFIID iid, void** ppv);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	STDMETHODIMP GetMarkerType(MFSTREAMSINK_MARKER_TYPE *pType);
	STDMETHODIMP GetMarkerValue(PROPVARIANT *pvar);
	STDMETHODIMP GetContext(PROPVARIANT *pvar);

protected:
	MFSTREAMSINK_MARKER_TYPE m_eMarkerType;
	PROPVARIANT m_varMarkerValue;
	PROPVARIANT m_varContextValue;

private:
	long    m_nRefCount;

	CGmMarker(MFSTREAMSINK_MARKER_TYPE eMarkerType);
	virtual ~CGmMarker();
};