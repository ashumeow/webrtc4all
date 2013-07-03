/*
* Copyright (C) 2013 Gotham City. All rights reserved.
* Copyright (C) Microsoft Corporation. All rights reserved.
*/
/**@file w4allSource.cc
 * @brief Audio/Video Media Foundation Source (implementation)
 *
 * @author Batman@GothamCity
 */

#include "w4allSource.h"
#include "w4allUtils.h"

#include <assert.h>

#define GOTHAM_VIDEO_WIDTH 640 // FIXME (must not be defined): 0 to ignore
#define GOTHAM_VIDEO_HEIGHT 480 // FIXME (must not be defined): 0 to ignore
#define GOTHAM_VIDEO_FPS	15 // FIXME

//
// Locking:
//     The source and stream objects both have critical sections. If you
//     hold both locks, the source lock must be held FIRST, to avoid
//     deadlocks.
//
// Shutdown:
//     Most methods start by calling CheckShutdown(). This method
//     fails if the source was shut down.
//



template <class T>
T AlignUp(T num, T mult)
{
    assert(num >= 0);
    T tmp = num + mult - 1;
    return tmp - (tmp % mult);
}



// Helper Functions

HRESULT QueueEventWithIUnknown(
    IMFMediaEventGenerator *pMEG,
    MediaEventType meType,
    HRESULT hrStatus,
    IUnknown *pUnk);

LONGLONG BufferSizeFromAudioDuration(const WAVEFORMATEX *pWav, LONGLONG duration);

HRESULT CW4allSource_CreateInstance(REFIID iid, void **ppMFT)
{
	return CW4allSource::CreateInstance(iid, ppMFT);
}


//-------------------------------------------------------------------
// Name: CreateInstance
// Description: Static method to create an instance of the source.
//
// iid:         IID of the requested interface on the source.
// ppSource:    Receives a ref-counted pointer to the source.
//-------------------------------------------------------------------

HRESULT CW4allSource::CreateInstance(REFIID iid, void **ppSource)
{
    if (ppSource == NULL)
    {
        return E_POINTER;
    }

    HRESULT hr = S_OK;
    CW4allSource *pSource = new (std::nothrow) CW4allSource(hr, true/*FIXME:Video*/); // Created with ref count = 1.
    if (pSource == NULL)
    {
        return E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        hr = pSource->QueryInterface(iid, ppSource);
		if(SUCCEEDED(hr))
		{
			((CW4allSource*)(*ppSource))->AddRef();
		}
    }

    SafeRelease(&pSource);
    return hr;
}


//-------------------------------------------------------------------
// CW4allSource constructor.
//
// hr: If the constructor fails, this value is set to a failure code.
//-------------------------------------------------------------------

CW4allSource::CW4allSource(HRESULT& hr, bool bVideo)
  : m_nRefCount(1),
    m_pEventQueue(NULL),
    m_pPresentationDescriptor(NULL),
    m_IsShutdown(FALSE),
    m_state(STATE_STOPPED),
    m_pStream(NULL),
	m_bVideo(bVideo)
{
    // Create the media event queue.
    hr = MFCreateEventQueue(&m_pEventQueue);

    InitializeCriticalSection(&m_critSec);
}


//-------------------------------------------------------------------
// CW4allSource destructor.
//-------------------------------------------------------------------


CW4allSource::~CW4allSource()
{
    assert(m_IsShutdown);
    assert(m_nRefCount == 0);

    DeleteCriticalSection(&m_critSec);
}


// IUnknown methods

ULONG CW4allSource::AddRef()
{
    return InterlockedIncrement(&m_nRefCount);
}

ULONG  CW4allSource::Release()
{
    ULONG uCount = InterlockedDecrement(&m_nRefCount);
    if (uCount == 0)
    {
        delete this;
    }
    // For thread safety, return a temporary variable.
    return uCount;
}

HRESULT CW4allSource::QueryInterface(REFIID iid, void** ppv)
{
    static const QITAB qit[] =
    {
        QITABENT(CW4allSource, IMFMediaEventGenerator),
        QITABENT(CW4allSource, IMFMediaSource),
        { 0 }
    };
    return QISearch(this, qit, iid, ppv);
}


// IMFMediaEventGenerator methods
//
// All of the IMFMediaEventGenerator methods do the following:
// 1. Check for shutdown status.
// 2. Call the event generator helper object.

HRESULT CW4allSource::BeginGetEvent(IMFAsyncCallback* pCallback, IUnknown* punkState)
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

HRESULT CW4allSource::EndGetEvent(IMFAsyncResult* pResult, IMFMediaEvent** ppEvent)
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

HRESULT CW4allSource::GetEvent(DWORD dwFlags, IMFMediaEvent** ppEvent)
{
    // NOTE: GetEvent can block indefinitely, so we don't hold the
    //       CW4allSource lock. This requires some juggling with the
    //       event queue pointer.

    HRESULT hr = S_OK;

    IMFMediaEventQueue *pQueue = NULL;

    EnterCriticalSection(&m_critSec);

    // Check shutdown
    hr = CheckShutdown();

    if (SUCCEEDED(hr))
    {
        pQueue = m_pEventQueue;
        pQueue->AddRef();
    }

    LeaveCriticalSection(&m_critSec);

    if (SUCCEEDED(hr))
    {
        hr = pQueue->GetEvent(dwFlags, ppEvent);
    }

    SafeRelease(&pQueue);
    return hr;
}

HRESULT CW4allSource::QueueEvent(MediaEventType met, REFGUID guidExtendedType, HRESULT hrStatus, const PROPVARIANT* pvValue)
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


// IMFMediaSource methods


//-------------------------------------------------------------------
// Name: CreatePresentationDescriptor
// Description: Returns a copy of the default presentation descriptor.
//-------------------------------------------------------------------

