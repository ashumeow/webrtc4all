#include <initguid.h>
#include <Windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <Mfreadwrite.h>
#include <mferror.h>
#include <Evr.h>

#include <initguid.h>
#include <d3d9.h>
#include <dxva2api.h>

//#include "EVRPresenter/EVRPresenter.h"


#include <stdio.h>

#pragma comment(lib, "mfreadwrite")
#pragma comment(lib, "mfplat")
#pragma comment(lib, "mfuuid")
#pragma comment(lib, "Mf")
#pragma comment(lib, "Strmiids")
#pragma comment(lib, "Evr")
#pragma comment(lib, "dxva2")
#pragma comment(lib, "d3d9")
#pragma comment(lib, "winmm")
#pragma comment(lib, "shlwapi")



#undef CHECK_HR
#define CHECK_HR(x) if (FAILED(x)) { fprintf(stdout, "Operation Failed"); goto bail; }

const DWORD NUM_BACK_BUFFERS = 2;
const UINT32 VIDEO_WIDTH = 640;
const UINT32 VIDEO_HEIGHT = 480;
const UINT32 VIDEO_FPS = 25;
//const UINT32 VIDEO_BUFFER_SIZE = (VIDEO_WIDTH * VIDEO_HEIGHT * 3) >> 1;
const UINT32 VIDEO_BUFFER_SIZE = (VIDEO_WIDTH * VIDEO_HEIGHT << 2);

