/*
* Copyright (C) 2013 Gotham City. All rights reserved.
*/
/**@file test.cc
 * @brief 
 *
 * @author Batman@GothamCity
 */
#if defined(_MSC_VER)
#	pragma comment(lib, "mfplat.lib")
#	pragma comment(lib, "mf.lib")
#	pragma comment(lib, "mfuuid.lib")
#	pragma comment(lib, "shlwapi.lib")
#endif

#include <stdio.h>
#include <tchar.h>
#include <assert.h>

#include <new>
#include <mfapi.h>
#include <mfidl.h>
#include <Mferror.h>
#include <shlwapi.h>

//#include <initguid.h>
//#include "w4allGuids.h"

//#include "w4allSource.h"
//#include "w4allSink.h"
#include "w4allUtils.h"
#include "mf_custom_src.h"

const UINT32 VIDEO_WIDTH = 640;
const UINT32 VIDEO_HEIGHT = 480;
const UINT32 VIDEO_FPS = 25;

static void testSource();
static void testSink();

#if !defined(GOTHAM_USE_CUSTOM_SRC)
#define GOTHAM_USE_CUSTOM_SRC	1
#endif

// EVR controller(e.g. Fullscreen, position...): http://msdn.microsoft.com/en-us/library/windows/desktop/ms704002(v=vs.85).aspx

static HRESULT CreatePlaybackTopology(
    IMFMediaSource *pSource,          // Media source.
    IMFPresentationDescriptor *pPD,   // Presentation descriptor.
    HWND hVideoWnd,                   // Video window.
    IMFTopology **ppTopology);         // Receives a pointer to the topology.
static HRESULT AddBranchToPartialTopology(
    IMFTopology *pTopology,         // Topology.
    IMFMediaSource *pSource,        // Media source.
    IMFPresentationDescriptor *pPD, // Presentation descriptor.
    DWORD iStream,                  // Stream index.
    HWND hVideoWnd);                 // Window for video playback.
static HRESULT CreateMediaSinkActivate(
    IMFStreamDescriptor *pSourceSD,     // Pointer to the stream descriptor.
    HWND hVideoWindow,                  // Handle to the video clipping window.
    IMFActivate **ppActivate);
static HRESULT AddSourceNode(
    IMFTopology *pTopology,           // Topology.
    IMFMediaSource *pSource,          // Media source.
    IMFPresentationDescriptor *pPD,   // Presentation descriptor.
    IMFStreamDescriptor *pSD,         // Stream descriptor.
    IMFTopologyNode **ppNode);         // Receives the node pointer.
static HRESULT AddOutputNode(
    IMFTopology *pTopology,     // Topology.
    IMFActivate *pActivate,     // Media sink activation object.
    DWORD dwId,                 // Identifier of the stream sink.
    IMFTopologyNode **ppNode);   // Receives the node pointer.
static HRESULT RunSession(
	IMFMediaSession *pSession, 
	IMFTopology *pTopology);
static HRESULT CreateVideoDeviceSource(
	IMFMediaSource **ppSource);
static HWND GetConsoleHwnd(void);

int _tmain(int argc, _TCHAR* argv[])
{
	HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (SUCCEEDED(hr))
    {
        hr = MFStartup(MF_VERSION);
        if (SUCCEEDED(hr))
        {            
            testSource();
			testSink();
            
            MFShutdown();
        }
        CoUninitialize();
    }

	return 0;
}