HRESULT CW4allSource::CreatePresentationDescriptor(IMFPresentationDescriptor** ppPresentationDescriptor)
{
    if (ppPresentationDescriptor == NULL)
    {
        return E_POINTER;
    }

    EnterCriticalSection(&m_critSec);

    HRESULT hr = S_OK;

    hr = CheckShutdown();

    if (SUCCEEDED(hr))
    {
        if (m_pPresentationDescriptor == NULL)
        {
            hr = CreatePresentationDescriptor();
        }
    }

    // Clone our default presentation descriptor.
    if (SUCCEEDED(hr))
    {
        hr = m_pPresentationDescriptor->Clone(ppPresentationDescriptor);
    }

    LeaveCriticalSection(&m_critSec);

    return hr;
}


//-------------------------------------------------------------------
// Name: GetCharacteristics
// Description: Returns flags the describe the source.
//-------------------------------------------------------------------

HRESULT CW4allSource::GetCharacteristics(DWORD* pdwCharacteristics)
{
    if (pdwCharacteristics == NULL)
    {
        return E_POINTER;
    }

    EnterCriticalSection(&m_critSec);

    HRESULT hr = S_OK;

    hr = CheckShutdown();

    if (SUCCEEDED(hr))
    {
        *pdwCharacteristics =  MFMEDIASOURCE_CAN_PAUSE | MFMEDIASOURCE_IS_LIVE;
    }

    LeaveCriticalSection(&m_critSec);

    return hr;
}

//-------------------------------------------------------------------
// Name: Start
// Description: Switches to running state.
//-------------------------------------------------------------------

HRESULT CW4allSource::Start(
    IMFPresentationDescriptor* pPresentationDescriptor,
    const GUID* pguidTimeFormat,
    const PROPVARIANT* pvarStartPosition
    )
{
    HRESULT hr = S_OK;
    LONGLONG llStartOffset = 0;
    BOOL bIsSeek = FALSE;
    BOOL bIsRestartFromCurrentPosition = FALSE;
    BOOL bQueuedStartEvent = FALSE;

    IMFMediaEvent *pEvent = NULL;

    PROPVARIANT var;
    PropVariantInit(&var);

    // Check parameters.
    // Start position and presentation descriptor cannot be NULL.
    if (pvarStartPosition == NULL || pPresentationDescriptor == NULL)
    {
        return E_INVALIDARG;
    }

    // Check the time format. Must be "reference time" units.
    if ((pguidTimeFormat != NULL) && (*pguidTimeFormat != GUID_NULL))
    {
        // Unrecognized time format GUID.
        return MF_E_UNSUPPORTED_TIME_FORMAT;
    }

    EnterCriticalSection(&m_critSec);

    // Fail if the source is shut down.
    hr = CheckShutdown();

    if (FAILED(hr)) { goto done; }

    // Check the start position.
    if (pvarStartPosition->vt == VT_I8)
    {
        // Start position is given in pvarStartPosition in 100-ns units.
        llStartOffset = pvarStartPosition->hVal.QuadPart;

        if (m_state != STATE_STOPPED)
        {
            // Source is running or paused, so this is a seek.
            bIsSeek = TRUE;
        }
    }
    else if (pvarStartPosition->vt == VT_EMPTY)
    {
        // Start position is "current position".
        // For stopped, that means 0. Otherwise, use the current position.
        if (m_state == STATE_STOPPED)
        {
            llStartOffset = 0;
        }
        else
        {
            llStartOffset = GetCurrentPosition();
            bIsRestartFromCurrentPosition = TRUE;
        }
    }
    else
    {
        // We don't support this time format.
        hr =  MF_E_UNSUPPORTED_TIME_FORMAT;
        goto done;
    }

    // Validate the caller's presentation descriptor.
    hr = ValidatePresentationDescriptor(pPresentationDescriptor);

    if (FAILED(hr)) { goto done; }

    // Sends the MENewStream or MEUpdatedStream event.
    hr = QueueNewStreamEvent(pPresentationDescriptor);

    if (FAILED(hr)) { goto done; }

    // Notify the stream of the new start time.
    hr = m_pStream->SetPosition(llStartOffset);

    if (FAILED(hr)) { goto done; }

    // Send Started or Seeked events.

    var.vt = VT_I8;
    var.hVal.QuadPart = llStartOffset;

    // Send the source event.
    if (bIsSeek)
    {
        hr = QueueEvent(MESourceSeeked, GUID_NULL, hr, &var);

        if (FAILED(hr)) { goto done; }
    }
    else
    {
        // For starting, if we are RESTARTING from the current position and our
        // previous state was running/paused, then we need to add the
        // MF_EVENT_SOURCE_ACTUAL_START attribute to the event. This requires
        // creating the event object first.

        // Create the event.
        hr = MFCreateMediaEvent(MESourceStarted, GUID_NULL, hr, &var, &pEvent);

        if (FAILED(hr)) { goto done; }

        // For restarts, set the actual start time as an attribute.
        if (bIsRestartFromCurrentPosition)
        {
            hr = pEvent->SetUINT64(MF_EVENT_SOURCE_ACTUAL_START, llStartOffset);
            if (FAILED(hr)) { goto done; }
        }

        // Now  queue the event.
        hr = m_pEventQueue->QueueEvent(pEvent);

        if (FAILED(hr)) { goto done; }
    }

    bQueuedStartEvent = TRUE;

    // Send the stream event.
    if (m_pStream)
    {
        if (bIsSeek)
        {
            hr = m_pStream->QueueEvent(MEStreamSeeked, GUID_NULL, hr, &var);
        }
        else
        {
            hr = m_pStream->QueueEvent(MEStreamStarted, GUID_NULL, hr, &var);
        }

        if (FAILED(hr)) { goto done; }
    }

    if (bIsSeek)
    {
        // For seek requests, flush any queued samples.
        hr = m_pStream->Flush();
    }
    else
    {
        // Otherwise, deliver any queued samples.
        hr = m_pStream->DeliverQueuedSamples();
    }

    if (FAILED(hr)) { goto done; }

    m_state = STATE_STARTED;

done:

    // If a failure occurred and we have not sent the
    // MESourceStarted/MESourceSeeked event yet, then it is
    // OK just to return an error code from Start().

    // If a failure occurred and we have already sent the
    // event (with a success code), then we need to raise an
    // MEError event.

    if (FAILED(hr) && bQueuedStartEvent)
    {
        hr = QueueEvent(MEError, GUID_NULL, hr, &var);
    }

    PropVariantClear(&var);
    SafeRelease(&pEvent);

    LeaveCriticalSection(&m_critSec);

    return hr;
}