template <class T> void SafeRelease(T **ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

class C2DBufferLock
{
public:
    C2DBufferLock(IDirect3DSurface9 *pSurface, IMFSample *pD3Sample) : m_pD3Sample(NULL), m_p2DBuffer(NULL), m_bLocked(FALSE)
    {
		HRESULT hr = S_OK;

		IMFMediaBuffer *pBuffer = NULL;
		
		/* CHECK_HR(hr = MFGetService(pBuffer, MR_BUFFER_SERVICE, __uuidof(IDirect3DSurface9),  (void**)&pSurface)); */	
		CHECK_HR(hr = MFCreateDXSurfaceBuffer(__uuidof(IDirect3DSurface9), pSurface, TRUE, /* FLIP? */ &pBuffer));		
		CHECK_HR(hr = pBuffer->QueryInterface(__uuidof(IMF2DBuffer), (void**)&m_p2DBuffer));
		CHECK_HR(hr = pD3Sample->AddBuffer(pBuffer));

		// FIXME:
		CHECK_HR(hr = pBuffer->SetCurrentLength(307200));

		m_pD3Sample = pD3Sample;
		m_pBuffer = pBuffer;
		m_pD3Sample->AddRef();
		m_pBuffer->AddRef();
		
bail:
		SafeRelease(&pBuffer);
    }

    ~C2DBufferLock()
    {
        UnlockBuffer();
		SafeRelease(&m_pD3Sample);
        SafeRelease(&m_pBuffer);
        SafeRelease(&m_p2DBuffer);
    }

	IMFSample* GetD3Sample()
	{
		return m_pD3Sample;
	}

	HRESULT SetSampleTime(LONGLONG hnsSampleTime)
	{
		if(m_pD3Sample)
		{
			return m_pD3Sample->SetSampleTime(hnsSampleTime);
		}
		return E_FAIL;
	}

	HRESULT SetSampleDuration(LONGLONG hnsSampleDuration)
	{
		if(m_pD3Sample)
		{
			return m_pD3Sample->SetSampleDuration(hnsSampleDuration);
		}
		return E_FAIL;
	}

    HRESULT LockBuffer(
        LONG  lDefaultStride,    // Minimum stride (with no padding).
        DWORD dwHeightInPixels,  // Height of the image, in pixels.
        BYTE  **ppbScanLine0,    // Receives a pointer to the start of scan line 0.
        LONG  *plStride          // Receives the actual stride.
        )
    {
        HRESULT hr = S_OK;

        // Use the 2-D version if available.
        if (m_p2DBuffer)
        {
            hr = m_p2DBuffer->Lock2D(ppbScanLine0, plStride);
        }
        else
        {
            // Use non-2D version.
            BYTE *pData = NULL;

            hr = m_pBuffer->Lock(&pData, NULL, NULL);
            if (SUCCEEDED(hr))
            {
                *plStride = lDefaultStride;
                if (lDefaultStride < 0)
                {
                    // Bottom-up orientation. Return a pointer to the start of the
                    // last row *in memory* which is the top row of the image.
                    *ppbScanLine0 = pData + abs(lDefaultStride) * (dwHeightInPixels - 1);
                }
                else
                {
                    // Top-down orientation. Return a pointer to the start of the
                    // buffer.
                    *ppbScanLine0 = pData;
                }
            }
        }

        m_bLocked = (SUCCEEDED(hr));

        return hr;
    }
    
    HRESULT UnlockBuffer()
    {
        if (m_bLocked)
        {
            if (m_p2DBuffer)
            {
                (void)m_p2DBuffer->Unlock2D();
            }
            else
            {
                (void)m_pBuffer->Unlock();
            }
            m_bLocked = FALSE;
        }
		return S_OK;
    }

private:
	IMFSample		*m_pD3Sample;
    IMFMediaBuffer  *m_pBuffer;
    IMF2DBuffer     *m_p2DBuffer;

    BOOL            m_bLocked;
};

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

static HRESULT CreateMediaSample(
	DWORD cbData,
	IMFVideoSampleAllocator *pAllocator, // Could be NULL.
	IMFSample **ppSample)
{
    HRESULT hr = S_OK;

    IMFSample *pSample = NULL;
    IMFMediaBuffer *pBuffer = NULL;

	CHECK_HR(hr = pAllocator ? pAllocator->AllocateSample(&pSample) : MFCreateSample(&pSample));    
    CHECK_HR(hr = MFCreateMemoryBuffer(cbData, &pBuffer));
    CHECK_HR(hr = pSample->AddBuffer(pBuffer));
    
    *ppSample = pSample;
    (*ppSample)->AddRef();

bail:
    SafeRelease(&pSample);
    SafeRelease(&pBuffer);
    return hr;
}

static HRESULT LockDevice(
    IDirect3DDeviceManager9 *pDeviceManager,
    BOOL fBlock,
    IDirect3DDevice9 **ppDevice, // Receives a pointer to the device.
    HANDLE *pHandle              // Receives a device handle.   
    )
{
    *pHandle = NULL;
    *ppDevice = NULL;

    HANDLE hDevice = NULL;

	HRESULT hr = S_OK;

    CHECK_HR(hr = pDeviceManager->OpenDeviceHandle(&hDevice));
    hr = pDeviceManager->LockDevice(hDevice, ppDevice, fBlock);

    if (hr == DXVA2_E_NEW_VIDEO_DEVICE)
    {
        // Invalid device handle. Try to open a new device handle.
        CHECK_HR(hr = pDeviceManager->CloseDeviceHandle(hDevice));        
        CHECK_HR(hr = pDeviceManager->OpenDeviceHandle(&hDevice));
        // Try to lock the device again.
        CHECK_HR(hr = pDeviceManager->LockDevice(hDevice, ppDevice, TRUE)); 
    }
    
    *pHandle = hDevice;

bail:
    return hr;
}

static HRESULT GetSwapChainPresentParameters(
	IDirect3DDevice9 *pDevice,
	IMFMediaType *pType, 
	HWND hWindow,
	D3DPRESENT_PARAMETERS* pPP
	)
{
  // Caller holds the object lock.

  HRESULT hr = S_OK; 

  UINT32 width = VIDEO_WIDTH, height = VIDEO_HEIGHT;
  DWORD d3dFormat = 0;

  CHECK_HR(hr = MFGetAttributeSize(pType, MF_MT_FRAME_SIZE, &width, &height)); 

  ZeroMemory(pPP, sizeof(D3DPRESENT_PARAMETERS));
/*
  // http://msdn.microsoft.com/en-us/library/bb172588(VS.85).aspx
  ZeroMemory(pPP, sizeof(D3DPRESENT_PARAMETERS));
  pPP->BackBufferWidth = width;
  pPP->BackBufferHeight = height;
  pPP->Windowed = TRUE;
  pPP->SwapEffect = D3DSWAPEFFECT_FLIPEX;
  pPP->BackBufferFormat = (D3DFORMAT)d3dFormat;
  pPP->hDeviceWindow = hWindow;
  pPP->Windowed = TRUE;
  pPP->Flags = D3DPRESENTFLAG_VIDEO;
  pPP->PresentationInterval = D3DPRESENT_INTERVAL_ONE;

  D3DDEVICE_CREATION_PARAMETERS params;
  CHECK_HR(hr = pDevice->GetCreationParameters(&params));
    
  if (params.DeviceType != D3DDEVTYPE_HAL)
  {
    pPP->Flags |= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
  }
*/

  pPP->BackBufferWidth  = width;
  pPP->BackBufferHeight = height;
  pPP->Windowed = TRUE;
  pPP->SwapEffect = D3DSWAPEFFECT_FLIP;
  pPP->hDeviceWindow = hWindow;
  pPP->BackBufferFormat = (D3DFORMAT)d3dFormat;
  pPP->Flags =
        D3DPRESENTFLAG_VIDEO | D3DPRESENTFLAG_DEVICECLIP |
        D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
  pPP->PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
  pPP->BackBufferCount = NUM_BACK_BUFFERS;

bail:
  return hr;
}

static HRESULT CreateD3DSample(
    IDirect3DSwapChain9 *pSwapChain,
	IDirect3DDevice9 *pDevice,
	C2DBufferLock** pp2DBufferLock
    )
{
    // Caller holds the object lock.

	*pp2DBufferLock = NULL;

	HRESULT hr = S_OK;

    D3DCOLOR clrBlack = D3DCOLOR_ARGB(0xFF, 0x00, 0x00, 0x00);

    IDirect3DSurface9* pSurface = NULL;
    IMFSample* pSample = NULL;
	IMFMediaBuffer *pBuffer = NULL;

    // Get the back buffer surface.
    CHECK_HR(hr = pSwapChain->GetBackBuffer( 0, D3DBACKBUFFER_TYPE_MONO, &pSurface));
    // Fill it with black.
    CHECK_HR(hr = pDevice->ColorFill(pSurface, NULL, clrBlack));
    // Create the sample.
	// IMFSample returns E_NOIMPL when you call Lock() as explained at http://msdn.microsoft.com/en-us/library/windows/desktop/bb530110(v=vs.85).aspx
    CHECK_HR(hr = MFCreateVideoSampleFromSurface(NULL, &pSample));	
	// Wrap the Sample
	if(!(*pp2DBufferLock = new C2DBufferLock(pSurface, pSample)))
	{
		return E_OUTOFMEMORY;
	}

bail:
    SafeRelease(&pSurface);
    SafeRelease(&pSample);
	SafeRelease(&pBuffer);
    return hr;
}

void main()
{
	HRESULT hr = S_OK;
	IMFMediaSink* pMediaSink = NULL;
	IMFMediaType* pMediaType = NULL;
	IMFVideoDisplayControl *pDisplayControl = NULL;
	IMFStreamSink *pStreamSink = NULL;
	IMFMediaTypeHandler *pMediaTypeHandler = NULL;
	IMFGetService *pService = NULL;
	IMFPresentationClock *pClock = NULL;
	IMFPresentationTimeSource *pTimeSource = NULL;
	DWORD dwCharacteristics = 0;
	DWORD dwTypeCount = 0;
	LONGLONG rtStart = 0;
    UINT64 rtDuration;
	IMFVideoSampleAllocator *pSampleAllocator = NULL; /* EVR requires D3D-surface based samples */
	IDirect3DDeviceManager9* pD3DDeviceManager9 = NULL;
	IDirect3DDevice9 *pD3DDevice = NULL;
    HANDLE hD3DDeviceHandle = 0;
	IDirect3DSwapChain9 *pSwapChain = NULL;
	D3DPRESENT_PARAMETERS PP = {0};
	HWND hWnd = GetConsoleHwnd();
	C2DBufferLock* p2DBufferLock = NULL;
	//D3DPresentEngine* pPresenterEngine = NULL;
//	VideoSampleList videoSampleQueue;

	CHECK_HR(hr = MFStartup(MF_VERSION, 0));

//	pPresenterEngine = new D3DPresentEngine(hr);
	//CHECK_HR(hr);

	

	CHECK_HR(hr = MFFrameRateToAverageTimePerFrame(VIDEO_FPS, 1, &rtDuration));

	CHECK_HR(hr = MFCreateMediaType(&pMediaType));
	CHECK_HR(hr = pMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
	CHECK_HR(hr = pMediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32));
    CHECK_HR(hr = pMediaType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive));
	CHECK_HR(hr = pMediaType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE));
    CHECK_HR(hr = MFSetAttributeSize(pMediaType, MF_MT_FRAME_SIZE, VIDEO_WIDTH, VIDEO_HEIGHT));       
    CHECK_HR(hr = MFSetAttributeRatio(pMediaType, MF_MT_FRAME_RATE, VIDEO_FPS, 1));
    CHECK_HR(hr = MFSetAttributeRatio(pMediaType, MF_MT_PIXEL_ASPECT_RATIO, 1, 1));

	//CHECK_HR(hr = pPresenterEngine->SetVideoWindow(hWnd));
	//CHECK_HR(hr = pPresenterEngine->CreateVideoSamples(pMediaType, videoSampleQueue));

	CHECK_HR(hr = MFCreateVideoRenderer(__uuidof(IMFMediaSink), (void**)&pMediaSink));
	CHECK_HR(hr = pMediaSink->GetCharacteristics(&dwCharacteristics));

	//MFCreateVideoSampleFromSurface: http://msdn.microsoft.com/en-us/library/ms703859(v=vs.85).aspx

	CHECK_HR(hr = pMediaSink->QueryInterface(__uuidof(IMFGetService), (void**)&pService));
	CHECK_HR(hr = pService->GetService(MR_VIDEO_RENDER_SERVICE, __uuidof(IMFVideoDisplayControl), (void**)&pDisplayControl));
	CHECK_HR(hr = pService->GetService(MR_VIDEO_ACCELERATION_SERVICE, __uuidof(IDirect3DDeviceManager9), (void**)&pD3DDeviceManager9));
	CHECK_HR(hr = pDisplayControl->SetVideoWindow(hWnd));

	if((dwCharacteristics & MEDIASINK_CLOCK_REQUIRED) == MEDIASINK_CLOCK_REQUIRED)
	{
		CHECK_HR(hr = MFCreatePresentationClock(&pClock));		
		CHECK_HR(hr = MFCreateSystemTimeSource(&pTimeSource));
		CHECK_HR(hr = pClock->SetTimeSource(pTimeSource));
		CHECK_HR(hr = pMediaSink->SetPresentationClock(pClock));
	}

	CHECK_HR(hr = pMediaSink->GetStreamSinkByIndex(0, &pStreamSink));
	CHECK_HR(hr = pStreamSink->GetMediaTypeHandler(&pMediaTypeHandler));
	CHECK_HR(hr = pMediaTypeHandler->SetCurrentMediaType(pMediaType));

	CHECK_HR(hr = LockDevice(pD3DDeviceManager9, TRUE, &pD3DDevice, &hD3DDeviceHandle));
	CHECK_HR(hr = GetSwapChainPresentParameters(pD3DDevice, pMediaType, hWnd, &PP));
	CHECK_HR(hr = pD3DDevice->CreateAdditionalSwapChain(&PP, &pSwapChain));
	CHECK_HR(hr = CreateD3DSample(pSwapChain, pD3DDevice, &p2DBufferLock));

	
	//CHECK_HR(hr = CreateMediaSample(VIDEO_BUFFER_SIZE, pSampleAllocator, &pSample));
	CHECK_HR(hr = p2DBufferLock->SetSampleDuration(rtDuration));
	CHECK_HR(hr = pD3DDeviceManager9->UnlockDevice(hD3DDeviceHandle, TRUE));

	if(pClock)
	{
		CHECK_HR(hr = pClock->Start(rtStart));
	}

	pStreamSink->Flush();

	for(INT i = 0; i < 500 ; ++i)
	{

		BYTE* pbScanLine0 = NULL;
        LONG  lStride = 0;
		//CHECK_HR(hr = pSample->GetBufferByIndex(0, &pMediaBuffer));
		CHECK_HR(hr = p2DBufferLock->LockBuffer(VIDEO_WIDTH, VIDEO_HEIGHT ,&pbScanLine0, &lStride));
		CHECK_HR(hr = p2DBufferLock->SetSampleTime(rtStart));
		for(DWORD j = 0; j < VIDEO_BUFFER_SIZE; ++j)
		{
			pbScanLine0[j] = (rand() % 0xFF);
		}
		CHECK_HR(hr = p2DBufferLock->UnlockBuffer());
		CHECK_HR(hr = pStreamSink->ProcessSample(p2DBufferLock->GetD3Sample()));
		rtStart += rtDuration;

		Sleep(1);
	}

	
bail:

	if(pClock)
	{
		pClock->Stop();
	}
	if(pMediaSink)
	{
		pMediaSink->Shutdown();
	}

	if(pD3DDeviceManager9 && hD3DDeviceHandle)
	{
		pD3DDeviceManager9->CloseDeviceHandle(hD3DDeviceHandle);
		hD3DDeviceHandle = 0;
	}

	if(p2DBufferLock)
	{
		delete p2DBufferLock;
		p2DBufferLock = NULL;
	}

	SafeRelease(&pMediaSink);
	SafeRelease(&pMediaType);
	SafeRelease(&pDisplayControl);
	SafeRelease(&pStreamSink);
	SafeRelease(&pMediaTypeHandler);
	SafeRelease(&pService);
	SafeRelease(&pClock);
	SafeRelease(&pTimeSource);
	SafeRelease(&pSampleAllocator);
	SafeRelease(&pD3DDevice);
	SafeRelease(&pD3DDeviceManager9);
	SafeRelease(&pSwapChain);

/*	if(pPresenterEngine)
	{
		delete pPresenterEngine;
		pPresenterEngine = NULL;
	}*/
}