static void testSource()
{
	HRESULT hr;
	IMFMediaSource* pIMFSource = NULL;
	IMFMediaSession* pIMFSession = NULL;
	IMFTopology* pIMFTopology = NULL;
	HWND hVideoWnd = GetConsoleHwnd();
	IMFPresentationDescriptor *pIMFPD = NULL;

	// Create the Custom Source
	CHECK_HR(hr = CreateVideoDeviceSource(&pIMFSource));

	// Create the Presentation Descriptor
	CHECK_HR(hr = pIMFSource->CreatePresentationDescriptor(&pIMFPD));
	// Create the Media Session
	CHECK_HR(hr = MFCreateMediaSession(NULL, &pIMFSession));
	// Create the Playback Topology
	CHECK_HR(hr = CreatePlaybackTopology(pIMFSource, pIMFPD, hVideoWnd, &pIMFTopology));
	// Run Session
	CHECK_HR(hr = RunSession(pIMFSession, pIMFTopology));

	{
		IMFMediaEvent *pEvent = NULL;

		while (1)
		{
			HRESULT hrStatus = S_OK;
			MediaEventType met;

			CHECK_HR(hr = pIMFSession->GetEvent(0, &pEvent));
			CHECK_HR(hr = pEvent->GetStatus(&hrStatus));
			CHECK_HR(hr = pEvent->GetType(&met));

			if (FAILED(hrStatus))
			{
				switch(hrStatus)
				{
					case MF_E_TOPO_CODEC_NOT_FOUND: printf("MF_E_TOPO_CODEC_NOT_FOUND"); break;
					case MF_E_NO_SAMPLE_TIMESTAMP: printf("MF_E_NO_SAMPLE_TIMESTAMP"); break;
				}
				

				printf("Session error: 0x%x (event id: %d)\n", hrStatus, met);
				hr = hrStatus;
				goto done;
			}
			if (met == MESessionEnded)
			{
				break;
			}
			SafeRelease(&pEvent);
		}
	}

done:
	if(pIMFSession)
	{
		pIMFSession->Shutdown();
	}
	if(pIMFSource)
	{
		pIMFSource->Shutdown();
	}

	SafeRelease(&pIMFSource);
	SafeRelease(&pIMFSession);
	SafeRelease(&pIMFPD);
	SafeRelease(&pIMFTopology);
}

static void testSink()
{

}



//  Create a playback topology from a media source.
static HRESULT CreatePlaybackTopology(
    IMFMediaSource *pSource,          // Media source.
    IMFPresentationDescriptor *pPD,   // Presentation descriptor.
    HWND hVideoWnd,                   // Video window.
    IMFTopology **ppTopology)         // Receives a pointer to the topology.
{
    IMFTopology *pTopology = NULL;
    DWORD cSourceStreams = 0;

    // Create a new topology.
    HRESULT hr = MFCreateTopology(&pTopology);
    if (FAILED(hr))
    {
        goto done;
    }

    // Get the number of streams in the media source.
    hr = pPD->GetStreamDescriptorCount(&cSourceStreams);
    if (FAILED(hr))
    {
        goto done;
    }

    // For each stream, create the topology nodes and add them to the topology.
    for (DWORD i = 0; i < cSourceStreams; i++)
    {
        hr = AddBranchToPartialTopology(pTopology, pSource, pPD, i, hVideoWnd);
        if (FAILED(hr))
        {
            goto done;
        }
    }

    // Return the IMFTopology pointer to the caller.
    *ppTopology = pTopology;
    (*ppTopology)->AddRef();

done:
    SafeRelease(&pTopology);
    return hr;
}


//  Add a topology branch for one stream.
//
//  For each stream, this function does the following:
//
//    1. Creates a source node associated with the stream. 
//    2. Creates an output node for the renderer. 
//    3. Connects the two nodes.
//
//  The media session will add any decoders that are needed.

static HRESULT AddBranchToPartialTopology(
    IMFTopology *pTopology,         // Topology.
    IMFMediaSource *pSource,        // Media source.
    IMFPresentationDescriptor *pPD, // Presentation descriptor.
    DWORD iStream,                  // Stream index.
    HWND hVideoWnd)                 // Window for video playback.
{
    IMFStreamDescriptor *pSD = NULL;
    IMFActivate         *pSinkActivate = NULL;
    IMFTopologyNode     *pSourceNode = NULL;
    IMFTopologyNode     *pOutputNode = NULL;

    BOOL fSelected = FALSE;

    HRESULT hr = pPD->GetStreamDescriptorByIndex(iStream, &fSelected, &pSD);
    if (FAILED(hr))
    {
        goto done;
    }

    if (fSelected)
    {
        // Create the media sink activation object.
        hr = CreateMediaSinkActivate(pSD, hVideoWnd, &pSinkActivate);
        if (FAILED(hr))
        {
            goto done;
        }

        // Add a source node for this stream.
        hr = AddSourceNode(pTopology, pSource, pPD, pSD, &pSourceNode);
        if (FAILED(hr))
        {
            goto done;
        }

        // Create the output node for the renderer.
        hr = AddOutputNode(pTopology, pSinkActivate, 0, &pOutputNode);
        if (FAILED(hr))
        {
            goto done;
        }

        // Connect the source node to the output node.
        hr = pSourceNode->ConnectOutput(0, pOutputNode, 0);
    }
    // else: If not selected, don't add the branch. 

done:
    SafeRelease(&pSD);
    SafeRelease(&pSinkActivate);
    SafeRelease(&pSourceNode);
    SafeRelease(&pOutputNode);
    return hr;
}

