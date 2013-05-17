/*
* Copyright (C) 2013 Gotham City. All rights reserved.
* Copyright (C) Microsoft Corporation. All rights reserved.
*/
/**@file w4allSink.cc
 * @brief Audio/Video Media Foundation Sink (source)
 *
 * @author Batman@GothamCity
 */

#include "w4allSink.h"

#include <aviriff.h>

#include <uuids.h>      // DirectShow GUIDs

#pragma warning( push )
#pragma warning( disable : 4355 )  // 'this' used in base member initializer list

// FIXME: to be removed
#if !defined(W4A_IS_VIDEO)
#	define W4A_IS_VIDEO		true
#endif


//-------------------------------------------------------------------
// Name: CreateWavSink
// Description: Creates an instance of the WavSink object.
//
// To use the WavSink, include this header file in an application
// and link to the library file created by this project.
//
// ppSink:  Receives a pointer to the IMFMediaSink interface. The
//          caller must release the interface.
//-------------------------------------------------------------------

HRESULT CreateWavSink(IMFMediaSink **ppSink)
{
    return CW4allSink::CreateInstance(IID_IMFMediaSink, (void**)ppSink);
}


// The stream ID of the one stream on the sink.
const DWORD W4A_SINK_STREAM_ID = 1;


// Video FOURCC codes.
const FOURCC FOURCC_YUY2 = MAKEFOURCC('Y', 'U', 'Y', '2');
const FOURCC FOURCC_UYVY = MAKEFOURCC('U', 'Y', 'V', 'Y');
const FOURCC FOURCC_NV12 = MAKEFOURCC('N', 'V', '1', '2');

// Static array of media types (preferred and accepted).
const GUID* g_VideoSubtypes[] =
{
    & MEDIASUBTYPE_NV12,
    & MEDIASUBTYPE_YUY2,
    & MEDIASUBTYPE_UYVY
};

// Number of media types in the aray.
DWORD g_NumVideoSubtypes = ARRAYSIZE(g_VideoSubtypes);

// PCM_Audio_Format_Params
// Defines parameters for uncompressed PCM audio formats.
// The remaining fields can be derived from these.
struct PCM_Audio_Format_Params
{
    DWORD   nSamplesPerSec; // Samples per second.
    WORD    wBitsPerSample; // Bits per sample.
    WORD    nChannels;      // Number of channels.
};


// g_AudioFormats: Static list of our preferred formats.

// This is an ordered list that we use to hand out formats in the
// stream's IMFMediaTypeHandler::GetMediaTypeByIndex method. The
// stream will accept other bit rates not listed here.

PCM_Audio_Format_Params g_AudioFormats[] =
{
	{ 8000, 16, 1 },

    { 48000, 16, 2 },
    { 48000, 8, 2 },
    { 44100, 16, 2 },
    { 44100, 8, 2 },
    { 22050, 16, 2 },
    { 22050, 8, 2 },

    { 48000, 16, 1 },
    { 48000, 8, 1 },
    { 44100, 16, 1 },
    { 44100, 8, 1 },
    { 22050, 16, 1 },
    { 22050, 8, 1 },
};

DWORD g_NumAudioFormats = ARRAYSIZE(g_AudioFormats);

// Forward declares
HRESULT ValidateWaveFormat(const WAVEFORMATEX *pWav, DWORD cbSize);
HRESULT ValidateVideoFormat(IMFMediaType *pmt);

HRESULT CreatePCMAudioType(
    UINT32 sampleRate,        // Samples per second
    UINT32 bitsPerSample,     // Bits per sample
    UINT32 cChannels,         // Number of channels
    IMFMediaType **ppType     // Receives a pointer to the media type.
    );

HRESULT CreateVideoType(
		const GUID* subType, // video subType
		IMFMediaType **ppType     // Receives a pointer to the media type.
	);

/////////////////////////////////////////////////////////////////////////////////////////////
//
// CW4allSink class. - Implements the media sink.
//
// Notes:
// - Most public methods calls CheckShutdown. This method fails if the sink was shut down.
//
/////////////////////////////////////////////////////////////////////////////////////////////


HRESULT CW4allSink_CreateInstance(REFIID iid, void **ppMFT)
{
	return CW4allSink::CreateInstance(iid, ppMFT);
}

//-------------------------------------------------------------------
// Name: CreateInstance
// Description: Creates an instance of the WavSink object.
// [See CreateWavSink]
//-------------------------------------------------------------------

/* static */ HRESULT CW4allSink::CreateInstance(REFIID iid, void **ppSink)
{
    if (ppSink == NULL)
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    CW4allSink *pSink = new CW4allSink(W4A_IS_VIDEO);   // Created with ref count = 1.

    if (pSink == NULL)
    {
        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        hr = pSink->Initialize();
    }

    if (SUCCEEDED(hr))
    {
        hr = pSink->QueryInterface(iid, ppSink);
    }

    SafeRelease(&pSink);

    return hr;
}


//-------------------------------------------------------------------
// CW4allSink constructor.
//-------------------------------------------------------------------

CW4allSink::CW4allSink(bool bVideo) :
    m_nRefCount(1), m_IsShutdown(FALSE), m_pStream(NULL), m_pClock(NULL), m_bVideo(bVideo)
{
    InitializeCriticalSection(&m_critSec);
}

//-------------------------------------------------------------------
// CW4allSink destructor.
//-------------------------------------------------------------------

CW4allSink::~CW4allSink()
{
    assert(m_IsShutdown);

    DeleteCriticalSection(&m_critSec);
}

// IUnknown methods

ULONG CW4allSink::AddRef()
{
    return InterlockedIncrement(&m_nRefCount);
}

ULONG  CW4allSink::Release()
{
    ULONG uCount = InterlockedDecrement(&m_nRefCount);
    if (uCount == 0)
    {
        delete this;
    }
    // For thread safety, return a temporary variable.
    return uCount;
}

HRESULT CW4allSink::QueryInterface(REFIID iid, void** ppv)
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

HRESULT CW4allSink::GetCharacteristics(DWORD *pdwCharacteristics)
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

