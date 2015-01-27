#include "GothamSink.h"
#include "GothamSinkGuids.h"
#include "GothamEngine.h"

#include <aviriff.h>

#include <uuids.h>      // DirectShow GUIDs

#include "tsk_debug.h"

#pragma warning( push )
#pragma warning( disable : 4355 )  // 'this' used in base member initializer list


//-------------------------------------------------------------------
// Name: CreateGmSink
// Description: Creates an instance of the GmSink object.
//
// To use the GmSink, include this header file in an application
// and link to the library file created by this project.
//
// ppSink:  Receives a pointer to the IMFMediaSink interface. The
//          caller must release the interface.
//-------------------------------------------------------------------

HRESULT CreateGmSink(IMFMediaSink **ppSink)
{
	return CGmSink::CreateInstance(IID_IMFMediaSink, (void**)ppSink);
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
// CGmSink class. - Implements the media sink.
//
// Notes:
// - Most public methods calls CheckShutdown. This method fails if the sink was shut down.
//
/////////////////////////////////////////////////////////////////////////////////////////////


HRESULT CGmSink_CreateInstance(REFIID iid, void **ppMFT)
{
	return CGmSink::CreateInstance(iid, ppMFT);
}

//-------------------------------------------------------------------
// Name: CreateInstance
// Description: Creates an instance of the GmSink object.
// [See CreateGmSink]
//-------------------------------------------------------------------

/* static */ HRESULT CGmSink::CreateInstance(REFIID iid, void **ppSink)
{
	HRESULT hr = S_OK;
	CGmSink *pSink = NULL;

	if (ppSink == NULL)
	{
		GM_CHECK_HR(hr = E_INVALIDARG);
	}

	// Initialize the media engine. NOP if already done
	GM_CHECK_HR(hr = GmEngine::Initialize());

	pSink = new CGmSink();   // Created with ref count = 1.

	if (pSink == NULL)
	{
		GM_CHECK_HR(hr = E_OUTOFMEMORY);
	}

	GM_CHECK_HR(hr = pSink->Initialize()); // create stream which will increment ref()

	GM_CHECK_HR(hr = pSink->QueryInterface(iid, ppSink));

bail:
	GmSafeRelease(&pSink);

	return hr;
}


//-------------------------------------------------------------------
// CGmSink constructor.
//-------------------------------------------------------------------

CGmSink::CGmSink() :
m_nRefCount(1), m_IsShutdown(FALSE), m_pStream(NULL), m_pClock(NULL)
{
	InitializeCriticalSection(&m_critSec);
}

//-------------------------------------------------------------------
// CGmSink destructor.
//-------------------------------------------------------------------

CGmSink::~CGmSink()
{
	assert(m_IsShutdown);

	m_objCall = NULL;

	DeleteCriticalSection(&m_critSec);

	GM_DEBUG_INFO("*** CGmSink destroyed ***");
}

// IUnknown methods

ULONG CGmSink::AddRef()
{
	return InterlockedIncrement(&m_nRefCount);
}

ULONG  CGmSink::Release()
{
	ULONG uCount = InterlockedDecrement(&m_nRefCount);
	if (uCount == 0)
	{
		delete this;
	}
	// For thread safety, return a temporary variable.
	return uCount;
}

HRESULT CGmSink::QueryInterface(REFIID iid, void** ppv)
{
	if (!ppv)
	{
		return E_POINTER;
	}
	if (iid == IID_IUnknown)
	{
		*ppv = static_cast<IUnknown*>(static_cast<IMFFinalizableMediaSink*>(this));
	}
	else if (iid == __uuidof(IMFMediaSink))
	{
		*ppv = static_cast<IMFMediaSink*>(this);
	}
	else if (iid == __uuidof(IMFFinalizableMediaSink))
	{
		*ppv = static_cast<IMFFinalizableMediaSink*>(this);
	}
	else if (iid == __uuidof(IMFClockStateSink))
	{
		*ppv = static_cast<IMFClockStateSink*>(this);
	}
	else if (iid == __uuidof(IMFAttributes))
	{
		*ppv = static_cast<IMFAttributes*>(this);
	}
	else if (iid == __uuidof(IGmSessionWebRTC))
	{
		*ppv = static_cast<IGmSessionWebRTC*>(this);
	}
	else
	{
		*ppv = NULL;
		return E_NOINTERFACE;
	}
	AddRef();
	return S_OK;
}




///  IMFMediaSink methods.


//-------------------------------------------------------------------
// Name: GetCharacteristics
// Description: Returns the characteristics flags.
//
// Note: This sink has a fixed number of streams and is rateless.
//-------------------------------------------------------------------

HRESULT CGmSink::GetCharacteristics(DWORD *pdwCharacteristics)
{
	if (pdwCharacteristics == NULL)
	{
		return E_INVALIDARG;
	}

	EnterCriticalSection(&m_critSec);

	HRESULT hr = CheckShutdown();

	if (SUCCEEDED(hr))
	{
		*pdwCharacteristics = MEDIASINK_FIXED_STREAMS | MEDIASINK_RATELESS;
	}

	LeaveCriticalSection(&m_critSec);
	return hr;
}


//-------------------------------------------------------------------
// Name: AddStreamSink
// Description: Adds a new stream to the sink.
//
// Note: This sink has a fixed number of streams, so this method
//       always returns MF_E_STREAMSINKS_FIXED.
//-------------------------------------------------------------------

HRESULT CGmSink::AddStreamSink(
	DWORD dwStreamSinkIdentifier,
	IMFMediaType *pMediaType,
	IMFStreamSink **ppStreamSink)
{
	return MF_E_STREAMSINKS_FIXED;
}



//-------------------------------------------------------------------
// Name: RemoveStreamSink
// Description: Removes a stream from the sink.
//
// Note: This sink has a fixed number of streams, so this method
//       always returns MF_E_STREAMSINKS_FIXED.
//-------------------------------------------------------------------

HRESULT CGmSink::RemoveStreamSink(DWORD dwStreamSinkIdentifier)
{
	return MF_E_STREAMSINKS_FIXED;
}


//-------------------------------------------------------------------
// Name: GetStreamSinkCount
// Description: Returns the number of streams.
//-------------------------------------------------------------------

HRESULT CGmSink::GetStreamSinkCount(DWORD *pcStreamSinkCount)
{
	HRESULT hr = S_OK;

	EnterCriticalSection(&m_critSec);

	if (pcStreamSinkCount == NULL)
	{
		GM_CHECK_HR(hr = E_INVALIDARG);
	}

	GM_CHECK_HR(hr = CheckShutdown());

	*pcStreamSinkCount = 1;  // Fixed number of streams.

bail:
	LeaveCriticalSection(&m_critSec);
	return hr;
}


//-------------------------------------------------------------------
// Name: GetStreamSinkByIndex
// Description: Retrieves a stream by index.
//-------------------------------------------------------------------

HRESULT CGmSink::GetStreamSinkByIndex(
	DWORD dwIndex,
	IMFStreamSink **ppStreamSink)
{
	HRESULT hr = S_OK;

	EnterCriticalSection(&m_critSec);

	if (ppStreamSink == NULL)
	{
		GM_CHECK_HR(hr = E_INVALIDARG);
	}

	// Fixed stream: Index must be always equal to 0.
	if (dwIndex != 0)
	{
		GM_CHECK_HR(hr = MF_E_INVALIDINDEX);
	}

	GM_CHECK_HR(hr = CheckShutdown());

	*ppStreamSink = m_pStream;
	(*ppStreamSink)->AddRef();

bail:
	LeaveCriticalSection(&m_critSec);
	return hr;
}


//-------------------------------------------------------------------
// Name: GetStreamSinkById
// Description: Retrieves a stream by ID.
//-------------------------------------------------------------------

HRESULT CGmSink::GetStreamSinkById(
	DWORD dwStreamSinkIdentifier,
	IMFStreamSink **ppStreamSink)
{
	HRESULT hr = S_OK;
	EnterCriticalSection(&m_critSec);

	if (ppStreamSink == NULL)
	{
		GM_CHECK_HR(hr = E_INVALIDARG);
	}

	// Fixed stream ID.
	if (dwStreamSinkIdentifier != GM_SINK_STREAM_ID)
	{
		GM_CHECK_HR(hr = MF_E_INVALIDSTREAMNUMBER);
	}

	GM_CHECK_HR(hr = CheckShutdown());

	*ppStreamSink = m_pStream;
	(*ppStreamSink)->AddRef();

bail:
	LeaveCriticalSection(&m_critSec);
	return hr;
}


//-------------------------------------------------------------------
// Name: SetPresentationClock
// Description: Sets the presentation clock.
//
// pPresentationClock: Pointer to the clock. Can be NULL.
//-------------------------------------------------------------------

HRESULT CGmSink::SetPresentationClock(IMFPresentationClock *pPresentationClock)
{
	HRESULT hr = S_OK;

	EnterCriticalSection(&m_critSec);

	GM_CHECK_HR(hr = CheckShutdown());

	// If we already have a clock, remove ourselves from that clock's
	// state notifications.
	if (m_pClock)
	{
		GM_CHECK_HR(hr = m_pClock->RemoveClockStateSink(this));
	}

	// Register ourselves to get state notifications from the new clock.
	if (pPresentationClock)
	{
		GM_CHECK_HR(hr = pPresentationClock->AddClockStateSink(this));
	}

	// Release the pointer to the old clock.
	// Store the pointer to the new clock.

	GmSafeRelease(&m_pClock);
	m_pClock = pPresentationClock;
	if (m_pClock)
	{
		m_pClock->AddRef();
	}

bail:
	LeaveCriticalSection(&m_critSec);
	return hr;
}


//-------------------------------------------------------------------
// Name: GetPresentationClock
// Description: Returns a pointer to the presentation clock.
//-------------------------------------------------------------------

HRESULT CGmSink::GetPresentationClock(IMFPresentationClock **ppPresentationClock)
{
	HRESULT hr = S_OK;

	EnterCriticalSection(&m_critSec);

	if (ppPresentationClock == NULL)
	{
		GM_CHECK_HR(hr = E_INVALIDARG);
	}

	GM_CHECK_HR(hr = CheckShutdown());

	if (m_pClock == NULL)
	{
		GM_CHECK_HR(hr = MF_E_NO_CLOCK); // There is no presentation clock.
	}
	else
	{
		// Return the pointer to the caller.
		*ppPresentationClock = m_pClock;
		(*ppPresentationClock)->AddRef();
	}

bail:
	LeaveCriticalSection(&m_critSec);
	return hr;
}


//-------------------------------------------------------------------
// Name: Shutdown
// Description: Releases resources held by the media sink.
//-------------------------------------------------------------------

HRESULT CGmSink::Shutdown()
{
	HRESULT hr = S_OK;

	EnterCriticalSection(&m_critSec);

	GM_CHECK_HR(hr = CheckShutdown());
	
	hr = m_pStream->Shutdown(); // Do not jump to bail even if it fails

	GmSafeRelease(&m_pClock);
	GmSafeRelease(&m_pStream);

	m_IsShutdown = true;

bail:
	LeaveCriticalSection(&m_critSec);
	return hr;
}

/// IMFFinalizableMediaSink methods


//-------------------------------------------------------------------
// Name: BeginFinalize
// Description: Starts the asynchronous finalize operation.
//
// Note: We use the Finalize operation to write the RIFF headers.
//-------------------------------------------------------------------

HRESULT CGmSink::BeginFinalize(
	IMFAsyncCallback *pCallback,
	IUnknown *punkState)
{
	HRESULT hr = S_OK;

	EnterCriticalSection(&m_critSec);

	GM_CHECK_HR(hr = CheckShutdown());

	// Tell the stream to finalize.
	GM_CHECK_HR(hr = m_pStream->Finalize(pCallback, punkState));

bail:
	LeaveCriticalSection(&m_critSec);
	return hr;
}


//-------------------------------------------------------------------
// Name: EndFinalize
// Description: Completes the asynchronous finalize operation.
//-------------------------------------------------------------------

HRESULT CGmSink::EndFinalize(IMFAsyncResult *pResult)
{
	HRESULT hr = S_OK;

	// Return the status code from the async result.
	if (pResult == NULL)
	{
		GM_CHECK_HR(hr = E_INVALIDARG);
	}
	else
	{
		GM_CHECK_HR(hr = pResult->GetStatus());
	}

bail:
	return hr;
}

//-------------------------------------------------------------------
// Name: OnClockStart
// Description: Called when the presentation clock starts.
//
// hnsSystemTime: System time when the clock started.
// llClockStartOffset: Starting presentatation time.
//
// Note: For an archive sink, we don't care about the system time.
//       But we need to cache the value of llClockStartOffset. This
//       gives us the earliest time stamp that we archive. If any
//       input samples have an earlier time stamp, we discard them.
//-------------------------------------------------------------------

HRESULT CGmSink::OnClockStart(
	/* [in] */ MFTIME hnsSystemTime,
	/* [in] */ LONGLONG llClockStartOffset)
{
	HRESULT hr = S_OK;

	EnterCriticalSection(&m_critSec);

	GM_CHECK_HR(hr = CheckShutdown());
	
	GM_CHECK_HR(hr = m_pStream->Start(llClockStartOffset));

	GM_CHECK_HR(hr = m_objCall->Start());

bail:
	LeaveCriticalSection(&m_critSec);
	return hr;
}

//-------------------------------------------------------------------
// Name: OnClockStop
// Description: Called when the presentation clock stops.
//
// Note: After this method is called, we stop accepting new data.
//-------------------------------------------------------------------

HRESULT CGmSink::OnClockStop(
	/* [in] */ MFTIME hnsSystemTime)
{
	HRESULT hr = S_OK;

	EnterCriticalSection(&m_critSec);

	GM_CHECK_HR(hr = CheckShutdown());
	
	GM_CHECK_HR(hr = m_pStream->Stop());

	GM_CHECK_HR(hr = m_objCall->Stop());

bail:
	LeaveCriticalSection(&m_critSec);
	return hr;
}


//-------------------------------------------------------------------
// Name: OnClockPause
// Description: Called when the presentation clock paused.
//
// Note: For an archive sink, the paused state is equivalent to the
//       running (started) state. We still accept data and archive it.
//-------------------------------------------------------------------

HRESULT CGmSink::OnClockPause(
	/* [in] */ MFTIME hnsSystemTime)
{
	HRESULT hr = S_OK;

	EnterCriticalSection(&m_critSec);

	GM_CHECK_HR(hr = CheckShutdown());
	
	GM_CHECK_HR(hr = m_pStream->Pause());

	GM_CHECK_HR(hr = m_objCall->Pause());

bail:
	LeaveCriticalSection(&m_critSec);
	return hr;
}


//-------------------------------------------------------------------
// Name: OnClockRestart
// Description: Called when the presentation clock restarts.
//-------------------------------------------------------------------

HRESULT CGmSink::OnClockRestart(
	/* [in] */ MFTIME hnsSystemTime)
{
	HRESULT hr = S_OK;

	EnterCriticalSection(&m_critSec);

	GM_CHECK_HR(hr = CheckShutdown());
	
	GM_CHECK_HR(hr = m_pStream->Restart());

bail:
	LeaveCriticalSection(&m_critSec);
	return hr;
}


//-------------------------------------------------------------------
// Name: OnClockSetRate
// Description: Called when the presentation clock's rate changes.
//
// Note: For a rateless sink, the clock rate is not important.
//-------------------------------------------------------------------

HRESULT CGmSink::OnClockSetRate(
	/* [in] */ MFTIME hnsSystemTime,
	/* [in] */ float flRate)
{
	return S_OK;
}






///
/// CGmAttributes methods
///

STDMETHODIMP CGmSink::GetUINT32(REFGUID guidKey, UINT32* punValue)
{
	HRESULT hr = S_OK;

	if (!punValue)
	{
		GM_CHECK_HR(hr = E_INVALIDARG);
	}

	if (guidKey == GM_PARAM_ICE_ENABLED)
	{
		
	}

	hr = CGmAttributes::GetUINT32(guidKey, punValue);

bail:
	return hr;
}

STDMETHODIMP CGmSink::SetUINT32(REFGUID guidKey, UINT32 unValue)
{
	HRESULT hr = S_OK;

	if (guidKey == GM_PARAM_ICE_ENABLED)
	{
		GM_CHECK_HR(hr = m_objCall->SetNattIceEnabled(unValue == TRUE));
	}
	else if (guidKey == GM_PARAM_ICE_STUN_ENABLED)
	{
		GM_CHECK_HR(hr = m_objCall->SetNattIceStunEnabled(unValue == TRUE));
	}
	else if (guidKey == GM_PARAM_ICE_TURN_ENABLED)
	{
		GM_CHECK_HR(hr = m_objCall->SetNattIceTurnEnabled(unValue == TRUE));
	}
	else if (guidKey == GM_PARAM_RTCP_ENABLED)
	{
		GM_CHECK_HR(hr = m_objCall->SetRTCPEnabled(unValue == TRUE));
	}
	else if (guidKey == GM_PARAM_RTCPMUX_ENABLED)
	{
		GM_CHECK_HR(hr = m_objCall->SetRTCPMuxEnabled(unValue == TRUE));
	}
	else if (guidKey == GM_PARAM_AVPF_MODE)
	{
		GM_CHECK_HR(hr = m_objCall->SetAVPFMode((GmMode_t)unValue));
	}
	else if (guidKey == GM_PARAM_WEBRTC_PROFILE_ENABLED)
	{
		GM_CHECK_HR(hr = m_objCall->SetRTCWebProfileEnabled(unValue == TRUE));
	}
	else if (guidKey == GM_PARAM_SRTP_MODE)
	{
		GM_CHECK_HR(hr = m_objCall->SetSRTPMode((GmMode_t)unValue));
	}
	else if (guidKey == GM_PARAM_SRTP_TYPE)
	{
		GM_CHECK_HR(hr = m_objCall->SetSRTPType(unValue));
	}
	else
	{
		hr = CGmAttributes::SetUINT32(guidKey, unValue);
	}

bail:
	return hr;
}

STDMETHODIMP CGmSink::GetString(REFGUID guidKey, LPWSTR pwszValue, UINT32 cchBufSize, UINT32* pcchLength)
{
	HRESULT hr = S_OK;

	if (guidKey == GM_PARAM_LOCAL_SDP)
	{
		std::string strSDP; 
		std::wstring wstrSDP;

		if (cchBufSize < 2 || !pwszValue || !pcchLength)
		{
			GM_CHECK_HR(hr = E_INVALIDARG);
		}
		
		// apply new settings before requesting local SDP
		GM_CHECK_HR(hr = m_pStream->ApplyMediaParams());

		hr = m_objCall->GetLocalSDP(strSDP);
		if (hr == E_PENDING)
		{
			goto bail;
		}
		GM_CHECK_HR(hr);
		GM_CHECK_HR(hr = GmSinkUtils::ConvertStringWString(strSDP, wstrSDP));
		*pcchLength = (UINT32) (TSK_MIN(cchBufSize - 1, wstrSDP.length()));
		_tcsncpy(pwszValue, wstrSDP.c_str(), *pcchLength);
		pwszValue[*pcchLength] = 0;
	}
	else if (guidKey == GM_PARAM_REMOTE_SDP)
	{
		GM_CHECK_HR(hr = E_NOTIMPL); // Doesn't make sense asking for remote SDP
	}
	else
	{
		hr = CGmAttributes::GetString(guidKey, pwszValue, cchBufSize, pcchLength);
	}

bail:
	return hr;
}

STDMETHODIMP CGmSink::SetString(REFGUID guidKey, LPCWSTR wszValue)
{
	HRESULT hr = S_OK;

	if (guidKey == GM_PARAM_LOCAL_SDP)
	{
		GM_CHECK_HR(hr = E_NOTIMPL); // Doesn't make sense setting local SDP
	}
	else if (guidKey == GM_PARAM_REMOTE_SDP || guidKey == GM_PARAM_REMOTE_SDP_OFFER || guidKey == GM_PARAM_REMOTE_SDP_ANSWER || guidKey == GM_PARAM_REMOTE_SDP_PRANSWER)
	{
		std::string strSDP;
		if (!wszValue || _tcsclen(wszValue) == 0)
		{
			GM_CHECK_HR(hr = E_INVALIDARG);
		}
		GM_CHECK_HR(hr = GmSinkUtils::ConvertWStringString(wszValue, strSDP));

		// apply new settings before setting new remote SDP
		GM_CHECK_HR(hr = m_pStream->ApplyMediaParams());

		GM_CHECK_HR(hr = m_objCall->SetRemoteSDP(strSDP, 
			guidKey == GM_PARAM_REMOTE_SDP_OFFER ? GmRoType_Offer : (guidKey == GM_PARAM_REMOTE_SDP_ANSWER ? GmRoType_Answer : (guidKey == GM_PARAM_REMOTE_SDP_PRANSWER ? GmRoType_PrAnswer : GmRoType_Unknown))));
	}
	else if (guidKey == GM_PARAM_SSL_PATH_PUBLIC || guidKey == GM_PARAM_SSL_PATH_PRIVATE || guidKey == GM_PARAM_SSL_PATH_CA)
	{ 
		std::string strPath;
		if (!wszValue || _tcsclen(wszValue) == 0)
		{
			GM_CHECK_HR(hr = E_INVALIDARG);
		}
		GM_CHECK_HR(hr = GmSinkUtils::ConvertWStringString(wszValue, strPath));
		GM_CHECK_HR(hr = (guidKey == GM_PARAM_SSL_PATH_PUBLIC) ? m_objCall->SetSSLPublic(strPath) : (guidKey == GM_PARAM_SSL_PATH_PRIVATE ? m_objCall->SetSSLPrivate(strPath) : m_objCall->SetSSLCA(strPath)));
	}
	else if (guidKey == GM_PARAM_ICE_SERVER)
	{
		GM_CHECK_HR(hr = m_objCall->AddIceServer(wszValue));
	}
	else
	{
		hr = CGmAttributes::SetString(guidKey, wszValue);
	}

bail:
	return hr;
}

STDMETHODIMP CGmSink::SetUnknown(REFGUID guidKey, IUnknown* pUnknown)
{
	HRESULT hr = S_OK;

	if (guidKey == GM_PARAM_ICE_CALLBACK)
	{
		IGmIceCallback* pCallBack = NULL;
		if (pUnknown)
		{
			GM_CHECK_HR(hr = pUnknown->QueryInterface(&pCallBack));
		}
		hr = m_objCall->SetIceCallback(pCallBack);
		GmSafeRelease(&pCallBack);
		GM_CHECK_HR(hr);
	}
	else
	{
		hr = CGmAttributes::SetUnknown(guidKey, pUnknown);
	}

bail:
	return hr;
}


///
/// IGmSessionWebRTC methods
///


STDMETHODIMP CGmSink::Test()
{
	GM_DEBUG_INFO("CGmSink::Test()");
	return S_OK;
}







/// Private methods


//-------------------------------------------------------------------
// Name: Initialize
// Description: Initializes the media sink.
//
// Note: This method is called once when the media sink is first
//       initialized.
//-------------------------------------------------------------------

HRESULT CGmSink::Initialize()
{
	HRESULT hr = S_OK;

	m_pStream = new CGmStreamSink();
	if (m_pStream == NULL)
	{
		GM_CHECK_HR(hr = E_OUTOFMEMORY);
	}

	// Initialize the attributes.
	GM_CHECK_HR(hr = CGmAttributes::Initialize());

	// Create the call session
	m_objCall = GmSessionCall::New();
	if (!m_objCall)
	{
		GM_CHECK_HR(hr = E_OUTOFMEMORY);
	}

	// Initialize the stream.
	GM_CHECK_HR(hr = m_pStream->Initialize(this));

bail:
	return hr;
}



/////////////////////////////////////////////////////////////////////////////////////////////
//
// CGmAsyncOperation class. - Private class used by CGmStreamSink class.
//
/////////////////////////////////////////////////////////////////////////////////////////////

CGmStreamSink::CGmAsyncOperation::CGmAsyncOperation(GmStreamOperation op)
	: m_nRefCount(1), m_op(op)
{
}

CGmStreamSink::CGmAsyncOperation::~CGmAsyncOperation()
{
	assert(m_nRefCount == 0);
}

ULONG CGmStreamSink::CGmAsyncOperation::AddRef()
{
	return InterlockedIncrement(&m_nRefCount);
}

ULONG CGmStreamSink::CGmAsyncOperation::Release()
{
	ULONG uCount = InterlockedDecrement(&m_nRefCount);
	if (uCount == 0)
	{
		delete this;
	}
	// For thread safety, return a temporary variable.
	return uCount;
}

HRESULT CGmStreamSink::CGmAsyncOperation::QueryInterface(REFIID iid, void** ppv)
{
	if (!ppv)
	{
		return E_POINTER;
	}
	if (iid == IID_IUnknown)
	{
		*ppv = static_cast<IUnknown*>(this);
	}
	else
	{
		*ppv = NULL;
		return E_NOINTERFACE;
	}
	AddRef();
	return S_OK;
}


/////////////////////////////////////////////////////////////////////////////////////////////
//
// CGmStreamSink class. - Implements the stream sink.
//
// Notes:
// - Most of the real work gets done in this class.
// - The sink has one stream. If it had multiple streams, it would need to coordinate them.
// - Most operations are done asynchronously on a work queue.
// - Async methods are handled like this:
//      1. Call ValidateOperation to check if the operation is permitted at this time
//      2. Create an CGmAsyncOperation object for the operation.
//      3. Call QueueAsyncOperation. This puts the operation on the work queue.
//      4. The workqueue calls OnDispatchWorkItem.
// - Locking:
//      To avoid deadlocks, do not hold the CGmStreamSink lock followed by the CGmSink lock.
//      The other order is OK (CGmSink, then CGmStreamSink).
//
/////////////////////////////////////////////////////////////////////////////////////////////


//-------------------------------------------------------------------
// CGmStreamSink constructor
//-------------------------------------------------------------------

CGmStreamSink::CGmStreamSink()
	: m_nRefCount(1), m_state(State_TypeNotSet), m_IsShutdown(FALSE),
	m_pSink(NULL), m_pEventQueue(NULL),
	m_pCurrentType(NULL), m_pFinalizeResult(NULL),
	m_StartTime(0), m_cbDataWritten(0), m_WorkQueueId(0),
	m_WorkQueueCB(this, &CGmStreamSink::OnDispatchWorkItem)
{
	InitializeCriticalSection(&m_critSec);
}


//-------------------------------------------------------------------
// CGmStreamSink destructor
//-------------------------------------------------------------------

CGmStreamSink::~CGmStreamSink()
{
	assert(m_IsShutdown);
	DeleteCriticalSection(&m_critSec);

	GM_DEBUG_INFO("*** CGmStreamSink destroyed ***");
}


// IUnknown methods

ULONG CGmStreamSink::AddRef()
{
	return InterlockedIncrement(&m_nRefCount);
}

ULONG  CGmStreamSink::Release()
{
	ULONG uCount = InterlockedDecrement(&m_nRefCount);
	if (uCount == 0)
	{
		delete this;
	}
	// For thread safety, return a temporary variable.
	return uCount;
}

HRESULT CGmStreamSink::QueryInterface(REFIID iid, void** ppv)
{
	if (!ppv)
	{
		return E_POINTER;
	}
	if (iid == IID_IUnknown)
	{
		*ppv = static_cast<IUnknown*>(static_cast<IMFStreamSink*>(this));
	}
	else if (iid == __uuidof(IMFStreamSink))
	{
		*ppv = static_cast<IMFStreamSink *>(this);
	}
	else if (iid == __uuidof(IMFMediaEventGenerator))
	{
		*ppv = static_cast<IMFMediaEventGenerator*>(this);
	}
	else if (iid == __uuidof(IMFMediaTypeHandler))
	{
		*ppv = static_cast<IMFMediaTypeHandler*>(this);
	}
	else if (iid == __uuidof(IMFAttributes))
	{
		*ppv = static_cast<IMFAttributes*>(this);
	}
	else if (iid == __uuidof(IGmStreamWebRTC))
	{
		*ppv = static_cast<IGmStreamWebRTC*>(this);
	}
	else
	{
		*ppv = NULL;
		return E_NOINTERFACE;
	}
	AddRef();
	return S_OK;
}


// IMFMediaEventGenerator methods.
// Note: These methods call through to the event queue helper object.

HRESULT CGmStreamSink::BeginGetEvent(IMFAsyncCallback* pCallback, IUnknown* punkState)
{
	HRESULT hr = S_OK;

	EnterCriticalSection(&m_critSec);

	hr = CheckShutdown();

	if (SUCCEEDED(hr))
	{
		hr = m_pEventQueue->BeginGetEvent(pCallback, punkState);
	}

	LeaveCriticalSection(&m_critSec);
	return hr;
}

HRESULT CGmStreamSink::EndGetEvent(IMFAsyncResult* pResult, IMFMediaEvent** ppEvent)
{
	HRESULT hr = S_OK;

	EnterCriticalSection(&m_critSec);

	hr = CheckShutdown();

	if (SUCCEEDED(hr))
	{
		hr = m_pEventQueue->EndGetEvent(pResult, ppEvent);
	}

	LeaveCriticalSection(&m_critSec);
	return hr;
}

HRESULT CGmStreamSink::GetEvent(DWORD dwFlags, IMFMediaEvent** ppEvent)
{
	// NOTE:
	// GetEvent can block indefinitely, so we don't hold the lock.
	// This requires some juggling with the event queue pointer.

	HRESULT hr = S_OK;

	IMFMediaEventQueue *pQueue = NULL;

	EnterCriticalSection(&m_critSec);

	// Check shutdown
	hr = CheckShutdown();

	// Get the pointer to the event queue.
	if (SUCCEEDED(hr))
	{
		pQueue = m_pEventQueue;
		pQueue->AddRef();
	}

	LeaveCriticalSection(&m_critSec);

	// Now get the event.
	if (SUCCEEDED(hr))
	{
		hr = pQueue->GetEvent(dwFlags, ppEvent);
	}

	GmSafeRelease(&pQueue);

	return hr;
}

HRESULT CGmStreamSink::QueueEvent(MediaEventType met, REFGUID guidExtendedType, HRESULT hrStatus, const PROPVARIANT* pvValue)
{
	HRESULT hr = S_OK;

	EnterCriticalSection(&m_critSec);

	GM_CHECK_HR(hr = CheckShutdown());

	GM_CHECK_HR(hr = m_pEventQueue->QueueEventParamVar(met, guidExtendedType, hrStatus, pvValue));

bail:
	LeaveCriticalSection(&m_critSec);
	return hr;
}




/// IMFStreamSink methods


//-------------------------------------------------------------------
// Name: GetMediaSink
// Description: Returns the parent media sink.
//-------------------------------------------------------------------

HRESULT CGmStreamSink::GetMediaSink(IMFMediaSink **ppMediaSink)
{
	HRESULT hr = S_OK;

	EnterCriticalSection(&m_critSec);

	if (ppMediaSink == NULL)
	{
		GM_CHECK_HR(hr = E_INVALIDARG);
	}

	GM_CHECK_HR(hr = CheckShutdown());
	
	*ppMediaSink = (IMFMediaSink*)m_pSink;
	(*ppMediaSink)->AddRef();

bail:
	LeaveCriticalSection(&m_critSec);
	return hr;
}


//-------------------------------------------------------------------
// Name: GetIdentifier
// Description: Returns the stream identifier.
//-------------------------------------------------------------------

HRESULT CGmStreamSink::GetIdentifier(DWORD *pdwIdentifier)
{
	HRESULT hr = S_OK;

	EnterCriticalSection(&m_critSec);

	if (pdwIdentifier == NULL)
	{
		GM_CHECK_HR(hr = E_INVALIDARG);
	}

	GM_CHECK_HR(hr = CheckShutdown());
	
	*pdwIdentifier = GM_SINK_STREAM_ID;

bail:
	LeaveCriticalSection(&m_critSec);
	return hr;
}


//-------------------------------------------------------------------
// Name: GetMediaTypeHandler
// Description: Returns a media type handler for this stream.
//-------------------------------------------------------------------

HRESULT CGmStreamSink::GetMediaTypeHandler(IMFMediaTypeHandler **ppHandler)
{
	HRESULT hr = S_OK;

	EnterCriticalSection(&m_critSec);

	if (ppHandler == NULL)
	{
		GM_CHECK_HR(hr = E_INVALIDARG);
	}

	GM_CHECK_HR(hr = CheckShutdown());

	// This stream object acts as its own type handler, so we QI ourselves.
	GM_CHECK_HR(hr = this->QueryInterface(IID_IMFMediaTypeHandler, (void**)ppHandler));

bail:
	LeaveCriticalSection(&m_critSec);
	return hr;
}


//-------------------------------------------------------------------
// Name: ProcessSample
// Description: Receives an input sample. [Asynchronous]
//
// Note: The client should only give us a sample after we send an
//       MEStreamSinkRequestSample event.
//-------------------------------------------------------------------

HRESULT CGmStreamSink::ProcessSample(IMFSample *pSample)
{
	HRESULT hr = S_OK;

	EnterCriticalSection(&m_critSec);

	if (pSample == NULL)
	{
		GM_CHECK_HR(hr = E_INVALIDARG);
	}

	GM_CHECK_HR(hr = CheckShutdown());

	// Validate the operation.
	GM_CHECK_HR(hr = ValidateOperation(OpProcessSample));

	// Add the sample to the sample queue.
	GM_CHECK_HR(hr = m_SampleQueue.InsertBack(pSample));

	// Unless we are paused, start an async operation to dispatch the next sample.
	if (m_state != State_Paused)
	{
		// Queue the operation.
		GM_CHECK_HR(hr = QueueAsyncOperation(OpProcessSample));
	}

bail:
	LeaveCriticalSection(&m_critSec);
	return hr;
}


//-------------------------------------------------------------------
// Name: PlaceMarker
// Description: Receives a marker. [Asynchronous]
//
// Note: The client can call PlaceMarker at any time. In response,
//       we need to queue an MEStreamSinkMarer event, but not until
//       *after* we have processed all samples that we have received
//       up to this point.
//
//       Also, in general you might need to handle specific marker
//       types, although this sink does not.
//-------------------------------------------------------------------

HRESULT CGmStreamSink::PlaceMarker(
	MFSTREAMSINK_MARKER_TYPE eMarkerType,
	const PROPVARIANT *pvarMarkerValue,
	const PROPVARIANT *pvarContextValue)
{
	HRESULT hr = S_OK;

	IGmMarker *pMarker = NULL;

	EnterCriticalSection(&m_critSec);

	GM_CHECK_HR(hr = CheckShutdown());
	
	GM_CHECK_HR(hr = ValidateOperation(OpPlaceMarker));

	// Create a marker object and put it on the sample queue.
	GM_CHECK_HR(hr = CGmMarker::Create(
		eMarkerType,
		pvarMarkerValue,
		pvarContextValue,
		&pMarker));

	
	GM_CHECK_HR(hr = m_SampleQueue.InsertBack(pMarker));

	// Unless we are paused, start an async operation to dispatch the next sample/marker.
	if (m_state != State_Paused)
	{
		// Queue the operation.
		GM_CHECK_HR(hr = QueueAsyncOperation(OpPlaceMarker)); // Increments ref count on pOp.
	}

bail:
	GmSafeRelease(&pMarker);
	LeaveCriticalSection(&m_critSec);
	return hr;
}


//-------------------------------------------------------------------
// Name: Flush
// Description: Discards all samples that were not processed yet.
//-------------------------------------------------------------------

HRESULT CGmStreamSink::Flush()
{
	HRESULT hr = S_OK;

	EnterCriticalSection(&m_critSec);

	GM_CHECK_HR(hr = CheckShutdown());
	
	// Note: Even though we are flushing data, we still need to send
	// any marker events that were queued.
	GM_CHECK_HR(hr = ProcessSamplesFromQueue(DropSamples));

bail:
	LeaveCriticalSection(&m_critSec);
	return hr;
}


/// IMFMediaTypeHandler methods

//-------------------------------------------------------------------
// Name: IsMediaTypeSupported
// Description: Check if a media type is supported.
//
// pMediaType: The media type to check.
// ppMediaType: Optionally, receives a "close match" media type.
//-------------------------------------------------------------------


HRESULT CGmStreamSink::IsMediaTypeSupported(
	/* [in] */ IMFMediaType *pMediaType,
	/* [out] */ IMFMediaType **ppMediaType)
{
	HRESULT hr = S_OK;
	GUID majorType = GUID_NULL;

	EnterCriticalSection(&m_critSec);

	if (pMediaType == NULL)
	{
		GM_CHECK_HR(hr = E_INVALIDARG);
	}

	GM_CHECK_HR(hr = CheckShutdown());
	
	GM_CHECK_HR(hr = pMediaType->GetGUID(MF_MT_MAJOR_TYPE, &majorType));

	// First make sure it's video.
	if (majorType != MFMediaType_Video)
	{
		GM_CHECK_HR(hr = MF_E_INVALIDTYPE);
	}
	
	GM_CHECK_HR(hr = GmSinkUtils::ValidateVideoFormat(pMediaType));

	// We don't return any "close match" types.
	if (ppMediaType)
	{
		*ppMediaType = NULL;
	}

bail:
	LeaveCriticalSection(&m_critSec);
	return hr;
}


//-------------------------------------------------------------------
// Name: GetMediaTypeCount
// Description: Return the number of preferred media types.
//-------------------------------------------------------------------

HRESULT CGmStreamSink::GetMediaTypeCount(DWORD *pdwTypeCount)
{
	HRESULT hr = S_OK;

	EnterCriticalSection(&m_critSec);

	if (pdwTypeCount == NULL)
	{
		GM_CHECK_HR(hr = E_INVALIDARG);
	}

	GM_CHECK_HR(hr = CheckShutdown());
	
	*pdwTypeCount = g_GmNumVideoSubtypes;

bail:
	LeaveCriticalSection(&m_critSec);
	return hr;
}


//-------------------------------------------------------------------
// Name: GetMediaTypeByIndex
// Description: Return a preferred media type by index.
//-------------------------------------------------------------------

HRESULT CGmStreamSink::GetMediaTypeByIndex(
	/* [in] */ DWORD dwIndex,
	/* [out] */ IMFMediaType **ppType)
{
	HRESULT hr = S_OK;
	const GUID* subType = NULL;

	EnterCriticalSection(&m_critSec);

	if (ppType == NULL)
	{
		GM_CHECK_HR(hr = E_INVALIDARG);
	}

	GM_CHECK_HR(hr = CheckShutdown());

	
	if (dwIndex >= g_GmNumVideoSubtypes)
	{
		GM_CHECK_HR(hr = MF_E_NO_MORE_TYPES);
	}

	subType = g_GmVideoSubtypes[dwIndex];
	GM_CHECK_HR(hr = GmSinkUtils::CreateVideoType(subType, ppType));
	
bail:
	LeaveCriticalSection(&m_critSec);
	return hr;
}


//-------------------------------------------------------------------
// Name: SetCurrentMediaType
// Description: Set the current media type.
//-------------------------------------------------------------------

HRESULT CGmStreamSink::SetCurrentMediaType(IMFMediaType *pMediaType)
{
	HRESULT hr = S_OK;

	EnterCriticalSection(&m_critSec);

	if (pMediaType == NULL)
	{
		GM_CHECK_HR(hr = E_INVALIDARG);
	}

	GM_CHECK_HR(hr = CheckShutdown());

	// We don't allow format changes after streaming starts,
	// because this would invalidate the .wav file.
	GM_CHECK_HR(hr = ValidateOperation(OpSetMediaType));

	if (SUCCEEDED(hr))
	{
		hr = IsMediaTypeSupported(pMediaType, NULL);
	}

	if (SUCCEEDED(hr))
	{
		GmSafeRelease(&m_pCurrentType);
		m_pCurrentType = pMediaType;
		m_pCurrentType->AddRef();

		m_state = State_Ready;
	}

bail:
	LeaveCriticalSection(&m_critSec);
	return hr;
}

//-------------------------------------------------------------------
// Name: GetCurrentMediaType
// Description: Return the current media type, if any.
//-------------------------------------------------------------------

HRESULT CGmStreamSink::GetCurrentMediaType(IMFMediaType **ppMediaType)
{
	HRESULT hr = S_OK;

	EnterCriticalSection(&m_critSec);

	if (ppMediaType == NULL)
	{
		GM_CHECK_HR(hr = E_INVALIDARG);
	}

	GM_CHECK_HR(hr = CheckShutdown());
	
	if (m_pCurrentType == NULL)
	{
		hr = MF_E_NOT_INITIALIZED;
		goto bail;
	}
	
	*ppMediaType = m_pCurrentType;
	(*ppMediaType)->AddRef();

bail:
	LeaveCriticalSection(&m_critSec);
	return hr;
}


//-------------------------------------------------------------------
// Name: GetMajorType
// Description: Return the major type GUID.
//-------------------------------------------------------------------

HRESULT CGmStreamSink::GetMajorType(GUID *pguidMajorType)
{
	HRESULT hr = S_OK;

	if (pguidMajorType == NULL)
	{
		GM_CHECK_HR(hr = E_INVALIDARG);
	}

	*pguidMajorType = MFMediaType_Video;

bail:
	return hr;
}


// private methods



//-------------------------------------------------------------------
// Name: Initialize
// Description: Initializes the stream sink.
//
// Note: This method is called once when the media sink is first
//       initialized.
//-------------------------------------------------------------------

HRESULT CGmStreamSink::Initialize(CGmSink *pParent)
{
	HRESULT hr = S_OK;

	if (pParent == NULL)
	{
		GM_CHECK_HR(hr = E_INVALIDARG);
	}

	// Create the event queue helper.
	GM_CHECK_HR(hr = MFCreateEventQueue(&m_pEventQueue));

	// Allocate a new work queue for async operations.
	GM_CHECK_HR(hr = MFAllocateWorkQueue(&m_WorkQueueId));
	
	m_pSink = pParent;
	m_pSink->AddRef();

bail:
	return hr;
}

HRESULT	CGmStreamSink::ApplyMediaParams()
{
	HRESULT hr = S_OK;
	GUID guidNegMajorType;

	EnterCriticalSection(&m_critSec);

	GM_CHECK_HR(hr = m_pCurrentType->GetGUID(MF_MT_MAJOR_TYPE, &guidNegMajorType));
	if (guidNegMajorType == MFMediaType_Video)
	{
		GUID guidNegSubType;
		UINT32 uNegWidth, uNegHeight;
		UINT32 uNumFPS = 25, uDenFPS = 1;
		UINT32 uBitRateKbps = 0;

		GM_CHECK_HR(hr = m_pCurrentType->GetGUID(MF_MT_SUBTYPE, &guidNegSubType));
		GM_CHECK_HR(hr = MFGetAttributeSize(m_pCurrentType, MF_MT_FRAME_SIZE, &uNegWidth, &uNegHeight));
		GM_CHECK_HR(hr = MFGetAttributeRatio(m_pCurrentType, MF_MT_FRAME_RATE, &uNumFPS, &uDenFPS));
		hr = m_pCurrentType->GetUINT32(MF_MT_AVG_BITRATE, &uBitRateKbps);
		uBitRateKbps /= 1024; // bps -> kbps

		GM_CHECK_HR(hr = m_pSink->m_objCall->SetInputSize(uNegWidth, uNegHeight));
		GM_CHECK_HR(hr = m_pSink->m_objCall->SetInputFormat(guidNegSubType));
		GM_CHECK_HR(hr = m_pSink->m_objCall->SetInputFPS(uNumFPS / uDenFPS));
		GM_CHECK_HR(hr = m_pSink->m_objCall->SetInputBitrate(uBitRateKbps));
	}

bail:
	LeaveCriticalSection(&m_critSec);
	return hr;
}

//-------------------------------------------------------------------
// Name: Start
// Description: Called when the presentation clock starts.
//
// Note: Start time can be PRESENTATION_CURRENT_POSITION meaning
//       resume from the last current position.
//-------------------------------------------------------------------

HRESULT CGmStreamSink::Start(MFTIME start)
{
	HRESULT hr = S_OK;

	EnterCriticalSection(&m_critSec);

	GM_CHECK_HR(hr = ValidateOperation(OpStart));

	GM_CHECK_HR(hr = ApplyMediaParams());
	
	if (start != PRESENTATION_CURRENT_POSITION)
	{
		m_StartTime = start;        // Cache the start time.
	}
	m_state = State_Started;
	GM_CHECK_HR(hr = QueueAsyncOperation(OpStart));
	
bail:
	LeaveCriticalSection(&m_critSec);
	return hr;
}

//-------------------------------------------------------------------
// Name: Stop
// Description: Called when the presentation clock stops.
//-------------------------------------------------------------------

HRESULT CGmStreamSink::Stop()
{
	HRESULT hr = S_OK;

	EnterCriticalSection(&m_critSec);

	GM_CHECK_HR(hr = ValidateOperation(OpStop));
	
	m_state = State_Stopped;
	GM_CHECK_HR(hr = QueueAsyncOperation(OpStop));
	
bail:
	LeaveCriticalSection(&m_critSec);
	return hr;
}


//-------------------------------------------------------------------
// Name: Pause
// Description: Called when the presentation clock pauses.
//-------------------------------------------------------------------

HRESULT CGmStreamSink::Pause()
{
	HRESULT hr = S_OK;
	
	EnterCriticalSection(&m_critSec);

	GM_CHECK_HR(hr = ValidateOperation(OpPause));
	
	m_state = State_Paused;
	GM_CHECK_HR(hr = QueueAsyncOperation(OpPause));

bail:
	LeaveCriticalSection(&m_critSec);
	return hr;
}


//-------------------------------------------------------------------
// Name: Restart
// Description: Called when the presentation clock restarts.
//-------------------------------------------------------------------

HRESULT CGmStreamSink::Restart()
{
	HRESULT hr = S_OK;

	EnterCriticalSection(&m_critSec);

	GM_CHECK_HR(hr = ValidateOperation(OpRestart));
	
	m_state = State_Started;
	GM_CHECK_HR(hr = QueueAsyncOperation(OpRestart));

bail:
	LeaveCriticalSection(&m_critSec);
	return hr;
}


//-------------------------------------------------------------------
// Name: Finalize
// Description: Starts the async finalize operation.
//-------------------------------------------------------------------

HRESULT CGmStreamSink::Finalize(IMFAsyncCallback *pCallback, IUnknown *punkState)
{
	HRESULT hr = S_OK;

	EnterCriticalSection(&m_critSec);

	GM_CHECK_HR(hr = ValidateOperation(OpFinalize));
	
	if (m_pFinalizeResult != NULL)
	{
		GM_CHECK_HR(hr = MF_E_INVALIDREQUEST);  // The operation is already pending.
	}

	// Create and store the async result object.
	GM_CHECK_HR(hr = MFCreateAsyncResult(NULL, pCallback, punkState, &m_pFinalizeResult));
	
	m_state = State_Finalized;
	hr = QueueAsyncOperation(OpFinalize);

bail:
	LeaveCriticalSection(&m_critSec);
	return hr;
}

//-------------------------------------------------------------------
// Name: ValidStateMatrix
// Description: Class-static matrix of operations vs states.
//
// If an entry is TRUE, the operation is valid from that state.
//-------------------------------------------------------------------

BOOL CGmStreamSink::ValidStateMatrix[CGmStreamSink::State_Count][CGmStreamSink::Op_Count] =
{
	// States:    Operations:
	//            SetType   Start     Restart   Pause     Stop      Sample    Marker    Finalize
	/* NotSet */  TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,

	/* Ready */   TRUE, TRUE, FALSE, TRUE, TRUE, FALSE, TRUE, TRUE,

	/* Start */   FALSE, TRUE, FALSE, TRUE, TRUE, TRUE, TRUE, TRUE,

	/* Pause */   FALSE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE,

	/* Stop */    FALSE, TRUE, FALSE, FALSE, TRUE, FALSE, TRUE, TRUE,

	/* Final */   FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE

	// Note about states:
	// 1. OnClockRestart should only be called from paused state.
	// 2. While paused, the sink accepts samples but does not process them.

};


//-------------------------------------------------------------------
// Name: ValidateOperation
// Description: Checks if an operation is valid in the current state.
//-------------------------------------------------------------------

HRESULT CGmStreamSink::ValidateOperation(GmStreamOperation op)
{
	HRESULT hr = S_OK;
	BOOL bTransitionAllowed;

	if (m_IsShutdown)
	{
		GM_CHECK_HR(hr = MF_E_INVALIDREQUEST);
	}

	bTransitionAllowed = ValidStateMatrix[m_state][op];

	if (bTransitionAllowed)
	{
		GM_CHECK_HR(hr = S_OK);
	}
	else
	{
		GM_CHECK_HR(hr = MF_E_INVALIDREQUEST);
	}
bail:
	return hr;
}

//-------------------------------------------------------------------
// Name: Shutdown
// Description: Shuts down the stream sink.
//-------------------------------------------------------------------

HRESULT CGmStreamSink::Shutdown()
{
	HRESULT hr = S_OK;

	if (m_IsShutdown)
	{
		GM_CHECK_HR(hr = MF_E_INVALIDREQUEST);
	}

	if (m_pEventQueue)
	{
		m_pEventQueue->Shutdown();
	}

	MFUnlockWorkQueue(m_WorkQueueId);

	m_SampleQueue.Clear();

	GmSafeRelease(&m_pSink);
	GmSafeRelease(&m_pEventQueue);
	GmSafeRelease(&m_pCurrentType);
	GmSafeRelease(&m_pFinalizeResult);

	m_IsShutdown = TRUE;

bail:
	return hr;
}


//-------------------------------------------------------------------
// Name: QueueAsyncOperation
// Description: Puts an async operation on the work queue.
//-------------------------------------------------------------------

HRESULT CGmStreamSink::QueueAsyncOperation(GmStreamOperation op)
{
	HRESULT hr = S_OK;
	CGmAsyncOperation *pOp = new CGmAsyncOperation(op); // Created with ref count = 1

	if (pOp == NULL)
	{
		GM_CHECK_HR(hr = E_OUTOFMEMORY);
	}

	GM_CHECK_HR(hr = MFPutWorkItem(m_WorkQueueId, &m_WorkQueueCB, pOp));

bail:
	GmSafeRelease(&pOp);
	return hr;
}



//-------------------------------------------------------------------
// Name: OnDispatchWorkItem
// Description: Callback for MFPutWorkItem.
//-------------------------------------------------------------------

HRESULT CGmStreamSink::OnDispatchWorkItem(IMFAsyncResult* pAsyncResult)
{
	HRESULT hr = S_OK;

	IUnknown *pState = NULL;
	CGmAsyncOperation *pOp = NULL;
	GmStreamOperation op;

	// Called by work queue thread. Need to hold the critical section.
	EnterCriticalSection(&m_critSec);

	GM_CHECK_HR(hr = pAsyncResult->GetState(&pState));
	
	// The state object is a CAsncOperation object.
	pOp = (CGmAsyncOperation*)pState;

	op = pOp->m_op;

	switch (op)
	{
	case OpStart:
	case OpRestart:
		// Send MEStreamSinkStarted.
		GM_CHECK_HR(hr = QueueEvent(MEStreamSinkStarted, GUID_NULL, hr, NULL));

		// Kick things off by requesting two samples...
		GM_CHECK_HR(hr = QueueEvent(MEStreamSinkRequestSample, GUID_NULL, hr, NULL));
		GM_CHECK_HR(hr = QueueEvent(MEStreamSinkRequestSample, GUID_NULL, hr, NULL));

		// There might be samples queue from earlier (ie, while paused).
		GM_CHECK_HR(hr = ProcessSamplesFromQueue(WriteSamples));
		break;

	case OpStop:
		// Drop samples from queue.
		GM_CHECK_HR(hr = ProcessSamplesFromQueue(DropSamples));

		// Send the event even if the previous call failed.
		GM_CHECK_HR(hr = QueueEvent(MEStreamSinkStopped, GUID_NULL, hr, NULL));
		break;

	case OpPause:
		GM_CHECK_HR(hr = QueueEvent(MEStreamSinkPaused, GUID_NULL, hr, NULL));
		break;

	case OpProcessSample:
	case OpPlaceMarker:
		GM_CHECK_HR(hr = DispatchProcessSample(pOp));
		break;

	case OpFinalize:
		GM_CHECK_HR(hr = DispatchFinalize(pOp));
		break;
	}

bail:
	GmSafeRelease(&pState);
	LeaveCriticalSection(&m_critSec);
	return hr;
}


//-------------------------------------------------------------------
// Name: DispatchProcessSample
// Description: Complete a ProcessSample or PlaceMarker request.
//-------------------------------------------------------------------

HRESULT CGmStreamSink::DispatchProcessSample(CGmAsyncOperation* pOp)
{
	HRESULT hr = S_OK, _hr;

	if (pOp == NULL)
	{
		GM_CHECK_HR(hr = E_POINTER);
	}

	_hr = ProcessSamplesFromQueue(WriteSamples);
	
	// Ask for another sample
	if (pOp->m_op == OpProcessSample)
	{
		GM_CHECK_HR(hr = QueueEvent(MEStreamSinkRequestSample, GUID_NULL, S_OK, NULL));
	}

	GM_CHECK_HR(hr = _hr);

bail:
	// We are in the middle of an asynchronous operation, so if something failed, send an error.
	if (FAILED(hr) && hr != GM_ERR_PRODUCER_NOT_STARTED) // Do not break the writer if the error is because of producer wrong state
	{
		hr = QueueEvent(MEError, GUID_NULL, hr, NULL);
	}
	return hr;
}


//-------------------------------------------------------------------
// Name: ProcessSamplesFromQueue
// Description:
//
// Removes all of the samples and markers that are currently in the
// queue and processes them.
//
// If bFlushData = DropSamples:
//     For each marker, send an MEStreamSinkMarker event, with hr = E_ABORT.
//     For each sample, drop the sample.
//
// If bFlushData = WriteSamples
//     For each marker, send an MEStreamSinkMarker event, with hr = S_OK.
//     For each sample, write the sample to the file.
//
// This method is called when we flush, stop, restart, receive a new
// sample, or receive a marker.
//-------------------------------------------------------------------

HRESULT CGmStreamSink::ProcessSamplesFromQueue(GmFlushState bFlushData)
{
	HRESULT hr = S_OK;

	ComPtrList<IUnknown>::POSITION pos = m_SampleQueue.FrontPosition();

	// Enumerate all of the samples/markers in the queue.

	while (pos != m_SampleQueue.EndPosition())
	{
		IUnknown *pUnk = NULL;
		IGmMarker  *pMarker = NULL;
		IMFSample *pSample = NULL;

		hr = m_SampleQueue.GetItemByPosition(pos, &pUnk);

		assert(pUnk != NULL); // GetItemByPosition should not fail unless we reached the end of the list.

		// Figure out if this is a marker or a sample.
		if (SUCCEEDED(hr))
		{
			hr = pUnk->QueryInterface(__uuidof(IGmMarker), (void**)&pMarker);
			if (hr == E_NOINTERFACE)
			{
				// If this is a sample, send it over the network.
				hr = pUnk->QueryInterface(IID_IMFSample, (void**)&pSample);
			}
		}

		// Now handle the sample/marker appropriately.
		if (SUCCEEDED(hr))
		{
			if (pMarker)
			{
				hr = SendMarkerEvent(pMarker, bFlushData);
			}
			else
			{
				assert(pSample != NULL);    // Not a marker, must be a sample
				if (bFlushData == WriteSamples)
				{
					hr = SendSampleOverNetwork(pSample);
				}
			}
		}
		GmSafeRelease(&pUnk);
		GmSafeRelease(&pMarker);
		GmSafeRelease(&pSample);

		if (FAILED(hr))
		{
			break;
		}

		pos = m_SampleQueue.Next(pos);

	}       // while loop

	// Now clear the list.
	m_SampleQueue.Clear();

	return hr;
}


//-------------------------------------------------------------------
// Name: WriteSampleToFile
// Description: Encode and send the sample over the network (RTP).
//-------------------------------------------------------------------

HRESULT CGmStreamSink::SendSampleOverNetwork(IMFSample *pSample)
{
	HRESULT hr = S_OK, _hr;
	LONGLONG time = 0;
	BYTE *pData = NULL;
	DWORD cbData = 0;
	DWORD cbWritten = 0;

	IMFMediaBuffer *pBuffer = NULL;

	// Get the time stamp
	GM_CHECK_HR(hr = pSample->GetSampleTime(&time));
	
	// If the time stamp is too early, just discard this sample.
	if (time < m_StartTime)
	{
		GM_DEBUG_INFO("Too early");
		return S_OK;
	}

	// Note: If there is no time stamp on the sample, proceed anyway.

	GM_CHECK_HR(hr = pSample->ConvertToContiguousBuffer(&pBuffer));

	// Lock the buffer and write the data to the file.
	GM_CHECK_HR(hr = pBuffer->Lock(&pData, NULL, &cbData));
	
	// SendOverNetwork(pData, cbData, &cbWritten);
	_hr = m_pSink->m_objCall->WriteSamples(pData, (UINT32)cbData);
	if (SUCCEEDED(_hr))
	{
		cbWritten = cbData;
	}

	GM_CHECK_HR(hr = pBuffer->Unlock());

	GM_CHECK_HR(hr = _hr); // after the unlock()

	// Update the running tally of bytes written.
	m_cbDataWritten += cbData;

bail:
	GmSafeRelease(&pBuffer);
	return hr;
}

//-------------------------------------------------------------------
// Name: SendMarkerEvent
// Description: Saned a marker event.
//
// pMarker: Pointer to our custom IMarker interface, which holds
//          the marker information.
//-------------------------------------------------------------------

HRESULT CGmStreamSink::SendMarkerEvent(IGmMarker *pMarker, GmFlushState FlushState)
{
	HRESULT hr = S_OK;
	HRESULT hrStatus = S_OK;  // Status code for marker event.
	PROPVARIANT var = {0};


	if (FlushState == DropSamples)
	{
		GM_CHECK_HR(hrStatus = E_ABORT);
	}
	
	PropVariantInit(&var);

	// Get the context data.
	GM_CHECK_HR(hr = pMarker->GetContext(&var));
	
	GM_CHECK_HR(hr = QueueEvent(MEStreamSinkMarker, GUID_NULL, hrStatus, &var));

bail:
	PropVariantClear(&var);
	return hr;
}


//-------------------------------------------------------------------
// Name: DispatchFinalize
// Description: Complete a BeginFinalize request.
//-------------------------------------------------------------------

HRESULT CGmStreamSink::DispatchFinalize(CGmAsyncOperation* pOp)
{
	HRESULT hr = S_OK;

	// Send any samples left in the queue...
	hr = ProcessSamplesFromQueue(WriteSamples);

	// Now we're done writing all of the audio data.

	// Fill in the RIFF headers...
	if (SUCCEEDED(hr))
	{
		// TODO: e.g. destroy producer
	}

	// Set the async status and invoke the callback.
	m_pFinalizeResult->SetStatus(hr);
	hr = MFInvokeCallback(m_pFinalizeResult);

	return hr;
}

STDMETHODIMP CGmStreamSink::SetString(REFGUID guidKey, LPCWSTR wszValue)
{
	return E_FAIL;
}





//-------------------------------------------------------------------
// IGmStreamWebRTC interface implementation
//-------------------------------------------------------------------


STDMETHODIMP CGmStreamSink::Test()
{
	GM_DEBUG_INFO("CGmStreamSink::Test()");
	return S_OK;
}




//////////////////////
// CGmMarker class
// Holds information from IMFStreamSink::PlaceMarker
//

CGmMarker::CGmMarker(MFSTREAMSINK_MARKER_TYPE eMarkerType) : m_nRefCount(1), m_eMarkerType(eMarkerType)
{
	PropVariantInit(&m_varMarkerValue);
	PropVariantInit(&m_varContextValue);
}

CGmMarker::~CGmMarker()
{
	assert(m_nRefCount == 0);

	PropVariantClear(&m_varMarkerValue);
	PropVariantClear(&m_varContextValue);
}

/* static */
HRESULT CGmMarker::Create(
	MFSTREAMSINK_MARKER_TYPE eMarkerType,
	const PROPVARIANT* pvarMarkerValue,     // Can be NULL.
	const PROPVARIANT* pvarContextValue,    // Can be NULL.
	IGmMarker **ppMarker
	)
{
	HRESULT hr = S_OK;
	CGmMarker *pMarker = NULL;

	if (ppMarker == NULL)
	{
		GM_CHECK_HR(hr = E_POINTER);
	}
	
	pMarker = new CGmMarker(eMarkerType);
	if (pMarker == NULL)
	{
		GM_CHECK_HR(hr = E_OUTOFMEMORY);
	}

	// Copy the marker data.
	if (pvarMarkerValue)
	{
		GM_CHECK_HR(hr = PropVariantCopy(&pMarker->m_varMarkerValue, pvarMarkerValue));
	}

	if (pvarContextValue)
	{
		GM_CHECK_HR(hr = PropVariantCopy(&pMarker->m_varContextValue, pvarContextValue));
	}
	
	*ppMarker = pMarker;
	(*ppMarker)->AddRef();
	
bail:
	GmSafeRelease(&pMarker);
	return hr;
}

// IUnknown methods.

ULONG CGmMarker::AddRef()
{
	return InterlockedIncrement(&m_nRefCount);
}

ULONG CGmMarker::Release()
{
	ULONG uCount = InterlockedDecrement(&m_nRefCount);
	if (uCount == 0)
	{
		delete this;
	}
	// For thread safety, return a temporary variable.
	return uCount;
}

HRESULT CGmMarker::QueryInterface(REFIID iid, void** ppv)
{
	if (!ppv)
	{
		return E_POINTER;
	}
	if (iid == IID_IUnknown)
	{
		*ppv = static_cast<IUnknown*>(this);
	}
	else if (iid == __uuidof(IGmMarker))
	{
		*ppv = static_cast<IGmMarker*>(this);
	}
	else
	{
		*ppv = NULL;
		return E_NOINTERFACE;
	}
	AddRef();
	return S_OK;
}

// IMarker methods
HRESULT CGmMarker::GetMarkerType(MFSTREAMSINK_MARKER_TYPE *pType)
{
	HRESULT hr = S_OK;
	if (pType == NULL)
	{
		GM_CHECK_HR(hr = E_POINTER);
	}
	*pType = m_eMarkerType;
bail:
	return hr;
}

HRESULT CGmMarker::GetMarkerValue(PROPVARIANT *pvar)
{
	HRESULT hr = S_OK;
	if (pvar == NULL)
	{
		GM_CHECK_HR(hr = E_POINTER);
	}
	
	GM_CHECK_HR(hr = PropVariantCopy(pvar, &m_varMarkerValue));
bail:
	return hr;
}
HRESULT CGmMarker::GetContext(PROPVARIANT *pvar)
{
	HRESULT hr = S_OK;

	if (pvar == NULL)
	{
		GM_CHECK_HR(hr = E_POINTER);
	}
	GM_CHECK_HR(hr = PropVariantCopy(pvar, &m_varContextValue));
bail:
	return hr;
}

#pragma warning( pop )