//  Create an activation object for a renderer, based on the stream media type.

static HRESULT CreateMediaSinkActivate(
    IMFStreamDescriptor *pSourceSD,     // Pointer to the stream descriptor.
    HWND hVideoWindow,                  // Handle to the video clipping window.
    IMFActivate **ppActivate
)
{
    IMFMediaTypeHandler *pHandler = NULL;
    IMFActivate *pActivate = NULL;

    // Get the media type handler for the stream.
    HRESULT hr = pSourceSD->GetMediaTypeHandler(&pHandler);
    if (FAILED(hr))
    {
        goto done;
    }

    // Get the major media type.
    GUID guidMajorType;
    hr = pHandler->GetMajorType(&guidMajorType);
    if (FAILED(hr))
    {
        goto done;
    }
 
    // Create an IMFActivate object for the renderer, based on the media type.
    if (MFMediaType_Audio == guidMajorType)
    {
        // Create the audio renderer.
        hr = MFCreateAudioRendererActivate(&pActivate);
    }
    else if (MFMediaType_Video == guidMajorType)
    {
        // Create the video renderer.
        hr = MFCreateVideoRendererActivate(hVideoWindow, &pActivate);
    }
    else
    {
        // Unknown stream type. 
        hr = E_FAIL;
        // Optionally, you could deselect this stream instead of failing.
    }
    if (FAILED(hr))
    {
        goto done;
    }
 
    // Return IMFActivate pointer to caller.
    *ppActivate = pActivate;
    (*ppActivate)->AddRef();

done:
    SafeRelease(&pHandler);
    SafeRelease(&pActivate);
    return hr;
}

// Add a source node to a topology.
static HRESULT AddSourceNode(
    IMFTopology *pTopology,           // Topology.
    IMFMediaSource *pSource,          // Media source.
    IMFPresentationDescriptor *pPD,   // Presentation descriptor.
    IMFStreamDescriptor *pSD,         // Stream descriptor.
    IMFTopologyNode **ppNode)         // Receives the node pointer.
{
    IMFTopologyNode *pNode = NULL;

    HRESULT hr = S_OK;
    CHECK_HR(hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &pNode));
    CHECK_HR(hr = pNode->SetUnknown(MF_TOPONODE_SOURCE, pSource));
    CHECK_HR(hr = pNode->SetUnknown(MF_TOPONODE_PRESENTATION_DESCRIPTOR, pPD));
    CHECK_HR(hr = pNode->SetUnknown(MF_TOPONODE_STREAM_DESCRIPTOR, pSD));
    CHECK_HR(hr = pTopology->AddNode(pNode));

    // Return the pointer to the caller.
    *ppNode = pNode;
    (*ppNode)->AddRef();

done:
    SafeRelease(&pNode);
    return hr;
}

// Add an output node to a topology.
static HRESULT AddOutputNode(
    IMFTopology *pTopology,     // Topology.
    IMFActivate *pActivate,     // Media sink activation object.
    DWORD dwId,                 // Identifier of the stream sink.
    IMFTopologyNode **ppNode)   // Receives the node pointer.
{
    IMFTopologyNode *pNode = NULL;

    HRESULT hr = S_OK;
    CHECK_HR(hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &pNode));
    CHECK_HR(hr = pNode->SetObject(pActivate));
    CHECK_HR(hr = pNode->SetUINT32(MF_TOPONODE_STREAMID, dwId));
    CHECK_HR(hr = pNode->SetUINT32(MF_TOPONODE_NOSHUTDOWN_ON_REMOVE, FALSE));
    CHECK_HR(hr = pTopology->AddNode(pNode));

    // Return the pointer to the caller.
    *ppNode = pNode;
    (*ppNode)->AddRef();

done:
    SafeRelease(&pNode);
    return hr;
}

