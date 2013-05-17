/*
* Copyright (C) 2013 Gotham City. All rights reserved.
* Copyright (C) Microsoft Corporation. All rights reserved.
*/
/**@file w4allSink.h
 * @brief Audio/Video Media Foundation Sink (header)
 *
 * @author Batman@GothamCity
 */

#include <windows.h>
#include <mfidl.h>
#include <mfapi.h>
#include <mferror.h>

#include <assert.h>

#include "w4allUtils.h"

class CW4allStream;
class CW4allMarker;

enum FlushState
{
    DropSamples = 0,
    WriteSamples
};


// IMarker:
// Custom interface for handling IMFStreamSink::PlaceMarker calls asynchronously.

// A marker consists of a marker type, marker data, and context data.
// By defining this interface, we can store the marker data inside an IUnknown object
// and keep that object on the same queue that holds the media samples. This is
// useful because samples and markers must be serialized. That is, we cannot respond
// to a marker until we have processed all of the samples that came before it.

// Note that IMarker is not a standard Media Foundation interface.
MIDL_INTERFACE("3AC82233-933C-43a9-AF3D-ADC94EABF406")
IMarker : public IUnknown
{
    virtual STDMETHODIMP GetMarkerType(MFSTREAMSINK_MARKER_TYPE *pType) = 0;
    virtual STDMETHODIMP GetMarkerValue(PROPVARIANT *pvar) = 0;
    virtual STDMETHODIMP GetContext(PROPVARIANT *pvar) = 0;
};



class CW4allSink : public IMFFinalizableMediaSink,   // Note: IMFFinalizableMediaSink inherits IMFMediaSink
                 public IMFClockStateSink
{

    friend class CW4allStream;

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

private:

    CW4allSink(bool bVideo);
    virtual ~CW4allSink();

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


	bool						m_bVideo;					// whether it's a video sink or not
    long                        m_nRefCount;                // reference count
    CRITICAL_SECTION            m_critSec;                  // critical section for thread safety

    BOOL                        m_IsShutdown;               // Flag to indicate if Shutdown() method was called.

    CW4allStream                  *m_pStream;                 // Byte stream
    IMFPresentationClock        *m_pClock;                  // Presentation clock.
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
class AsyncCallback : public IMFAsyncCallback
{
public:
    typedef HRESULT (T::*InvokeFn)(IMFAsyncResult *pAsyncResult);

    AsyncCallback(T *pParent, InvokeFn fn) : m_pParent(pParent), m_pInvokeFn(fn)
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

class CW4allStream : public IMFStreamSink, public IMFMediaTypeHandler
{
    friend class CW4allSink;

public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID iid, void** ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IMFMediaEventGenerator
    STDMETHODIMP BeginGetEvent(IMFAsyncCallback* pCallback,IUnknown* punkState);
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


private:

    // State enum: Defines the current state of the stream.
    enum State
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
    enum StreamOperation
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

    class CAsyncOperation : public IUnknown
    {
    public:
        CAsyncOperation(StreamOperation op);

        StreamOperation m_op;   // The operation to perform.

        // IUnknown methods.
        STDMETHODIMP QueryInterface(REFIID iid, void** ppv);
        STDMETHODIMP_(ULONG) AddRef();
        STDMETHODIMP_(ULONG) Release();

    private:
        long    m_nRefCount;
        virtual ~CAsyncOperation();
    };


    // ValidStateMatrix: Defines a look-up table that says which operations
    // are valid from which states.
    static BOOL ValidStateMatrix[State_Count][Op_Count];


    CW4allStream(bool bVideo);
    virtual ~CW4allStream();

    HRESULT Initialize(CW4allSink *pParent);

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

    HRESULT     ValidateOperation(StreamOperation op);

    HRESULT     QueueAsyncOperation(StreamOperation op);

    HRESULT     OnDispatchWorkItem(IMFAsyncResult* pAsyncResult);
    HRESULT     DispatchProcessSample(CAsyncOperation* pOp);
    HRESULT     DispatchFinalize(CAsyncOperation* pOp);

    HRESULT     ProcessSamplesFromQueue(FlushState bFlushData);
    HRESULT     SendSampleOverNetwork(IMFSample *pSample);
    HRESULT     SendMarkerEvent(IMarker *pMarker, FlushState bFlushData);


	bool						m_bVideo;					// whether it's a video stream (otherwise it's audio)
    long                        m_nRefCount;                // reference count
    CRITICAL_SECTION            m_critSec;                  // critical section for thread safety

    State                       m_state;
    BOOL                        m_IsShutdown;               // Flag to indicate if Shutdown() method was called.

    DWORD                       m_WorkQueueId;              // ID of the work queue for asynchronous operations.
    AsyncCallback<CW4allStream>   m_WorkQueueCB;              // Callback for the work queue.

    MFTIME                      m_StartTime;                // Presentation time when the clock started.
    DWORD                       m_cbDataWritten;            // How many bytes we have written so far. (FIXME: remove)

    CW4allSink                    *m_pSink;                   // Parent media sink

    IMFMediaEventQueue          *m_pEventQueue;             // Event queue
    IMFMediaType                *m_pCurrentType;

    ComPtrList<IUnknown>        m_SampleQueue;              // Queue to hold samples and markers.
                                                            // Applies to: ProcessSample, PlaceMarker, BeginFinalize

    IMFAsyncResult              *m_pFinalizeResult;         // Result object for Finalize operation.

};




// Holds marker information for IMFStreamSink::PlaceMarker

class CW4allMarker : public IMarker
{
public:
    static HRESULT Create(
        MFSTREAMSINK_MARKER_TYPE eMarkerType,
        const PROPVARIANT* pvarMarkerValue,
        const PROPVARIANT* pvarContextValue,
        IMarker **ppMarker
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

    CW4allMarker(MFSTREAMSINK_MARKER_TYPE eMarkerType);
    virtual ~CW4allMarker();
};