//-------------------------------------------------------------------
// Name: Pause
// Description: Switches to paused state.
//-------------------------------------------------------------------

HRESULT CW4allSource::Pause()
{
    EnterCriticalSection(&m_critSec);

    HRESULT hr = S_OK;

    hr = CheckShutdown();

    // Pause is only allowed from started state.
    if (SUCCEEDED(hr))
    {
        if (m_state != STATE_STARTED)
        {
            hr = MF_E_INVALID_STATE_TRANSITION;
        }
    }

    // Send the appropriate events.
    if (SUCCEEDED(hr))
    {
        if (m_pStream)
        {
            hr = m_pStream->QueueEvent(MEStreamPaused, GUID_NULL, S_OK, NULL);
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = QueueEvent(MESourcePaused, GUID_NULL, S_OK, NULL);
    }

    // Update our state.
    if (SUCCEEDED(hr))
    {
        m_state = STATE_PAUSED;
    }

    LeaveCriticalSection(&m_critSec);

    return hr;
}


//-------------------------------------------------------------------
// Name: Stop
// Description: Switches to stopped state.
//-------------------------------------------------------------------

HRESULT CW4allSource::Stop()
{
    EnterCriticalSection(&m_critSec);

    HRESULT hr = S_OK;

    hr = CheckShutdown();

    if (SUCCEEDED(hr))
    {
        // Update our state.
        m_state = STATE_STOPPED;

        // Flush all queued samples.
        hr = m_pStream->Flush();
    }

    //
    // Queue events.
    //

    if (SUCCEEDED(hr))
    {
        if (m_pStream)
        {
            hr = m_pStream->QueueEvent(MEStreamStopped, GUID_NULL, S_OK, NULL);
        }
    }
    if (SUCCEEDED(hr))
    {
        hr = QueueEvent(MESourceStopped, GUID_NULL, S_OK, NULL);
    }

    LeaveCriticalSection(&m_critSec);

    return hr;
}


//-------------------------------------------------------------------
// Name: Shutdown
// Description: Releases resources.
//
// The source and stream objects hold reference counts on each other.
// To avoid memory leaks caused by circular ref. counts, the Shutdown
// method releases the pointer to the stream.
//-------------------------------------------------------------------

HRESULT CW4allSource::Shutdown()
{
    EnterCriticalSection(&m_critSec);

    HRESULT hr = S_OK;

    hr = CheckShutdown();

    if (SUCCEEDED(hr))
    {
        // Shut down the stream object.
        if (m_pStream)
        {
            (void)m_pStream->Shutdown();
        }

        // Shut down the event queue.
        if (m_pEventQueue)
        {
            (void)m_pEventQueue->Shutdown();
        }

        // Release objects.
        SafeRelease(&m_pStream);
        SafeRelease(&m_pEventQueue);
        SafeRelease(&m_pPresentationDescriptor);

        // Set our shutdown flag.
        m_IsShutdown = TRUE;
    }

    LeaveCriticalSection(&m_critSec);

    return hr;
}

/////////////// Private CW4allSource methods

// NOTE: These private methods do not hold the source's critical
// section. The caller must ensure the critical section is held.
// Also, these methods do not check for shut-down.


//-------------------------------------------------------------------
// Name: CreatePresentationDescriptor
// Description: Creates the default presentation descriptor.
//-------------------------------------------------------------------

HRESULT CW4allSource::CreatePresentationDescriptor()
{
    HRESULT hr = S_OK;
    MFTIME duration = 0;

    IMFMediaType *pMediaType = NULL;
    IMFStreamDescriptor *pStreamDescriptor = NULL;
    IMFMediaTypeHandler *pHandler = NULL;

	DWORD dwMediaCount = m_bVideo ? g_NumVideoSubtypes : g_NumAudioFormats;
	IMFMediaType** ppMediaType = (IMFMediaType**)calloc(dwMediaCount, sizeof(IMFMediaType*));
	DWORD dwIndex;

    // Create an empty media type.
    hr = MFCreateMediaType(&pMediaType);

    // Initialize the media type from the WAVEFORMATEX structure.
    if (SUCCEEDED(hr))
    {
		if(m_bVideo)
		{
			for(dwIndex = 0; dwIndex < dwMediaCount; ++dwIndex)
			{
				const GUID* subType = g_VideoSubtypes[dwIndex];
				hr = CreateVideoType(subType, &ppMediaType[dwIndex], GOTHAM_VIDEO_WIDTH, GOTHAM_VIDEO_HEIGHT);
				if (!SUCCEEDED(hr))
				{
					break;
				}
				hr = MFSetAttributeRatio(ppMediaType[dwIndex], MF_MT_FRAME_RATE, (GOTHAM_VIDEO_FPS * 1000), 1001);
				// hr = MFSetAttributeRatio(ppMediaType[dwIndex], MF_MT_PIXEL_ASPECT_RATIO, 1, 1); 
			}
		}
		else
		{
			for(dwIndex = 0; dwIndex < dwMediaCount; ++dwIndex)
			{
				hr = CreatePCMAudioType(g_AudioFormats[dwIndex].nSamplesPerSec, g_AudioFormats[dwIndex].wBitsPerSample, g_AudioFormats[dwIndex].nChannels, &ppMediaType[dwIndex]);
				if (!SUCCEEDED(hr))
				{
					break;
				}
			}
		}
        // hr = MFInitMediaTypeFromWaveFormatEx(pMediaType, WaveFormat(), WaveFormatSize());
    }

    // Create the stream descriptor.
    if (SUCCEEDED(hr))
    {
        hr = MFCreateStreamDescriptor(
            0,          // stream identifier
            dwMediaCount,          // Number of media types.
            ppMediaType, // Array of media types
            &pStreamDescriptor
            );
    }

    // Set the default media type on the media type handler.
    if (SUCCEEDED(hr))
    {
        hr = pStreamDescriptor->GetMediaTypeHandler(&pHandler);
    }

	//FIXME
    if (SUCCEEDED(hr))
    {
       hr = pHandler->SetCurrentMediaType(ppMediaType[0]);
    }

    // Create the presentation descriptor.
    if (SUCCEEDED(hr))
    {
        hr = MFCreatePresentationDescriptor(
            1,                      // Number of stream descriptors
            &pStreamDescriptor,     // Array of stream descriptors
            &m_pPresentationDescriptor
            );
    }

    // FIXME: Select the first stream
    if (SUCCEEDED(hr))
    {
        hr = m_pPresentationDescriptor->SelectStream(0);
    }

    // Set the file duration as an attribute on the presentation descriptor.
    if (SUCCEEDED(hr))
    {
		// FIXME:
		/*
        duration = m_pRiff->FileDuration();
        hr = m_pPresentationDescriptor->SetUINT64(MF_PD_DURATION, (UINT64)duration);*/
		//hr = m_pPresentationDescriptor->SetUINT64(MF_PD_DURATION, (UINT64)ULLONG_MAX);//FIXME
    }

	for(dwIndex = 0; dwIndex < dwMediaCount; ++dwIndex)
	{
		SafeRelease(&ppMediaType[dwIndex]);
	}
	free((void*)ppMediaType);
    
    SafeRelease(&pStreamDescriptor);
    SafeRelease(&pHandler);
    return hr;
}



//-------------------------------------------------------------------
// Name: ValidatePresentationDescriptor
// Description: Validates the caller's presentation descriptor.
//
// This method is called when Start() is called with a non-NULL
// presentation descriptor. The caller is supposed to give us back
// the same PD that we gave out in CreatePresentationDescriptor().
// This method performs a sanity check on the caller's PD to make
// sure it matches ours.
//
// Note: Because this media source has one stream with single, fixed
//       media type, there is not much for the caller to decide. In
//       a more complicated source, the caller might select different
//       streams, or select from a list of media types.
//-------------------------------------------------------------------

HRESULT CW4allSource::ValidatePresentationDescriptor(IMFPresentationDescriptor *pPD)
{
    HRESULT hr;

    assert(pPD != NULL);

    IMFStreamDescriptor *pStreamDescriptor = NULL;
    IMFMediaTypeHandler *pHandler = NULL;
    IMFMediaType        *pMediaType = NULL;

    DWORD   cStreamDescriptors = 0;
    BOOL    fSelected = FALSE;

    // Make sure there is only one stream.
    hr = pPD->GetStreamDescriptorCount(&cStreamDescriptors);

    if (SUCCEEDED(hr))
    {
        if (cStreamDescriptors != 1)
        {
            hr = MF_E_UNSUPPORTED_REPRESENTATION;
        }
    }

    // Get the stream descriptor.
    if (SUCCEEDED(hr))
    {
        hr = pPD->GetStreamDescriptorByIndex(0, &fSelected, &pStreamDescriptor);
    }

    // Make sure it's selected. (This media source has only one stream, so it
    // is not useful to deselect the only stream.)
    if (SUCCEEDED(hr))
    {
        if (!fSelected)
        {
            hr = MF_E_UNSUPPORTED_REPRESENTATION;
        }
    }

    // Get the media type handler, so that we can get the media type.
    if (SUCCEEDED(hr))
    {
        hr = pStreamDescriptor->GetMediaTypeHandler(&pHandler);
    }

    if (SUCCEEDED(hr))
    {
        hr = pHandler->GetCurrentMediaType(&pMediaType);
    }

	if(m_bVideo)
	{
		if (SUCCEEDED(hr))
		{
			hr = ValidateVideoFormat(pMediaType);
		}
	}
	else
	{
		WAVEFORMATEX        *pFormat = NULL;
		UINT32  cbWaveFormat = 0;

		if (SUCCEEDED(hr))
		{
			hr = MFCreateWaveFormatExFromMFMediaType(
				pMediaType,
				&pFormat,
				&cbWaveFormat);
		}
		if (SUCCEEDED(hr))
		{
			/*assert(this->WaveFormat() != NULL);

			if (cbWaveFormat < this->WaveFormatSize())
			{
				hr = MF_E_INVALIDMEDIATYPE;
			}*/
		}

		if (SUCCEEDED(hr))
		{
			/*if (memcmp(pFormat, WaveFormat(), WaveFormatSize()) != 0)
			{
				hr = MF_E_INVALIDMEDIATYPE;
			}*/
		}

		CoTaskMemFree(pFormat);
	}

    SafeRelease(&pStreamDescriptor);
    SafeRelease(&pHandler);
    SafeRelease(&pMediaType);

    return hr;
}


//-------------------------------------------------------------------
// Name: QueueNewStreamEvent
// Description:
// Queues an MENewStream or MEUpdatedStream event during Start.
//
// pPD: The presentation descriptor.
//
// Precondition: The presentation descriptor is assumed to be valid.
// Call ValidatePresentationDescriptor before calling this method.
//-------------------------------------------------------------------

HRESULT CW4allSource::QueueNewStreamEvent(IMFPresentationDescriptor *pPD)
{
    assert(pPD != NULL);

    HRESULT hr = S_OK;
    IMFStreamDescriptor *pSD = NULL;

    BOOL fSelected = FALSE;

    hr = pPD->GetStreamDescriptorByIndex(0, &fSelected, &pSD);

    if (SUCCEEDED(hr))
    {
        // The stream must be selected, because we don't allow the app
        // to de-select the stream. See ValidatePresentationDescriptor.
        assert(fSelected);

        if (m_pStream)
        {
            // The stream already exists, and is still selected.
            // Send the MEUpdatedStream event.
            hr = QueueEventWithIUnknown(this, MEUpdatedStream, S_OK, m_pStream);
        }
        else
        {
            // The stream does not exist, and is now selected.
            // Create a new stream.

            hr = CreateCW4allStreamSource(pSD);

            if (SUCCEEDED(hr))
            {
                // CreateCW4allStreamSource creates the stream, so m_pStream is no longer NULL.
                assert(m_pStream != NULL);

                // Send the MENewStream event.
                hr = QueueEventWithIUnknown(this, MENewStream, S_OK, m_pStream);
            }
        }
    }

    SafeRelease(&pSD);
    return hr;
}

//-------------------------------------------------------------------
// Name: CreateCW4allStreamSource
// Description: Creates the source's media stream object.
//-------------------------------------------------------------------

HRESULT CW4allSource::CreateCW4allStreamSource(IMFStreamDescriptor *pSD)
{
    HRESULT hr = S_OK;
    m_pStream = new (std::nothrow) CW4allStreamSource(this, pSD, hr);

    if (m_pStream == NULL)
    {
        hr = E_OUTOFMEMORY;
    }

    if (FAILED(hr))
    {
        SafeRelease(&m_pStream);
    }

    return hr;
}



//-------------------------------------------------------------------
// Name: GetCurrentPosition
// Description: Returns the current playback position.
//-------------------------------------------------------------------

LONGLONG CW4allSource::GetCurrentPosition() const
{
    if (m_pStream)
    {
        return m_pStream->GetCurrentPosition();
    }
    else
    {
        // If no stream is selected, we are at time 0 by definition.
        return 0;
    }
}



////////// AUDIO STREAM

//-------------------------------------------------------------------
// CW4allStreamSource constructor.
//
// pSource: Parent media source.
// pSD: Stream descriptor that describes this stream.
// hr: If the constructor fails, this value is set to a failure code.
//-------------------------------------------------------------------


CW4allStreamSource::CW4allStreamSource(CW4allSource *pSource,  IMFStreamDescriptor *pSD, HRESULT& hr) :
    m_nRefCount(1),
    m_pEventQueue(NULL),
    m_IsShutdown(FALSE),
    m_rtCurrentPosition(0),
    m_discontinuity(FALSE),
    m_EOS(FALSE)
{
    m_pSource = pSource;
    m_pSource->AddRef();

    m_pStreamDescriptor = pSD;
    m_pStreamDescriptor->AddRef();

    // Create the media event queue.
    hr = MFCreateEventQueue(&m_pEventQueue);

    InitializeCriticalSection(&m_critSec);
}


//-------------------------------------------------------------------
// CW4allStreamSource destructor.
//-------------------------------------------------------------------

CW4allStreamSource::~CW4allStreamSource()
{
    assert(m_IsShutdown);
    assert(m_nRefCount == 0);

    DeleteCriticalSection(&m_critSec);
}


// IUnknown methods

ULONG CW4allStreamSource::AddRef()
{
    return InterlockedIncrement(&m_nRefCount);
}

ULONG  CW4allStreamSource::Release()
{
    ULONG uCount = InterlockedDecrement(&m_nRefCount);
    if (uCount == 0)
    {
        delete this;
    }
    // For thread safety, return a temporary variable.
    return uCount;
}

HRESULT CW4allStreamSource::QueryInterface(REFIID iid, void** ppv)
{
    static const QITAB qit[] =
    {
        QITABENT(CW4allStreamSource, IMFMediaEventGenerator),
        QITABENT(CW4allStreamSource, IMFMediaStream),
        { 0 }
    };
    return QISearch(this, qit, iid, ppv);
}


// IMFMediaEventGenerator methods
// [See note for CW4allSource class]

HRESULT CW4allStreamSource::BeginGetEvent(IMFAsyncCallback* pCallback, IUnknown* punkState)
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

HRESULT CW4allStreamSource::EndGetEvent(IMFAsyncResult* pResult, IMFMediaEvent** ppEvent)
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

HRESULT CW4allStreamSource::GetEvent(DWORD dwFlags, IMFMediaEvent** ppEvent)
{
    HRESULT hr = S_OK;

    IMFMediaEventQueue *pQueue = NULL;

    EnterCriticalSection(&m_critSec);

    hr = CheckShutdown();

    if (SUCCEEDED(hr))
    {
        pQueue = m_pEventQueue;
        pQueue->AddRef();
    }

    LeaveCriticalSection(&m_critSec);

    if (SUCCEEDED(hr))
    {
        hr = pQueue->GetEvent(dwFlags, ppEvent);
    }

    SafeRelease(&pQueue);
    return hr;
}

HRESULT CW4allStreamSource::QueueEvent(MediaEventType met, REFGUID guidExtendedType, HRESULT hrStatus, const PROPVARIANT* pvValue)
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


// IMFMediaStream methods.


//-------------------------------------------------------------------
// Name: GetMediaSource
// Description: Returns a pointer to the media source.
//-------------------------------------------------------------------

HRESULT CW4allStreamSource::GetMediaSource(IMFMediaSource** ppMediaSource)
{
    if (ppMediaSource == NULL)
    {
        return E_POINTER;
    }

    EnterCriticalSection(&m_critSec);

    HRESULT hr = S_OK;

    // If called after shutdown, them m_pSource is NULL.
    // Otherwise, m_pSource should not be NULL.

    hr = CheckShutdown();

    if (SUCCEEDED(hr))
    {
        if (m_pSource == NULL)
        {
            hr = E_UNEXPECTED;
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = m_pSource->QueryInterface(IID_PPV_ARGS(ppMediaSource));
    }

    LeaveCriticalSection(&m_critSec);
    return hr;
}


//-------------------------------------------------------------------
// Name: GetStreamDescriptor
// Description: Returns the stream descriptor for this stream.
//-------------------------------------------------------------------

HRESULT CW4allStreamSource::GetStreamDescriptor(IMFStreamDescriptor** ppStreamDescriptor)
{
    if (ppStreamDescriptor == NULL)
    {
        return E_POINTER;
    }

    if (m_pStreamDescriptor == NULL)
    {
        return E_UNEXPECTED;
    }

    EnterCriticalSection(&m_critSec);

    HRESULT hr = S_OK;

    hr = CheckShutdown();

    if (SUCCEEDED(hr))
    {
        *ppStreamDescriptor = m_pStreamDescriptor;
        (*ppStreamDescriptor)->AddRef();
    }

    LeaveCriticalSection(&m_critSec);

    return hr;
}



//-------------------------------------------------------------------
// Name: RequestSample
// Description: Requests a new sample.
//
// pToken: Token object. Can be NULL.
//-------------------------------------------------------------------

HRESULT CW4allStreamSource::RequestSample(IUnknown* pToken)
{
    if (m_pSource == NULL)
    {
        return E_UNEXPECTED;
    }

    HRESULT hr = S_OK;

    IMFMediaSource *pSource = NULL;
    IMFSample *pSample = NULL;  // Sample to deliver.

    EnterCriticalSection(&m_critSec);

    // Check if we are shut down.
    hr = CheckShutdown();

    // Check if we already reached the end of the stream.
    if (SUCCEEDED(hr))
    {
        if (m_EOS)
        {
            hr = MF_E_END_OF_STREAM;
        }
    }

    // Check the source is stopped.
    // GetState does not hold the source's critical section. Safe to call.
    if (SUCCEEDED(hr))
    {
        if (m_pSource->GetState() == CW4allSource::STATE_STOPPED)
        {
            hr = MF_E_INVALIDREQUEST;
        }
    }

    if (SUCCEEDED(hr))
    {
        // Create a new audio sample.
        hr = CreateSample(&pSample);
    }

    if (SUCCEEDED(hr))
    {
        // If the caller provided a token, attach it to the sample as
        // an attribute.

        // NOTE: If we processed sample requests asynchronously, we would
        // need to call AddRef on the token and put the token onto a FIFO
        // queue. See documenation for IMFMediaStream::RequestSample.
        if (pToken)
        {
            hr = pSample->SetUnknown(MFSampleExtension_Token, pToken);
        }
    }

    // If paused, queue the sample for later delivery. Otherwise, deliver the sample now.
    if (SUCCEEDED(hr))
    {
        if (m_pSource->GetState() == CW4allSource::STATE_PAUSED)
        {
            hr = m_sampleQueue.Queue(pSample);
        }
        else
        {
            hr = DeliverSample(pSample);
        }
    }

    // Cache a pointer to the source, prior to leaving the critical section.
    if (SUCCEEDED(hr))
    {
        pSource = m_pSource;
        pSource->AddRef();
    }

    LeaveCriticalSection(&m_critSec);


    // We only have one stream, so the end of the stream is also the end of the
    // presentation. Therefore, when we reach the end of the stream, we need to
    // queue the end-of-presentation event from the source. Logically we would do
    // this inside the CheckEndOfStream method. However, we cannot hold the
    // source's critical section while holding the stream's critical section, at
    // risk of deadlock.

    if (SUCCEEDED(hr))
    {
        if (m_EOS)
        {
            hr = pSource->QueueEvent(MEEndOfPresentation, GUID_NULL, S_OK, NULL);
        }
    }

    SafeRelease(&pSample);
    SafeRelease(&pSource);
    return hr;
}


///// Private CW4allStreamSource methods

// NOTE: Some of these methods hold the stream's critical section
// because they are called by the media source object.

//-------------------------------------------------------------------
// Name: CreateSample
// Description: Creates a new audio/video sample.
//-------------------------------------------------------------------

HRESULT CW4allStreamSource::CreateSample(IMFSample **ppSample)
{
    HRESULT hr = S_OK;

	IMFMediaBuffer *pBuffer = NULL;
    IMFSample *pSample = NULL;
	BYTE        *pData = NULL;

	if(m_pSource->m_bVideo)
	{
		DWORD       cbBuffer = (GOTHAM_VIDEO_HEIGHT * GOTHAM_VIDEO_WIDTH * 2);
		// FIXME: create only once
		 // Create the buffer.
		 hr = MFCreateMemoryBuffer(cbBuffer, &pBuffer);

		 // Get a pointer to the buffer memory.
		if (SUCCEEDED(hr))
		{
			hr = pBuffer->Lock(&pData, NULL, NULL);
		}

		// TODO: Copy data
		for(UINT32 i = 0; i < cbBuffer; ++i)
		{
			pData[i] = (rand() % 0xFF);
		}

		 // Unlock the buffer.
		if (SUCCEEDED(hr))
		{
			hr = pBuffer->Unlock();
			pData = NULL;
		}

		// Set the size of the valid data in the buffer.
		if (SUCCEEDED(hr))
		{
			hr = pBuffer->SetCurrentLength(cbBuffer);
		}

		 // Create a new sample and add the buffer to it.
		if (SUCCEEDED(hr))
		{
			hr = MFCreateSample(&pSample);
		}

		if (SUCCEEDED(hr))
		{
			hr = pSample->AddBuffer(pBuffer);
		}

		if (SUCCEEDED(hr))
		{
			UINT64 rtDuration;
			hr = MFFrameRateToAverageTimePerFrame(GOTHAM_VIDEO_FPS, 1, &rtDuration);
			if(SUCCEEDED(hr))
			{
				// Set the time stamps, duration, and sample flags.
				hr = pSample->SetSampleTime(m_rtCurrentPosition);
				if (SUCCEEDED(hr))
				{
					hr = pSample->SetSampleDuration(rtDuration);
					if (SUCCEEDED(hr))
					{
						m_rtCurrentPosition += rtDuration;
					}
				}
			}
		}
	}
	else
	{

	}

	if (SUCCEEDED(hr))
    {
        // Give the pointer to the caller.
        if((*ppSample = pSample))
		{
			(*ppSample)->AddRef();
		}
    }

    if (pData && pBuffer)
    {
        hr = pBuffer->Unlock();
    }

    SafeRelease(&pBuffer);
    SafeRelease(&pSample);

/*
    IMFMediaBuffer *pBuffer = NULL;
    IMFSample *pSample = NULL;

    DWORD       cbBuffer = 0;
    BYTE        *pData = NULL;
    LONGLONG    duration = 0;

    // Start with one second of data, rounded up to the nearest block.
    cbBuffer = AlignUp<DWORD>(m_pRiff->Format()->nAvgBytesPerSec, m_pRiff->Format()->nBlockAlign);

    // Don't request any more than what's left.
    cbBuffer = min(cbBuffer, m_pRiff->BytesRemainingInChunk());

    // Create the buffer.
    hr = MFCreateMemoryBuffer(cbBuffer, &pBuffer);

    // Get a pointer to the buffer memory.
    if (SUCCEEDED(hr))
    {
        hr = pBuffer->Lock(&pData, NULL, NULL);
    }

    // Fill the buffer
    if (SUCCEEDED(hr))
    {
        hr = m_pRiff->ReadDataFromChunk(pData, cbBuffer);
    }

    // Unlock the buffer.
    if (SUCCEEDED(hr))
    {
        hr = pBuffer->Unlock();
        pData = NULL;
    }

    // Set the size of the valid data in the buffer.
    if (SUCCEEDED(hr))
    {
        hr = pBuffer->SetCurrentLength(cbBuffer);
    }

    // Create a new sample and add the buffer to it.
    if (SUCCEEDED(hr))
    {
        hr = MFCreateSample(&pSample);
    }

    if (SUCCEEDED(hr))
    {
        hr = pSample->AddBuffer(pBuffer);
    }

    // Set the time stamps, duration, and sample flags.
    if (SUCCEEDED(hr))
    {
        hr = pSample->SetSampleTime(m_rtCurrentPosition);
    }

    if (SUCCEEDED(hr))
    {
        duration = AudioDurationFromBufferSize(m_pRiff->Format(), cbBuffer);
        hr = pSample->SetSampleDuration(duration);
    }

    // Set the discontinuity flag.
    if (SUCCEEDED(hr))
    {
        if (m_discontinuity)
        {
            hr = pSample->SetUINT32(MFSampleExtension_Discontinuity, TRUE);
        }
    }

    if (SUCCEEDED(hr))
    {
        // Update our current position.
        m_rtCurrentPosition += duration;

        // Give the pointer to the caller.
        *ppSample = pSample;
        (*ppSample)->AddRef();
    }

    if (pData && pBuffer)
    {
        hr = pBuffer->Unlock();
    }

    SafeRelease(&pBuffer);
    SafeRelease(&pSample);

	*/
    return hr;
}

//-------------------------------------------------------------------
// Name: DeliverSample
// Description: Delivers a sample by sending an MEMediaSample event.
//-------------------------------------------------------------------
HRESULT CW4allStreamSource::DeliverSample(IMFSample *pSample)
{
    HRESULT hr = S_OK;

	if(pSample)
	{
		// Send the MEMediaSample event with the new sample.
		hr = QueueEventWithIUnknown(this, MEMediaSample, hr, pSample);
	}

    // See if we reached the end of the stream.
    if (SUCCEEDED(hr))
    {
        hr = CheckEndOfStream();    // This method sends MEEndOfStream if needed.
    }

    return hr;
}

//-------------------------------------------------------------------
// Name: DeliverQueuedSamples
// Description: Delivers any samples waiting in the queue.
//
// Note: If the client requests a sample while the source is paused,
// the sample is queued and delivered on the next non-seeking call
// to Start(). The queue is flushed if the source is seeked or
// stopped.
//-------------------------------------------------------------------

HRESULT CW4allStreamSource::DeliverQueuedSamples()
{
    HRESULT hr = S_OK;
    IMFSample *pSample = NULL;

    EnterCriticalSection(&m_critSec);

    // If we already reached the end of the stream, send the MEEndStream
    // event again.
    if (m_EOS)
    {
        hr = QueueEvent(MEEndOfStream, GUID_NULL, S_OK, NULL);
    }

    if (SUCCEEDED(hr))
    {
        // Deliver any queued samples.
        while (!m_sampleQueue.IsEmpty())
        {
            hr = m_sampleQueue.Dequeue(&pSample);
            if (FAILED(hr))
            {
                break;
            }

            hr = DeliverSample(pSample);
            if (FAILED(hr))
            {
                break;
            }

            SafeRelease(&pSample);
        }
    }

    LeaveCriticalSection(&m_critSec);

    // If we reached the end of the stream, send the end-of-presentation event from
    // the media source.
    if (SUCCEEDED(hr))
    {
        if (m_EOS)
        {
            hr = m_pSource->QueueEvent(MEEndOfPresentation, GUID_NULL, S_OK, NULL);
        }
    }

    SafeRelease(&pSample);
    return hr;
}


//-------------------------------------------------------------------
// Name: Flush
// Description: Flushes the sample queue.
//-------------------------------------------------------------------

HRESULT CW4allStreamSource::Flush()
{
    EnterCriticalSection(&m_critSec);

    m_sampleQueue.Clear();

    LeaveCriticalSection(&m_critSec);
    return S_OK;
}


//-------------------------------------------------------------------
// Name: Shutdown
// Description: Notifies the stream that the source was shut down.
//-------------------------------------------------------------------

HRESULT CW4allStreamSource::Shutdown()
{
    EnterCriticalSection(&m_critSec);

    // Flush queued samples.
    Flush();

    // Shut down the event queue.
    if (m_pEventQueue)
    {
        m_pEventQueue->Shutdown();
    }

    SafeRelease(&m_pEventQueue);
    SafeRelease(&m_pSource);
    SafeRelease(&m_pStreamDescriptor);

    m_IsShutdown = TRUE;

    LeaveCriticalSection(&m_critSec);
    return S_OK;
}

//-------------------------------------------------------------------
// Name: SetPosition
// Description: Updates the new stream position.
//-------------------------------------------------------------------

HRESULT CW4allStreamSource::SetPosition(LONGLONG rtNewPosition)
{
    EnterCriticalSection(&m_critSec);

	HRESULT hr = S_OK;

/*
    // Check if the requested position is beyond the end of the stream.
    LONGLONG duration = AudioDurationFromBufferSize(m_pRiff->Format(), m_pRiff->Chunk().DataSize());

    if (rtNewPosition > duration)
    {
        LeaveCriticalSection(&m_critSec);

        return MF_E_INVALIDREQUEST; // Start position is past the end of the presentation.
    }

    if (m_rtCurrentPosition != rtNewPosition)
    {
        LONGLONG offset = BufferSizeFromAudioDuration(m_pRiff->Format(), rtNewPosition);

        // The chunk size is a DWORD. So if our calculations are correct, there is no
        // way that the maximum valid seek position can be larger than a DWORD.
        assert(offset <= MAXDWORD);

        hr = m_pRiff->MoveToChunkOffset((DWORD)offset);

        if (SUCCEEDED(hr))
        {
            m_rtCurrentPosition = rtNewPosition;
            m_discontinuity = TRUE;
            m_EOS = FALSE;
        }
    }
*/
    LeaveCriticalSection(&m_critSec);
    return hr;
}

HRESULT CW4allStreamSource::CheckEndOfStream()
{
    HRESULT hr = S_OK;
/*
    if (m_pRiff->BytesRemainingInChunk() < m_pRiff->Format()->nBlockAlign)
    {
        // The remaining data is smaller than the audio block size. (In theory there shouldn't be
        // partial bits of data at the end, so we should reach an even zero bytes, but the file
        // might not be authored correctly.)
        m_EOS = TRUE;

        // Send the end-of-stream event,
        hr = QueueEvent(MEEndOfStream, GUID_NULL, S_OK, NULL);
    }
	*/
    return hr;
}




//-------------------------------------------------------------------
// Name: QueueEventWithIUnknown
// Description: Helper function to queue an event with an IUnknown
//              pointer value.
//
// pMEG:        Media event generator that will queue the event.
// meType:      Media event type.
// hrStatus:    Status code for the event.
// pUnk:        IUnknown pointer value.
//
//-------------------------------------------------------------------


HRESULT QueueEventWithIUnknown(
    IMFMediaEventGenerator *pMEG,
    MediaEventType meType,
    HRESULT hrStatus,
    IUnknown *pUnk)
{

    // Create the PROPVARIANT to hold the IUnknown value.
    PROPVARIANT var;
    var.vt = VT_UNKNOWN;
    var.punkVal = pUnk;
    pUnk->AddRef();

    // Queue the event.
    HRESULT hr = pMEG->QueueEvent(meType, GUID_NULL, hrStatus, &var);

    // Clear the PROPVARIANT.
    PropVariantClear(&var);

    return hr;
}

LONGLONG AudioDurationFromBufferSize(const WAVEFORMATEX *pWav, DWORD cbAudioDataSize)
{
    assert(pWav != NULL);

    if (pWav->nAvgBytesPerSec == 0)
    {
        return 0;
    }
    return (LONGLONG)cbAudioDataSize * 10000000 / pWav->nAvgBytesPerSec;
}

LONGLONG BufferSizeFromAudioDuration(const WAVEFORMATEX *pWav, LONGLONG duration)
{
    LONGLONG cbSize = duration * pWav->nAvgBytesPerSec / 10000000;

    ULONG ulRemainder = (ULONG)(cbSize % pWav->nBlockAlign);

    // Round up to the next block.
    if(ulRemainder)
    {
        cbSize += pWav->nBlockAlign - ulRemainder;
    }

    return cbSize;
}