// Run session
static HRESULT RunSession(IMFMediaSession *pSession, IMFTopology *pTopology)
{
    IMFMediaEvent *pEvent = NULL;

    PROPVARIANT var;
    PropVariantInit(&var);

	MediaEventType met;
	HRESULT hrStatus = S_OK;
    HRESULT hr = S_OK;
    CHECK_HR(hr = pSession->SetTopology(0, pTopology));
    CHECK_HR(hr = pSession->Start(&GUID_NULL, &var));

	// Check first event
	hr = pSession->GetEvent(MF_EVENT_FLAG_NO_WAIT, &pEvent);
	if(hr == MF_E_NO_EVENTS_AVAILABLE){
		hr = S_OK;
		goto done;
	}
    CHECK_HR(hr = pEvent->GetStatus(&hrStatus));
	if (FAILED(hrStatus))
    {
		CHECK_HR(hr = pEvent->GetType(&met));
		printf_s("Session error: 0x%x (event id: %d)\n", hrStatus, met);
        hr = hrStatus;
        goto done;
    }

done:
    SafeRelease(&pEvent);
    return hr;
}


static HRESULT CreateVideoDeviceSource(IMFMediaSource **ppSource)
{
#if GOTHAM_USE_CUSTOM_SRC
	/*return CoCreateInstance(
                CLSID_WebRTC4AllSourceMFT, NULL,
                CLSCTX_INPROC_SERVER,
				IID_IMFMediaSource,
                (void**)ppSource);*/

	HRESULT hr = S_OK;
	IMFMediaType *pMediaType = NULL;

	CHECK_HR(hr = MFCreateMediaType(&pMediaType));
	CHECK_HR(hr = pMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
	CHECK_HR(hr = pMediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12));     
    CHECK_HR(hr = pMediaType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive));
	CHECK_HR(hr = pMediaType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE));
    CHECK_HR(hr = MFSetAttributeSize(pMediaType, MF_MT_FRAME_SIZE, VIDEO_WIDTH, VIDEO_HEIGHT));       
    CHECK_HR(hr = MFSetAttributeRatio(pMediaType, MF_MT_FRAME_RATE, VIDEO_FPS, 1));       
    CHECK_HR(hr = MFSetAttributeRatio(pMediaType, MF_MT_PIXEL_ASPECT_RATIO, 1, 1));

	CHECK_HR(hr = CMFSource::CreateInstanceEx(IID_IMFMediaSource, (void**)ppSource, pMediaType));

done:
	SafeRelease(&pMediaType);
	return hr;
#else
    *ppSource = NULL;

    IMFMediaSource *pSource = NULL;
    IMFAttributes *pAttributes = NULL;
    IMFActivate **ppDevices = NULL;

    // Create an attribute store to specify the enumeration parameters.
    HRESULT hr = MFCreateAttributes(&pAttributes, 1);
    if (FAILED(hr))
    {
        goto done;
    }

    // Source type: video capture devices
    hr = pAttributes->SetGUID(
        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, 
        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID
        );
    if (FAILED(hr))
    {
        goto done;
    }

    // Enumerate devices.
    UINT32 count;
    hr = MFEnumDeviceSources(pAttributes, &ppDevices, &count);
    if (FAILED(hr))
    {
        goto done;
    }

    if (count == 0)
    {
        hr = E_FAIL;
        goto done;
    }

    // Create the media source object.
    hr = ppDevices[0]->ActivateObject(IID_PPV_ARGS(&pSource));
    if (FAILED(hr))
    {
        goto done;
    }

    *ppSource = pSource;
    (*ppSource)->AddRef();

done:
    SafeRelease(&pAttributes);

    for (DWORD i = 0; i < count; i++)
    {
        SafeRelease(&ppDevices[i]);
    }
    CoTaskMemFree(ppDevices);
    SafeRelease(&pSource);
    return hr;
#endif
}

static HWND GetConsoleHwnd(void)
{
   #define MY_BUFSIZE 1024 // Buffer size for console window titles.
   HWND hwndFound;         // This is what is returned to the caller.
   TCHAR pszNewWindowTitle[MY_BUFSIZE]; // Contains fabricated
                                       // WindowTitle.
   TCHAR pszOldWindowTitle[MY_BUFSIZE]; // Contains original
                                       // WindowTitle.

   // Fetch current window title.
   GetConsoleTitle(pszOldWindowTitle, MY_BUFSIZE);

   // Format a "unique" NewWindowTitle.
   wsprintf(pszNewWindowTitle,TEXT("%d/%d"),
               GetTickCount(),
               GetCurrentProcessId());

   // Change current window title.
   SetConsoleTitle(pszNewWindowTitle);

   // Ensure window title has been updated.
   Sleep(40);

   // Look for NewWindowTitle.
   hwndFound=FindWindow(NULL, pszNewWindowTitle);

   // Restore original window title.
   SetConsoleTitle(pszOldWindowTitle);

   return(hwndFound);
}