HRESULT CW4allSink::AddStreamSink(
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

HRESULT CW4allSink::RemoveStreamSink(DWORD dwStreamSinkIdentifier)
{
    return MF_E_STREAMSINKS_FIXED;
}


//-------------------------------------------------------------------
// Name: GetStreamSinkCount
// Description: Returns the number of streams.
//-------------------------------------------------------------------

HRESULT CW4allSink::GetStreamSinkCount(DWORD *pcStreamSinkCount)
{
    if (pcStreamSinkCount == NULL)
    {
        return E_INVALIDARG;
    }

    EnterCriticalSection(&m_critSec);

    HRESULT hr = CheckShutdown();

    if (SUCCEEDED(hr))
    {
        *pcStreamSinkCount = 1;  // Fixed number of streams.
    }

    LeaveCriticalSection(&m_critSec);
    return hr;
}


//-------------------------------------------------------------------
// Name: GetStreamSinkByIndex
// Description: Retrieves a stream by index.
//-------------------------------------------------------------------

HRESULT CW4allSink::GetStreamSinkByIndex(
    DWORD dwIndex,
    IMFStreamSink **ppStreamSink)
{
    if (ppStreamSink == NULL)
    {
        return E_INVALIDARG;
    }

    // Fixed stream: Index 0.
    if (dwIndex > 0)
    {
        return MF_E_INVALIDINDEX;
    }

    EnterCriticalSection(&m_critSec);

    HRESULT hr = CheckShutdown();

    if (SUCCEEDED(hr))
    {
        *ppStreamSink = m_pStream;
        (*ppStreamSink)->AddRef();
    }

    LeaveCriticalSection(&m_critSec);
    return hr;
}


//-------------------------------------------------------------------
// Name: GetStreamSinkById
// Description: Retrieves a stream by ID.
//-------------------------------------------------------------------

HRESULT CW4allSink::GetStreamSinkById(
    DWORD dwStreamSinkIdentifier,
    IMFStreamSink **ppStreamSink)
{
    if (ppStreamSink == NULL)
    {
        return E_INVALIDARG;
    }

    // Fixed stream ID.
    if (dwStreamSinkIdentifier != W4A_SINK_STREAM_ID)
    {
        return MF_E_INVALIDSTREAMNUMBER;
    }

    EnterCriticalSection(&m_critSec);

    HRESULT hr = CheckShutdown();

    if (SUCCEEDED(hr))
    {
        *ppStreamSink = m_pStream;
        (*ppStreamSink)->AddRef();
    }

    LeaveCriticalSection(&m_critSec);
    return hr;
}


//-------------------------------------------------------------------
// Name: SetPresentationClock
// Description: Sets the presentation clock.
//
// pPresentationClock: Pointer to the clock. Can be NULL.
//-------------------------------------------------------------------

HRESULT CW4allSink::SetPresentationClock(IMFPresentationClock *pPresentationClock)
{
    EnterCriticalSection(&m_critSec);

    HRESULT hr = CheckShutdown();

    // If we already have a clock, remove ourselves from that clock's
    // state notifications.
    if (SUCCEEDED(hr))
    {
        if (m_pClock)
        {
            hr = m_pClock->RemoveClockStateSink(this);
        }
    }

    // Register ourselves to get state notifications from the new clock.
    if (SUCCEEDED(hr))
    {
        if (pPresentationClock)
        {
            hr = pPresentationClock->AddClockStateSink(this);
        }
    }

    if (SUCCEEDED(hr))
    {
        // Release the pointer to the old clock.
        // Store the pointer to the new clock.

        SafeRelease(&m_pClock);
        m_pClock = pPresentationClock;
        if (m_pClock)
        {
            m_pClock->AddRef();
        }
    }

    LeaveCriticalSection(&m_critSec);
    return hr;
}


//-------------------------------------------------------------------
// Name: GetPresentationClock
// Description: Returns a pointer to the presentation clock.
//-------------------------------------------------------------------

HRESULT CW4allSink::GetPresentationClock(IMFPresentationClock **ppPresentationClock)
{
    if (ppPresentationClock == NULL)
    {
        return E_INVALIDARG;
    }

    EnterCriticalSection(&m_critSec);

    HRESULT hr = CheckShutdown();

    if (SUCCEEDED(hr))
    {
        if (m_pClock == NULL)
        {
            hr = MF_E_NO_CLOCK; // There is no presentation clock.
        }
        else
        {
            // Return the pointer to the caller.
            *ppPresentationClock = m_pClock;
            (*ppPresentationClock)->AddRef();
        }
    }

    LeaveCriticalSection(&m_critSec);
    return hr;
}


//-------------------------------------------------------------------
// Name: Shutdown
// Description: Releases resources held by the media sink.
//-------------------------------------------------------------------

HRESULT CW4allSink::Shutdown()
{
    EnterCriticalSection(&m_critSec);

    HRESULT hr = CheckShutdown();

    if (SUCCEEDED(hr))
    {
        hr = m_pStream->Shutdown();

        SafeRelease(&m_pClock);
        SafeRelease(&m_pStream);

        m_IsShutdown = true;
    }

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

HRESULT CW4allSink::BeginFinalize(
    IMFAsyncCallback *pCallback,
    IUnknown *punkState)
{
    EnterCriticalSection(&m_critSec);

    HRESULT hr = CheckShutdown();

    // Tell the stream to finalize.
    if (SUCCEEDED(hr))
    {
        hr = m_pStream->Finalize(pCallback, punkState);
    }
    LeaveCriticalSection(&m_critSec);
    return hr;
}


//-------------------------------------------------------------------
// Name: EndFinalize
// Description: Completes the asynchronous finalize operation.
//-------------------------------------------------------------------

HRESULT CW4allSink::EndFinalize(IMFAsyncResult *pResult)
{
    HRESULT hr = S_OK;

    // Return the status code from the async result.
    if (pResult == NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = pResult->GetStatus();
    }

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

HRESULT CW4allSink::OnClockStart(
        /* [in] */ MFTIME hnsSystemTime,
        /* [in] */ LONGLONG llClockStartOffset)
{
    EnterCriticalSection(&m_critSec);

    HRESULT hr = CheckShutdown();

    if (SUCCEEDED(hr))
    {
        hr = m_pStream->Start(llClockStartOffset);
    }

    LeaveCriticalSection(&m_critSec);
    return hr;
}

//-------------------------------------------------------------------
// Name: OnClockStop
// Description: Called when the presentation clock stops.
//
// Note: After this method is called, we stop accepting new data.
//-------------------------------------------------------------------

HRESULT CW4allSink::OnClockStop(
        /* [in] */ MFTIME hnsSystemTime)
{
    EnterCriticalSection(&m_critSec);

    HRESULT hr = CheckShutdown();

    if (SUCCEEDED(hr))
    {
        hr = m_pStream->Stop();
    }

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

HRESULT CW4allSink::OnClockPause(
        /* [in] */ MFTIME hnsSystemTime)
{
    EnterCriticalSection(&m_critSec);

    HRESULT hr = CheckShutdown();

    if (SUCCEEDED(hr))
    {
        hr = m_pStream->Pause();
    }

    LeaveCriticalSection(&m_critSec);
    return hr;
}


//-------------------------------------------------------------------
// Name: OnClockRestart
// Description: Called when the presentation clock restarts.
//-------------------------------------------------------------------

HRESULT CW4allSink::OnClockRestart(
        /* [in] */ MFTIME hnsSystemTime)
{
    EnterCriticalSection(&m_critSec);

    HRESULT hr = CheckShutdown();

    if (SUCCEEDED(hr))
    {
        hr = m_pStream->Restart();
    }

    LeaveCriticalSection(&m_critSec);
    return hr;
}


//-------------------------------------------------------------------
// Name: OnClockSetRate
// Description: Called when the presentation clock's rate changes.
//
// Note: For a rateless sink, the clock rate is not important.
//-------------------------------------------------------------------

HRESULT CW4allSink::OnClockSetRate(
        /* [in] */ MFTIME hnsSystemTime,
        /* [in] */ float flRate)
{
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

HRESULT CW4allSink::Initialize()
{
    HRESULT hr = S_OK;

    m_pStream = new CW4allStream(m_bVideo);
    if (m_pStream == NULL)
    {
        hr = E_OUTOFMEMORY;
    }

    // Initialize the stream.
    if (SUCCEEDED(hr))
    {
        hr = m_pStream->Initialize(this);
    }
    return hr;
}



/////////////////////////////////////////////////////////////////////////////////////////////
//
// CAsyncOperation class. - Private class used by CW4allStream class.
//
/////////////////////////////////////////////////////////////////////////////////////////////

CW4allStream::CAsyncOperation::CAsyncOperation(StreamOperation op)
    : m_nRefCount(1), m_op(op)
{
}

CW4allStream::CAsyncOperation::~CAsyncOperation()
{
    assert(m_nRefCount == 0);
}

ULONG CW4allStream::CAsyncOperation::AddRef()
{
    return InterlockedIncrement(&m_nRefCount);
}

ULONG CW4allStream::CAsyncOperation::Release()
{
    ULONG uCount = InterlockedDecrement(&m_nRefCount);
    if (uCount == 0)
    {
        delete this;
    }
    // For thread safety, return a temporary variable.
    return uCount;
}

HRESULT CW4allStream::CAsyncOperation::QueryInterface(REFIID iid, void** ppv)
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
// CW4allStream class. - Implements the stream sink.
//
// Notes:
// - Most of the real work gets done in this class.
// - The sink has one stream. If it had multiple streams, it would need to coordinate them.
// - Most operations are done asynchronously on a work queue.
// - Async methods are handled like this:
//      1. Call ValidateOperation to check if the operation is permitted at this time
//      2. Create an CAsyncOperation object for the operation.
//      3. Call QueueAsyncOperation. This puts the operation on the work queue.
//      4. The workqueue calls OnDispatchWorkItem.
// - Locking:
//      To avoid deadlocks, do not hold the CW4allStream lock followed by the CW4allSink lock.
//      The other order is OK (CW4allSink, then CW4allStream).
//
/////////////////////////////////////////////////////////////////////////////////////////////


//-------------------------------------------------------------------
// CW4allStream constructor
//-------------------------------------------------------------------

CW4allStream::CW4allStream(bool bVideo)
    : m_nRefCount(1), m_state(State_TypeNotSet), m_IsShutdown(FALSE), m_bVideo(bVideo),
    m_pSink(NULL), m_pEventQueue(NULL),
    m_pCurrentType(NULL), m_pFinalizeResult(NULL),
    m_StartTime(0), m_cbDataWritten(0), m_WorkQueueId(0),
    m_WorkQueueCB(this, &CW4allStream::OnDispatchWorkItem)
{
    InitializeCriticalSection(&m_critSec);
}


//-------------------------------------------------------------------
// CW4allStream destructor
//-------------------------------------------------------------------

CW4allStream::~CW4allStream()
{
    assert(m_IsShutdown);
    DeleteCriticalSection(&m_critSec);
}


// IUnknown methods

ULONG CW4allStream::AddRef()
{
    return InterlockedIncrement(&m_nRefCount);
}

ULONG  CW4allStream::Release()
{
    ULONG uCount = InterlockedDecrement(&m_nRefCount);
    if (uCount == 0)
    {
        delete this;
    }
    // For thread safety, return a temporary variable.
    return uCount;
}

HRESULT CW4allStream::QueryInterface(REFIID iid, void** ppv)
{
    if (!ppv)
    {
        return E_POINTER;
    }
    if (iid == IID_IUnknown)
    {
        *ppv = static_cast<IUnknown*>(static_cast<IMFStreamSink*>(this));
    }
    else if (iid == __uuidof(IMFStreamSink ))
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

HRESULT CW4allStream::BeginGetEvent(IMFAsyncCallback* pCallback, IUnknown* punkState)
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

HRESULT CW4allStream::EndGetEvent(IMFAsyncResult* pResult, IMFMediaEvent** ppEvent)
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

HRESULT CW4allStream::GetEvent(DWORD dwFlags, IMFMediaEvent** ppEvent)
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

    SafeRelease(&pQueue);

    return hr;
}

HRESULT CW4allStream::QueueEvent(MediaEventType met, REFGUID guidExtendedType, HRESULT hrStatus, const PROPVARIANT* pvValue)
{
    HRESULT hr = S_OK;

    EnterCriticalSection(&m_critSec);

    hr = CheckShutdown();

    if (SUCCEEDED(hr))
    {
        hr = m_pEventQueue->QueueEventParamVar(met, guidExtendedType, hrStatus, pvValue);
    }

    LeaveCriticalSection(&m_critSec);
    return hr;
}




/// IMFStreamSink methods


//-------------------------------------------------------------------
// Name: GetMediaSink
// Description: Returns the parent media sink.
//-------------------------------------------------------------------

HRESULT CW4allStream::GetMediaSink(IMFMediaSink **ppMediaSink)
{
    if (ppMediaSink == NULL)
    {
        return E_INVALIDARG;
    }

    EnterCriticalSection(&m_critSec);

    HRESULT hr = CheckShutdown();

    if (SUCCEEDED(hr))
    {
        *ppMediaSink = (IMFMediaSink*)m_pSink;
        (*ppMediaSink)->AddRef();
    }

    LeaveCriticalSection(&m_critSec);
    return hr;
}


//-------------------------------------------------------------------
// Name: GetIdentifier
// Description: Returns the stream identifier.
//-------------------------------------------------------------------

HRESULT CW4allStream::GetIdentifier(DWORD *pdwIdentifier)
{
    if (pdwIdentifier == NULL)
    {
        return E_INVALIDARG;
    }

    EnterCriticalSection(&m_critSec);

    HRESULT hr = CheckShutdown();

    if (SUCCEEDED(hr))
    {
        *pdwIdentifier = W4A_SINK_STREAM_ID;
    }

    LeaveCriticalSection(&m_critSec);
    return hr;
}


//-------------------------------------------------------------------
// Name: GetMediaTypeHandler
// Description: Returns a media type handler for this stream.
//-------------------------------------------------------------------

HRESULT CW4allStream::GetMediaTypeHandler(IMFMediaTypeHandler **ppHandler)
{
    if (ppHandler == NULL)
    {
        return E_INVALIDARG;
    }

    EnterCriticalSection(&m_critSec);

    HRESULT hr = CheckShutdown();

    // This stream object acts as its own type handler, so we QI ourselves.
    if (SUCCEEDED(hr))
    {
        hr = this->QueryInterface(IID_IMFMediaTypeHandler, (void**)ppHandler);
    }
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

HRESULT CW4allStream::ProcessSample(IMFSample *pSample)
{
    if (pSample == NULL)
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    EnterCriticalSection(&m_critSec);

    hr = CheckShutdown();

    // Validate the operation.
    if (SUCCEEDED(hr))
    {
        hr = ValidateOperation(OpProcessSample);
    }

    // Add the sample to the sample queue.
    if (SUCCEEDED(hr))
    {
        hr = m_SampleQueue.InsertBack(pSample);
    }

    // Unless we are paused, start an async operation to dispatch the next sample.
    if (SUCCEEDED(hr))
    {
        if (m_state != State_Paused)
        {
            // Queue the operation.
            hr = QueueAsyncOperation(OpProcessSample);
        }
    }

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

HRESULT CW4allStream::PlaceMarker(
    MFSTREAMSINK_MARKER_TYPE eMarkerType,
    const PROPVARIANT *pvarMarkerValue,
    const PROPVARIANT *pvarContextValue)
{
    EnterCriticalSection(&m_critSec);

    HRESULT hr = S_OK;

    IMarker *pMarker = NULL;

    hr = CheckShutdown();

    if (SUCCEEDED(hr))
    {
        hr = ValidateOperation(OpPlaceMarker);
    }

    // Create a marker object and put it on the sample queue.
    if (SUCCEEDED(hr))
    {
        hr = CW4allMarker::Create(
            eMarkerType,
            pvarMarkerValue,
            pvarContextValue,
            &pMarker);
    }

    if (SUCCEEDED(hr))
    {
        hr = m_SampleQueue.InsertBack(pMarker);
    }

    // Unless we are paused, start an async operation to dispatch the next sample/marker.
    if (SUCCEEDED(hr))
    {
        if (m_state != State_Paused)
        {
            // Queue the operation.
            hr = QueueAsyncOperation(OpPlaceMarker); // Increments ref count on pOp.
        }
    }

    SafeRelease(&pMarker);
    LeaveCriticalSection(&m_critSec);
    return hr;
}


//-------------------------------------------------------------------
// Name: Flush
// Description: Discards all samples that were not processed yet.
//-------------------------------------------------------------------

HRESULT CW4allStream::Flush()
{
    EnterCriticalSection(&m_critSec);

    HRESULT hr = CheckShutdown();

    if (SUCCEEDED(hr))
    {
        // Note: Even though we are flushing data, we still need to send
        // any marker events that were queued.
        hr = ProcessSamplesFromQueue(DropSamples);
    }

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


HRESULT CW4allStream::IsMediaTypeSupported(
    /* [in] */ IMFMediaType *pMediaType,
    /* [out] */ IMFMediaType **ppMediaType)
{
    if (pMediaType == NULL)
    {
        return E_INVALIDARG;
    }

    EnterCriticalSection(&m_critSec);

    GUID majorType = GUID_NULL;

    HRESULT hr = CheckShutdown();

    if (SUCCEEDED(hr))
    {
        hr = pMediaType->GetGUID(MF_MT_MAJOR_TYPE, &majorType);
    }

    // First make sure it's audio.
    if (SUCCEEDED(hr))
    {
		if(m_bVideo)
		{
			if (majorType != MFMediaType_Video)
			{
				hr = MF_E_INVALIDTYPE;
			}
		}
		else
		{
			if (majorType != MFMediaType_Audio)
			{
				hr = MF_E_INVALIDTYPE;
			}
		}
    }

	if(m_bVideo)
	{
		hr = ValidateVideoFormat(pMediaType);
	}
	else
	{
		WAVEFORMATEX *pWav = NULL;
		UINT cbSize = 0;

		// Get a WAVEFORMATEX structure to validate against.
		if (SUCCEEDED(hr))
		{
			hr = MFCreateWaveFormatExFromMFMediaType(pMediaType, &pWav, &cbSize);
		}

		// Validate the WAVEFORMATEX structure.
		if (SUCCEEDED(hr))
		{
			hr = ValidateWaveFormat(pWav, cbSize);
		}

		CoTaskMemFree(pWav);
	}

    // We don't return any "close match" types.
    if (ppMediaType)
    {
        *ppMediaType = NULL;
    }
    
    LeaveCriticalSection(&m_critSec);
    return hr;
}


//-------------------------------------------------------------------
// Name: GetMediaTypeCount
// Description: Return the number of preferred media types.
//-------------------------------------------------------------------

HRESULT CW4allStream::GetMediaTypeCount(DWORD *pdwTypeCount)
{
    if (pdwTypeCount == NULL)
    {
        return E_INVALIDARG;
    }

    EnterCriticalSection(&m_critSec);

    HRESULT hr = CheckShutdown();

    if (SUCCEEDED(hr))
    {
		*pdwTypeCount = m_bVideo ? g_NumVideoSubtypes :  g_NumAudioFormats;
    }

    LeaveCriticalSection(&m_critSec);
    return hr;
}


//-------------------------------------------------------------------
// Name: GetMediaTypeByIndex
// Description: Return a preferred media type by index.
//-------------------------------------------------------------------

HRESULT CW4allStream::GetMediaTypeByIndex(
    /* [in] */ DWORD dwIndex,
    /* [out] */ IMFMediaType **ppType)
{
    if (ppType == NULL)
    {
        return E_INVALIDARG;
    }

    EnterCriticalSection(&m_critSec);

    HRESULT hr = CheckShutdown();

    if (SUCCEEDED(hr))
    {
		if(m_bVideo)
		{
			if(dwIndex >= g_NumVideoSubtypes)
			{
				hr = MF_E_NO_MORE_TYPES;
			}
		}
		else
		{
			if (dwIndex >= g_NumAudioFormats)
			{
				hr = MF_E_NO_MORE_TYPES;
			}
		}
    }

    if (SUCCEEDED(hr))
    {
		if(m_bVideo)
		{
			const GUID* subType = g_VideoSubtypes[dwIndex];
			hr = CreateVideoType(subType, ppType);
		}
		else
		{
			const DWORD   nSamplesPerSec = g_AudioFormats[dwIndex].nSamplesPerSec;
			const WORD    wBitsPerSample = g_AudioFormats[dwIndex].wBitsPerSample;
			const WORD    nChannels = g_AudioFormats[dwIndex].nChannels;

			hr = CreatePCMAudioType(nSamplesPerSec, wBitsPerSample, nChannels, ppType);
		}
    }

    LeaveCriticalSection(&m_critSec);
    return hr;
}


//-------------------------------------------------------------------
// Name: SetCurrentMediaType
// Description: Set the current media type.
//-------------------------------------------------------------------

HRESULT CW4allStream::SetCurrentMediaType(IMFMediaType *pMediaType)
{
    if (pMediaType == NULL)
    {
        return E_INVALIDARG;
    }

    EnterCriticalSection(&m_critSec);

    HRESULT hr = CheckShutdown();

    // We don't allow format changes after streaming starts,
    // because this would invalidate the .wav file.
    if (SUCCEEDED(hr))
    {
        hr = ValidateOperation(OpSetMediaType);
    }

    if (SUCCEEDED(hr))
    {
        hr = IsMediaTypeSupported(pMediaType, NULL);
    }

    if (SUCCEEDED(hr))
    {
        SafeRelease(&m_pCurrentType);
        m_pCurrentType = pMediaType;
        m_pCurrentType->AddRef();

        m_state = State_Ready;
    }

    LeaveCriticalSection(&m_critSec);
    return hr;
}

//-------------------------------------------------------------------
// Name: GetCurrentMediaType
// Description: Return the current media type, if any.
//-------------------------------------------------------------------

HRESULT CW4allStream::GetCurrentMediaType(IMFMediaType **ppMediaType)
{
    if (ppMediaType == NULL)
    {
        return E_INVALIDARG;
    }

    EnterCriticalSection(&m_critSec);

    HRESULT hr = CheckShutdown();

    if (SUCCEEDED(hr))
    {
        if (m_pCurrentType == NULL)
        {
            hr = MF_E_NOT_INITIALIZED;
        }
    }

    if (SUCCEEDED(hr))
    {
        *ppMediaType = m_pCurrentType;
        (*ppMediaType)->AddRef();
    }

    LeaveCriticalSection(&m_critSec);
    return hr;
}


//-------------------------------------------------------------------
// Name: GetMajorType
// Description: Return the major type GUID.
//-------------------------------------------------------------------

HRESULT CW4allStream::GetMajorType(GUID *pguidMajorType)
{
    if (pguidMajorType == NULL)
    {
        return E_INVALIDARG;
    }

	*pguidMajorType = m_bVideo ? MFMediaType_Video : MFMediaType_Audio;

    return S_OK;
}


// private methods



//-------------------------------------------------------------------
// Name: Initialize
// Description: Initializes the stream sink.
//
// Note: This method is called once when the media sink is first
//       initialized.
//-------------------------------------------------------------------

HRESULT CW4allStream::Initialize(CW4allSink *pParent)
{
    assert(pParent != NULL);

    HRESULT hr = S_OK;

    // Create the event queue helper.
    if (SUCCEEDED(hr))
    {
        hr = MFCreateEventQueue(&m_pEventQueue);
    }

    // Allocate a new work queue for async operations.
    if (SUCCEEDED(hr))
    {
        hr = MFAllocateWorkQueue(&m_WorkQueueId);
    }

    if (SUCCEEDED(hr))
    {
        m_pSink = pParent;
        m_pSink->AddRef();
    }

    return hr;
}


//-------------------------------------------------------------------
// Name: Start
// Description: Called when the presentation clock starts.
//
// Note: Start time can be PRESENTATION_CURRENT_POSITION meaning
//       resume from the last current position.
//-------------------------------------------------------------------

HRESULT CW4allStream::Start(MFTIME start)
{
    EnterCriticalSection(&m_critSec);

    HRESULT hr = S_OK;

    hr = ValidateOperation(OpStart);

    if (SUCCEEDED(hr))
    {
        if (start != PRESENTATION_CURRENT_POSITION)
        {
            m_StartTime = start;        // Cache the start time.
        }
        m_state = State_Started;
        hr = QueueAsyncOperation(OpStart);
    }

    LeaveCriticalSection(&m_critSec);
    return hr;
}

//-------------------------------------------------------------------
// Name: Stop
// Description: Called when the presentation clock stops.
//-------------------------------------------------------------------

HRESULT CW4allStream::Stop()
{
    EnterCriticalSection(&m_critSec);

    HRESULT hr = S_OK;

    hr = ValidateOperation(OpStop);

    if (SUCCEEDED(hr))
    {
        m_state = State_Stopped;
        hr = QueueAsyncOperation(OpStop);
    }
    LeaveCriticalSection(&m_critSec);
    return hr;
}


//-------------------------------------------------------------------
// Name: Pause
// Description: Called when the presentation clock pauses.
//-------------------------------------------------------------------

HRESULT CW4allStream::Pause()
{
    EnterCriticalSection(&m_critSec);

    HRESULT hr = S_OK;

    hr = ValidateOperation(OpPause);

    if (SUCCEEDED(hr))
    {
        m_state = State_Paused;
        hr = QueueAsyncOperation(OpPause);
    }

    LeaveCriticalSection(&m_critSec);
    return hr;
}


//-------------------------------------------------------------------
// Name: Restart
// Description: Called when the presentation clock restarts.
//-------------------------------------------------------------------

HRESULT CW4allStream::Restart()
{
    EnterCriticalSection(&m_critSec);

    HRESULT hr = S_OK;

    hr = ValidateOperation(OpRestart);

    if (SUCCEEDED(hr))
    {
        m_state = State_Started;
        hr = QueueAsyncOperation(OpRestart);
    }

    LeaveCriticalSection(&m_critSec);
    return hr;
}


//-------------------------------------------------------------------
// Name: Finalize
// Description: Starts the async finalize operation.
//-------------------------------------------------------------------

HRESULT CW4allStream::Finalize(IMFAsyncCallback *pCallback, IUnknown *punkState)
{
    EnterCriticalSection(&m_critSec);

    HRESULT hr = S_OK;

    hr = ValidateOperation(OpFinalize);

    if (SUCCEEDED(hr))
    {
        if (m_pFinalizeResult != NULL)
        {
            hr = MF_E_INVALIDREQUEST;  // The operation is already pending.
        }
    }

    // Create and store the async result object.
    if (SUCCEEDED(hr))
    {
        hr = MFCreateAsyncResult(NULL, pCallback, punkState, &m_pFinalizeResult);
    }

    if (SUCCEEDED(hr))
    {
        m_state = State_Finalized;
        hr = QueueAsyncOperation(OpFinalize);
    }

    LeaveCriticalSection(&m_critSec);
    return hr;
}

//-------------------------------------------------------------------
// Name: ValidStateMatrix
// Description: Class-static matrix of operations vs states.
//
// If an entry is TRUE, the operation is valid from that state.
//-------------------------------------------------------------------

BOOL CW4allStream::ValidStateMatrix[CW4allStream::State_Count][CW4allStream::Op_Count] =
{
// States:    Operations:
//            SetType   Start     Restart   Pause     Stop      Sample    Marker    Finalize
/* NotSet */  TRUE,     FALSE,    FALSE,    FALSE,    FALSE,    FALSE,    FALSE,    FALSE,

/* Ready */   TRUE,     TRUE,     FALSE,    TRUE,     TRUE,     FALSE,    TRUE,     TRUE,

/* Start */   FALSE,    TRUE,     FALSE,    TRUE,     TRUE,     TRUE,     TRUE,     TRUE,

/* Pause */   FALSE,    TRUE,     TRUE,     TRUE,     TRUE,     TRUE,     TRUE,     TRUE,

/* Stop */    FALSE,    TRUE,     FALSE,    FALSE,    TRUE,     FALSE,    TRUE,     TRUE,

/* Final */   FALSE,    FALSE,    FALSE,    FALSE,    FALSE,    FALSE,    FALSE,    FALSE

// Note about states:
// 1. OnClockRestart should only be called from paused state.
// 2. While paused, the sink accepts samples but does not process them.

};


//-------------------------------------------------------------------
// Name: ValidateOperation
// Description: Checks if an operation is valid in the current state.
//-------------------------------------------------------------------

HRESULT CW4allStream::ValidateOperation(StreamOperation op)
{
    assert(!m_IsShutdown);

    HRESULT hr = S_OK;

    BOOL bTransitionAllowed = ValidStateMatrix[m_state][op];

    if (bTransitionAllowed)
    {
        return S_OK;
    }
    else
    {
        return MF_E_INVALIDREQUEST;
    }
}

//-------------------------------------------------------------------
// Name: Shutdown
// Description: Shuts down the stream sink.
//-------------------------------------------------------------------

HRESULT CW4allStream::Shutdown()
{
    assert(!m_IsShutdown);

    if (m_pEventQueue)
    {
        m_pEventQueue->Shutdown();
    }

    MFUnlockWorkQueue(m_WorkQueueId);

    m_SampleQueue.Clear();

    SafeRelease(&m_pSink);
    SafeRelease(&m_pEventQueue);
    SafeRelease(&m_pCurrentType);
    SafeRelease(&m_pFinalizeResult);

    m_IsShutdown = TRUE;

    return S_OK;
}


//-------------------------------------------------------------------
// Name: QueueAsyncOperation
// Description: Puts an async operation on the work queue.
//-------------------------------------------------------------------

HRESULT CW4allStream::QueueAsyncOperation(StreamOperation op)
{
    HRESULT hr = S_OK;
    CAsyncOperation *pOp = new CAsyncOperation(op); // Created with ref count = 1
    if (pOp == NULL)
    {
        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        hr = MFPutWorkItem(m_WorkQueueId, &m_WorkQueueCB, pOp);
    }

    SafeRelease(&pOp);

    return hr;
}



//-------------------------------------------------------------------
// Name: OnDispatchWorkItem
// Description: Callback for MFPutWorkItem.
//-------------------------------------------------------------------

HRESULT CW4allStream::OnDispatchWorkItem(IMFAsyncResult* pAsyncResult)
{
    // Called by work queue thread. Need to hold the critical section.
    EnterCriticalSection(&m_critSec);

    HRESULT hr = S_OK;

    IUnknown *pState = NULL;

    hr = pAsyncResult->GetState(&pState);

    if (SUCCEEDED(hr))
    {
        // The state object is a CAsncOperation object.
        CAsyncOperation *pOp = (CAsyncOperation*)pState;

        StreamOperation op = pOp->m_op;

        switch (op)
        {
        case OpStart:
        case OpRestart:
            // Send MEStreamSinkStarted.
            hr = QueueEvent(MEStreamSinkStarted, GUID_NULL, hr, NULL);

            // Kick things off by requesting two samples...
            if (SUCCEEDED(hr))
            {
                hr = QueueEvent(MEStreamSinkRequestSample, GUID_NULL, hr, NULL);
            }
            if (SUCCEEDED(hr))
            {
                hr = QueueEvent(MEStreamSinkRequestSample, GUID_NULL, hr, NULL);
            }

            // There might be samples queue from earlier (ie, while paused).
            if (SUCCEEDED(hr))
            {
                hr = ProcessSamplesFromQueue(WriteSamples);
            }
            break;

        case OpStop:
            // Drop samples from queue.
            hr = ProcessSamplesFromQueue(DropSamples);

            // Send the event even if the previous call failed.
            hr = QueueEvent(MEStreamSinkStopped, GUID_NULL, hr, NULL);
            break;

        case OpPause:
            hr = QueueEvent(MEStreamSinkPaused, GUID_NULL, hr, NULL);
            break;

        case OpProcessSample:
        case OpPlaceMarker:
            hr = DispatchProcessSample(pOp);
            break;

        case OpFinalize:
            hr = DispatchFinalize(pOp);
            break;
        }
    }

    SafeRelease(&pState);
    LeaveCriticalSection(&m_critSec);
    return hr;
}


//-------------------------------------------------------------------
// Name: DispatchProcessSample
// Description: Complete a ProcessSample or PlaceMarker request.
//-------------------------------------------------------------------

HRESULT CW4allStream::DispatchProcessSample(CAsyncOperation* pOp)
{
    HRESULT hr = S_OK;
    assert(pOp != NULL);

    hr = ProcessSamplesFromQueue(WriteSamples);

    // Ask for another sample
    if (SUCCEEDED(hr))
    {
        if (pOp->m_op == OpProcessSample)
        {
            hr = QueueEvent(MEStreamSinkRequestSample, GUID_NULL, S_OK, NULL);
        }
    }

    // We are in the middle of an asynchronous operation, so if something failed, send an error.
    if (FAILED(hr))
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

HRESULT CW4allStream::ProcessSamplesFromQueue(FlushState bFlushData)
{
    HRESULT hr = S_OK;

    ComPtrList<IUnknown>::POSITION pos = m_SampleQueue.FrontPosition();

    // Enumerate all of the samples/markers in the queue.

    while (pos != m_SampleQueue.EndPosition())
    {
        IUnknown *pUnk = NULL;
        IMarker  *pMarker = NULL;
        IMFSample *pSample = NULL;

        hr = m_SampleQueue.GetItemByPosition(pos, &pUnk);

        assert(pUnk != NULL); // GetItemByPosition should not fail unless we reached the end of the list.

        // Figure out if this is a marker or a sample.
        if (SUCCEEDED(hr))
        {
            hr = pUnk->QueryInterface(__uuidof(IMarker), (void**)&pMarker);
            if (hr == E_NOINTERFACE)
            {
                // If this is a sample, write it to the file.
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
        SafeRelease(&pUnk);
        SafeRelease(&pMarker);
        SafeRelease(&pSample);

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

HRESULT CW4allStream::SendSampleOverNetwork(IMFSample *pSample)
{
    HRESULT hr = S_OK;
    LONGLONG time = 0;
    BYTE *pData = NULL;
    DWORD cbData = 0;
    DWORD cbWritten = 0;

    IMFMediaBuffer *pBuffer = NULL;

    // Get the time stamp
    hr = pSample->GetSampleTime(&time);

    if (SUCCEEDED(hr))
    {
        // If the time stamp is too early, just discard this sample.
        if (time < m_StartTime)
        {
            return S_OK;
        }
    }
    // Note: If there is no time stamp on the sample, proceed anyway.

    hr = pSample->ConvertToContiguousBuffer(&pBuffer);

    // Lock the buffer and write the data to the file.
    if (SUCCEEDED(hr))
    {
        hr = pBuffer->Lock(&pData, NULL, &cbData);
    }

    if (SUCCEEDED(hr))
    {
		//if(m_pByteStream)
		//{
		//	hr = m_pByteStream->Write(pData, cbData, &cbWritten);
		//}
        pBuffer->Unlock();
    }

    // Update the running tally of bytes written.
    if (SUCCEEDED(hr))
    {
        m_cbDataWritten += cbData;
    }

    SafeRelease(&pBuffer);
    return hr;
}

//-------------------------------------------------------------------
// Name: SendMarkerEvent
// Description: Saned a marker event.
//
// pMarker: Pointer to our custom IMarker interface, which holds
//          the marker information.
//-------------------------------------------------------------------

HRESULT CW4allStream::SendMarkerEvent(IMarker *pMarker, FlushState FlushState)
{
    HRESULT hr = S_OK;
    HRESULT hrStatus = S_OK;  // Status code for marker event.

    if (FlushState == DropSamples)
    {
        hrStatus = E_ABORT;
    }

    PROPVARIANT var;
    PropVariantInit(&var);

    // Get the context data.
    hr = pMarker->GetContext(&var);

    if (SUCCEEDED(hr))
    {
        hr = QueueEvent(MEStreamSinkMarker, GUID_NULL, hrStatus, &var);
    }

    PropVariantClear(&var);
    return hr;
}


//-------------------------------------------------------------------
// Name: DispatchFinalize
// Description: Complete a BeginFinalize request.
//-------------------------------------------------------------------

HRESULT CW4allStream::DispatchFinalize(CAsyncOperation* pOp)
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




//////////////////////
// CW4allMarker class
// Holds information from IMFStreamSink::PlaceMarker
//

CW4allMarker::CW4allMarker(MFSTREAMSINK_MARKER_TYPE eMarkerType) : m_nRefCount(1), m_eMarkerType(eMarkerType)
{
    PropVariantInit(&m_varMarkerValue);
    PropVariantInit(&m_varContextValue);
}

CW4allMarker::~CW4allMarker()
{
    assert(m_nRefCount == 0);

    PropVariantClear(&m_varMarkerValue);
    PropVariantClear(&m_varContextValue);
}

/* static */
HRESULT CW4allMarker::Create(
    MFSTREAMSINK_MARKER_TYPE eMarkerType,
    const PROPVARIANT* pvarMarkerValue,     // Can be NULL.
    const PROPVARIANT* pvarContextValue,    // Can be NULL.
    IMarker **ppMarker
    )
{
    if (ppMarker == NULL)
    {
        return E_POINTER;
    }

    HRESULT hr = S_OK;
    CW4allMarker *pMarker = new CW4allMarker(eMarkerType);

    if (pMarker == NULL)
    {
        hr = E_OUTOFMEMORY;
    }

    // Copy the marker data.
    if (SUCCEEDED(hr))
    {
        if (pvarMarkerValue)
        {
            hr = PropVariantCopy(&pMarker->m_varMarkerValue, pvarMarkerValue);
        }
    }

    if (SUCCEEDED(hr))
    {
        if (pvarContextValue)
        {
            hr = PropVariantCopy(&pMarker->m_varContextValue, pvarContextValue);
        }
    }

    if (SUCCEEDED(hr))
    {
        *ppMarker = pMarker;
        (*ppMarker)->AddRef();
    }

    SafeRelease(&pMarker);

    return hr;
}

// IUnknown methods.

ULONG CW4allMarker::AddRef()
{
    return InterlockedIncrement(&m_nRefCount);
}

ULONG CW4allMarker::Release()
{
    ULONG uCount = InterlockedDecrement(&m_nRefCount);
    if (uCount == 0)
    {
        delete this;
    }
    // For thread safety, return a temporary variable.
    return uCount;
}

HRESULT CW4allMarker::QueryInterface(REFIID iid, void** ppv)
{
    if (!ppv)
    {
        return E_POINTER;
    }
    if (iid == IID_IUnknown)
    {
        *ppv = static_cast<IUnknown*>(this);
    }
    else if (iid == __uuidof(IMarker))
    {
        *ppv = static_cast<IMarker*>(this);
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
HRESULT CW4allMarker::GetMarkerType(MFSTREAMSINK_MARKER_TYPE *pType)
{
    if (pType == NULL)
    {
        return E_POINTER;
    }

    *pType = m_eMarkerType;
    return S_OK;
}

HRESULT CW4allMarker::GetMarkerValue(PROPVARIANT *pvar)
{
    if (pvar == NULL)
    {
        return E_POINTER;
    }
    return PropVariantCopy(pvar, &m_varMarkerValue);

}
HRESULT CW4allMarker::GetContext(PROPVARIANT *pvar)
{
    if (pvar == NULL)
    {
        return E_POINTER;
    }
    return PropVariantCopy(pvar, &m_varContextValue);
}

//-------------------------------------------------------------------
// CreatePCMAudioType
//
// Creates a media type that describes an uncompressed PCM audio
// format.
//-------------------------------------------------------------------

HRESULT CreatePCMAudioType(
    UINT32 sampleRate,        // Samples per second
    UINT32 bitsPerSample,     // Bits per sample
    UINT32 cChannels,         // Number of channels
    IMFMediaType **ppType     // Receives a pointer to the media type.
    )
{
    HRESULT hr = S_OK;

    IMFMediaType *pType = NULL;

    // Calculate derived values.
    UINT32 blockAlign = cChannels * (bitsPerSample / 8);
    UINT32 bytesPerSecond = blockAlign * sampleRate;

    // Create the empty media type.
    hr = MFCreateMediaType(&pType);

    // Set attributes on the type.
    if (SUCCEEDED(hr))
    {
        hr = pType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    }

    if (SUCCEEDED(hr))
    {
        hr = pType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
    }

    if (SUCCEEDED(hr))
    {
        hr = pType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, cChannels);
    }

    if (SUCCEEDED(hr))
    {
        hr = pType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, sampleRate);
    }

    if (SUCCEEDED(hr))
    {
        hr = pType->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, blockAlign);
    }

    if (SUCCEEDED(hr))
    {
        hr = pType->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, bytesPerSecond);
    }

    if (SUCCEEDED(hr))
    {
        hr = pType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, bitsPerSample);
    }

    if (SUCCEEDED(hr))
    {
        hr = pType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
    }

    if (SUCCEEDED(hr))
    {
        // Return the type to the caller.
        *ppType = pType;
        (*ppType)->AddRef();
    }

    SafeRelease(&pType);
    return hr;
}


//-------------------------------------------------------------------
// CreateVideoType
//
// Creates a media type that describes a video subtype
// format.
//-------------------------------------------------------------------
HRESULT CreateVideoType(
		const GUID* subType, // video subType
		IMFMediaType **ppType     // Receives a pointer to the media type.
	)
{
	HRESULT hr = S_OK;

    IMFMediaType *pType = NULL;

    CHECK_HR(hr = MFCreateMediaType(&pType));

    CHECK_HR(hr = pType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));

    CHECK_HR(hr = pType->SetGUID(MF_MT_SUBTYPE, *subType));
	
    *ppType = pType;
    (*ppType)->AddRef();

done:
    SafeRelease(&pType);
    return hr;
}

//-------------------------------------------------------------------
// Name: ValidateWaveFormat
// Description: Validates a WAVEFORMATEX structure.
//
// Just to keep the sample as simple as possible, we only accept
// uncompressed PCM formats.
//-------------------------------------------------------------------

HRESULT ValidateWaveFormat(const WAVEFORMATEX *pWav, DWORD cbSize)
{
    if (pWav->wFormatTag != WAVE_FORMAT_PCM)
    {
        return MF_E_INVALIDMEDIATYPE;
    }

    if (pWav->nChannels != 1 && pWav->nChannels != 2)
    {
        return MF_E_INVALIDMEDIATYPE;
    }

    if (pWav->wBitsPerSample != 8 && pWav->wBitsPerSample != 16)
    {
        return MF_E_INVALIDMEDIATYPE;
    }

    if (pWav->cbSize != 0)
    {
        return MF_E_INVALIDMEDIATYPE;
    }

    // Make sure block alignment was calculated correctly.
    if (pWav->nBlockAlign != pWav->nChannels * (pWav->wBitsPerSample / 8))
    {
        return MF_E_INVALIDMEDIATYPE;
    }

    // Check possible overflow...
    if (pWav->nSamplesPerSec  > (DWORD)(MAXDWORD / pWav->nBlockAlign))        // Is (nSamplesPerSec * nBlockAlign > MAXDWORD) ?
    {
        return MF_E_INVALIDMEDIATYPE;
    }

    // Make sure average bytes per second was calculated correctly.
    if (pWav->nAvgBytesPerSec != pWav->nSamplesPerSec * pWav->nBlockAlign)
    {
        return MF_E_INVALIDMEDIATYPE;
    }

    // Everything checked out.
    return S_OK;
}

//-------------------------------------------------------------------
// Name: ValidateVideoFormat
// Description: Validates a media type for this sink.
//-------------------------------------------------------------------
HRESULT ValidateVideoFormat(IMFMediaType *pmt)
{
	GUID major_type = GUID_NULL;
    GUID subtype = GUID_NULL;
    MFVideoInterlaceMode interlace = MFVideoInterlace_Unknown;
    UINT32 val = 0;
    BOOL bFoundMatchingSubtype = FALSE;

    HRESULT hr = S_OK;

    // Major type must be video.
    CHECK_HR(hr = pmt->GetGUID(MF_MT_MAJOR_TYPE, &major_type));

    if (major_type != MFMediaType_Video)
    {
        CHECK_HR(hr = MF_E_INVALIDMEDIATYPE);
    }

    // Subtype must be one of the subtypes in our global list.

    // Get the subtype GUID.
    CHECK_HR(hr = pmt->GetGUID(MF_MT_SUBTYPE, &subtype));

    // Look for the subtype in our list of accepted types.
	for (DWORD i = 0; i < g_NumVideoSubtypes; i++)
    {
        if (subtype == *g_VideoSubtypes[i])
        {
            bFoundMatchingSubtype = TRUE;
            break;
        }
    }

    if (!bFoundMatchingSubtype)
    {
        CHECK_HR(hr = MF_E_INVALIDMEDIATYPE);
    }

    // Video must be progressive frames.
    CHECK_HR(hr = pmt->GetUINT32(MF_MT_INTERLACE_MODE, (UINT32*)&interlace));
    if (interlace != MFVideoInterlace_Progressive)
    {
        CHECK_HR(hr = MF_E_INVALIDMEDIATYPE);
    }

done:
    return hr;
}


#pragma warning( pop )



