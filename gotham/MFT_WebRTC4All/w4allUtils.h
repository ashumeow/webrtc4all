/*
* Copyright (C) 2013 Gotham City. All rights reserved.
* Copyright (C) Microsoft Corporation. All rights reserved.
*/
/**@file w4allSink.h
 * @brief Utility functions
 *
 * @author Batman@GothamCity
 */
#pragma once

#include <strsafe.h>
#include <new>
#include <uuids.h>      // DirectShow GUIDs

template <class T> void SafeRelease(T **ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

#if !defined(CHECK_HR)
#	define CHECK_HR(hr) if (FAILED(hr)) { goto done; }
#endif

// The stream ID of the one stream on the sink.
static const DWORD W4A_SINK_STREAM_ID = 1;


// Video FOURCC codes.
static const FOURCC FOURCC_YUY2 = MAKEFOURCC('Y', 'U', 'Y', '2');
static const FOURCC FOURCC_UYVY = MAKEFOURCC('U', 'Y', 'V', 'Y');
static const FOURCC FOURCC_NV12 = MAKEFOURCC('N', 'V', '1', '2');

// Static array of media types (preferred and accepted).
static const GUID* g_VideoSubtypes[] =
{
#if 0
	& MEDIASUBTYPE_NV12,
    & MEDIASUBTYPE_YUY2,
    & MEDIASUBTYPE_UYVY
#else
    & MFVideoFormat_NV12,
    & MFVideoFormat_YUY2,
    & MFVideoFormat_UYVY,
#endif
};


// Number of media types in the aray.
static const DWORD g_NumVideoSubtypes = ARRAYSIZE(g_VideoSubtypes);

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

static const PCM_Audio_Format_Params g_AudioFormats[] =
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

static const DWORD g_NumAudioFormats = ARRAYSIZE(g_AudioFormats);


/*---------------------------------------------------------------------

 ComPtrList template

 Implements a list of COM pointers.

 Template parameters:

    T:        COM interface type.

    NULLABLE: If true, client can insert NULL pointers. By default,
              the list does not allow NULL pointers.

 Examples:

     ComPtrList<IUnknown>
     ComPtrList<IUnknown, TRUE>


 The list supports enumeration:

     for (ComPtrList<IUnknown>::POSIITON pos = list.FrontPosition();
          pos != list.EndPosition();
          pos = list.Next(pos))
     {
         IUnknown *pUnk = NULL;

         hr = list.GetItemByPosition(pos, &pUnk);

         if (SUCCEEDED(hr))
         {
             pUnk->Release();
         }
     }

 The list is implemented as a double-linked list with an anchor node.

---------------------------------------------------------------------*/

template <class T, bool NULLABLE = FALSE>
class ComPtrList
{
protected:

    typedef T* Ptr;

    // Nodes in the linked list
    struct Node
    {
        Node *prev;
        Node *next;
        Ptr  item;

        Node() : prev(NULL), next(NULL)
        {
        }

        Node(Ptr item) : prev(NULL), next(NULL)
        {
            this->item = item;
            if (item)
            {
                item->AddRef();
            }
        }
        ~Node()
        {
            if (item)
            {
                item->Release();
            }
        }

        Ptr Item() const { return item; }
    };

public:

    // Object for enumerating the list.
    class POSITION
    {
        friend class ComPtrList<T>;

    public:
        POSITION() : pNode(NULL)
        {
        }

        bool operator==(const POSITION &p) const
        {
            return pNode == p.pNode;
        }

        bool operator!=(const POSITION &p) const
        {
            return pNode != p.pNode;
        }

    private:
        const Node *pNode;

        POSITION(Node *p) : pNode(p)
        {
        }
    };

protected:
    Node    m_anchor;  // Anchor node for the linked list.
    DWORD   m_count;   // Number of items in the list.

    Node* Front() const
    {
        return m_anchor.next;
    }

    Node* Back() const
    {
        return m_anchor.prev;
    }

    virtual HRESULT InsertAfter(Ptr item, Node *pBefore)
    {
        if (pBefore == NULL)
        {
            return E_POINTER;
        }

        // Do not allow NULL item pointers unless NULLABLE is true.
        if (!item && !NULLABLE)
        {
            return E_POINTER;
        }

        Node *pNode = new (std::nothrow) Node(item);
        if (pNode == NULL)
        {
            return E_OUTOFMEMORY;
        }

        Node *pAfter = pBefore->next;

        pBefore->next = pNode;
        pAfter->prev = pNode;

        pNode->prev = pBefore;
        pNode->next = pAfter;

        m_count++;

        return S_OK;
    }

    virtual HRESULT GetItem(const Node *pNode, Ptr* ppItem)
    {
        if (pNode == NULL || ppItem == NULL)
        {
            return E_POINTER;
        }

        *ppItem = pNode->item;

        if (*ppItem)
        {
            (*ppItem)->AddRef();
        }

        return S_OK;
    }

    // RemoveItem:
    // Removes a node and optionally returns the item.
    // ppItem can be NULL.
    virtual HRESULT RemoveItem(Node *pNode, Ptr *ppItem)
    {
        if (pNode == NULL)
        {
            return E_POINTER;
        }

        assert(pNode != &m_anchor); // We should never try to remove the anchor node.
        if (pNode == &m_anchor)
        {
            return E_INVALIDARG;
        }


        Ptr item;

        // The next node's previous is this node's previous.
        pNode->next->prev = pNode->prev;

        // The previous node's next is this node's next.
        pNode->prev->next = pNode->next;

        item = pNode->item;

        if (ppItem)
        {
            *ppItem = item;

            if (*ppItem)
            {
                (*ppItem)->AddRef();
            }
        }

        delete pNode;
        m_count--;

        return S_OK;
    }

public:

    ComPtrList()
    {
        m_anchor.next = &m_anchor;
        m_anchor.prev = &m_anchor;

        m_count = 0;
    }

    virtual ~ComPtrList()
    {
        Clear();
    }

    void Clear()
    {
        Node *n = m_anchor.next;

        // Delete the nodes
        while (n != &m_anchor)
        {
            if (n->item)
            {
                n->item->Release();
				n->item = NULL; // FIXME: added by MDI
            }

            Node *tmp = n->next;
            delete n;
            n = tmp;
        }

        // Reset the anchor to point at itself
        m_anchor.next = &m_anchor;
        m_anchor.prev = &m_anchor;

        m_count = 0;
    }

    // Insertion functions
    HRESULT InsertBack(Ptr item)
    {
        return InsertAfter(item, m_anchor.prev);
    }


    HRESULT InsertFront(Ptr item)
    {
        return InsertAfter(item, &m_anchor);
    }

    // RemoveBack: Removes the tail of the list and returns the value.
    // ppItem can be NULL if you don't want the item back.
    HRESULT RemoveBack(Ptr *ppItem)
    {
        if (IsEmpty())
        {
            return E_FAIL;
        }
        else
        {
            return RemoveItem(Back(), ppItem);
        }
    }

    // RemoveFront: Removes the head of the list and returns the value.
    // ppItem can be NULL if you don't want the item back.
    HRESULT RemoveFront(Ptr *ppItem)
    {
        if (IsEmpty())
        {
            return E_FAIL;
        }
        else
        {
            return RemoveItem(Front(), ppItem);
        }
    }

    // GetBack: Gets the tail item.
    HRESULT GetBack(Ptr *ppItem)
    {
        if (IsEmpty())
        {
            return E_FAIL;
        }
        else
        {
            return GetItem(Back(), ppItem);
        }
    }

    // GetFront: Gets the front item.
    HRESULT GetFront(Ptr *ppItem)
    {
        if (IsEmpty())
        {
            return E_FAIL;
        }
        else
        {
            return GetItem(Front(), ppItem);
        }
    }


    // GetCount: Returns the number of items in the list.
    DWORD GetCount() const { return m_count; }

    bool IsEmpty() const
    {
        return (GetCount() == 0);
    }

    // Enumerator functions

    POSITION FrontPosition()
    {
        if (IsEmpty())
        {
            return POSITION(NULL);
        }
        else
        {
            return POSITION(Front());
        }
    }

    POSITION EndPosition() const
    {
        return POSITION();
    }

    HRESULT GetItemByPosition(POSITION pos, Ptr *ppItem)
    {
        if (pos.pNode)
        {
            return GetItem(pos.pNode, ppItem);
        }
        else
        {
            return E_FAIL;
        }
    }

    POSITION Next(const POSITION pos)
    {
        if (pos.pNode && (pos.pNode->next != &m_anchor))
        {
            return POSITION(pos.pNode->next);
        }
        else
        {
            return POSITION(NULL);
        }
    }
};



//-------------------------------------------------------------------
// Name: ValidateWaveFormat
// Description: Validates a WAVEFORMATEX structure.
//
// This method is called when the byte stream handler opens the
// source. The WAVEFORMATEX structure is copied directly from the
// .wav file. Therefore the source should not trust any of the
// values in the format header.
//
// Just to keep the sample as simple as possible, we only accept
// uncompressed PCM formats in this media source.
//-------------------------------------------------------------------


static HRESULT ValidateWaveFormat(const WAVEFORMATEX *pWav, DWORD cbSize)
{
    if (cbSize < sizeof(WAVEFORMATEX))
    {
        return MF_E_INVALIDMEDIATYPE;
    }

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
// CreatePCMAudioType
//
// Creates a media type that describes an uncompressed PCM audio
// format.
//-------------------------------------------------------------------

static HRESULT CreatePCMAudioType(
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
static HRESULT CreateVideoType(
		const GUID* subType, // video subType
		IMFMediaType **ppType,     // Receives a pointer to the media type.
		UINT32 unWidth = 0, // Video width (0 to ignore)
		UINT32 unHeight = 0 // Video height (0 to ignore)
	)
{
	HRESULT hr = S_OK;

    IMFMediaType *pType = NULL;

    CHECK_HR(hr = MFCreateMediaType(&pType));

    CHECK_HR(hr = pType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));

    CHECK_HR(hr = pType->SetGUID(MF_MT_SUBTYPE, *subType));

	CHECK_HR(hr = pType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE)); // UnCompressed

	CHECK_HR(hr = pType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive));

	if(unWidth > 0 && unHeight > 0)
	{
		CHECK_HR(hr = MFSetAttributeSize(pType, MF_MT_FRAME_SIZE, unWidth, unHeight));
	}
	
    *ppType = pType;
    (*ppType)->AddRef();

done:
    SafeRelease(&pType);
    return hr;
}

//-------------------------------------------------------------------
// Name: ValidateVideoFormat
// Description: Validates a media type for this sink.
//-------------------------------------------------------------------
static HRESULT ValidateVideoFormat(IMFMediaType *pmt)
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