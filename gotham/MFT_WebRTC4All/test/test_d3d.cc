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
#define CHECK_HR(x) if (FAILED(x)) { fprintf(stdout, "Operation Failed (%d)", __LINE__); goto bail; }

const DWORD NUM_BACK_BUFFERS = 2;
const UINT32 VIDEO_WIDTH = 640;
const UINT32 VIDEO_HEIGHT = 480;
const UINT32 VIDEO_FPS = 25;
const UINT32 VIDEO_BUFFER_SIZE = (VIDEO_WIDTH * VIDEO_HEIGHT << 2);

template <class T> void SafeRelease(T **ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
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

static HRESULT CreateDeviceD3D9(HWND hWnd, IDirect3DDevice9** ppDevice, IDirect3D9 **ppD3D)
{
	HRESULT hr = S_OK;

	D3DPRESENT_PARAMETERS pp = { 0 };
    D3DDISPLAYMODE mode = { 0 };

	if(!ppDevice || *ppDevice || !ppD3D || *ppD3D)
	{
		CHECK_HR(hr = E_POINTER);
	}
    
    if(!(*ppD3D = Direct3DCreate9(D3D_SDK_VERSION)))
    {
        CHECK_HR(hr = E_OUTOFMEMORY);
    }

    CHECK_HR(hr = (*ppD3D)->GetAdapterDisplayMode(
        D3DADAPTER_DEFAULT,
        &mode
        ));

    CHECK_HR(hr = (*ppD3D)->CheckDeviceType(
        D3DADAPTER_DEFAULT,
        D3DDEVTYPE_HAL,
        mode.Format,
        D3DFMT_X8R8G8B8,
        TRUE    // windowed
        ));

    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.SwapEffect = D3DSWAPEFFECT_COPY;
    pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    pp.Windowed = TRUE;
    pp.hDeviceWindow = hWnd;

    CHECK_HR(hr = (*ppD3D)->CreateDevice(
        D3DADAPTER_DEFAULT,
        D3DDEVTYPE_HAL,
        hWnd,
        D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE,
        &pp,
        ppDevice
        ));

bail:
	if(FAILED(hr))
	{
		SafeRelease(ppD3D);
		SafeRelease(ppDevice);
	}
    return hr;
}

static HRESULT TestCooperativeLevel(IDirect3DDevice9* pDevice)
{
	HRESULT hr = S_OK;

    if (pDevice == NULL)
    {
		CHECK_HR(hr = E_POINTER);
    }
    
    switch((hr = pDevice->TestCooperativeLevel()))
    {
		case D3D_OK:
			{
				break;
			}

		case D3DERR_DEVICELOST:
			hr = S_OK; // FIXME
			break;

		case D3DERR_DEVICENOTRESET:
			{
				// hr = ResetDevice(); // FIXME
				break;
			}

		default:
			{
				break;
			}
    }
	if(FAILED(hr))
	{
		int kaka = 0;//FIXME
		++kaka;
	}

	CHECK_HR(hr);

bail:
    return hr;
}

static HRESULT CreateSwapChain(HWND hWnd, UINT32 nFrameWidth, UINT32 nFrameHeight, IDirect3DDevice9* pDevice, IDirect3DSwapChain9 **ppSwapChain)
{
    HRESULT hr = S_OK;

    D3DPRESENT_PARAMETERS pp = { 0 };

	if(!pDevice || !ppSwapChain || *ppSwapChain)
	{
		CHECK_HR(hr = E_POINTER);
	}

    pp.BackBufferWidth  = nFrameWidth;
    pp.BackBufferHeight = nFrameHeight;
    pp.Windowed = TRUE;
    pp.SwapEffect = D3DSWAPEFFECT_FLIP;
    pp.hDeviceWindow = hWnd;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.Flags =
        D3DPRESENTFLAG_VIDEO | D3DPRESENTFLAG_DEVICECLIP |
        D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
    pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    pp.BackBufferCount = NUM_BACK_BUFFERS;

    CHECK_HR(hr = pDevice->CreateAdditionalSwapChain(&pp, ppSwapChain));

bail:
    return hr;
}

void main()
{
	HRESULT hr = S_OK;
	D3DLOCKED_RECT lr = {0};
	HWND hWnd = GetConsoleHwnd();
	IDirect3DDevice9* pDevice = NULL;
	IDirect3D9 *pD3D = NULL;
	IDirect3DSwapChain9 *pSwapChain = NULL;
	IDirect3DSurface9 *pSurf = NULL;
    IDirect3DSurface9 *pBB = NULL;
	

	CHECK_HR(hr = CreateDeviceD3D9(hWnd, &pDevice, &pD3D));
	CHECK_HR(hr = CreateSwapChain(hWnd, VIDEO_WIDTH, VIDEO_HEIGHT, pDevice, &pSwapChain));

	for(INT i = 0; i < 500; ++i)
	{
		CHECK_HR(hr = TestCooperativeLevel(pDevice));
		CHECK_HR(hr = pSwapChain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &pSurf));    
		CHECK_HR(hr = pSurf->LockRect(&lr, NULL, D3DLOCK_NOSYSLOCK ));	

		// Draw()
		for(INT j = 0; j < VIDEO_BUFFER_SIZE; ++j)
		{
			((BYTE*)lr.pBits)[j] = (rand() & 0xFF);
		}

		CHECK_HR(hr = pSurf->UnlockRect());

		// Color fill the back buffer
		CHECK_HR(hr = pDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBB));
		CHECK_HR(hr = pDevice->ColorFill(pBB, NULL, D3DCOLOR_XRGB(0xFF, 0xFF, 0xFF)));
		
		// Resize keeping aspect ratio and Blit the frame (required)
		RECT rcDst = { 0, 0, 100, 100 };
		//GetClientRect(hWnd, &rcDst);
		hr = pDevice->StretchRect(pSurf, NULL, pBB, &rcDst, D3DTEXF_LINEAR); // could fail when display is being resized
		if(SUCCEEDED(hr))
		{
			// Present the frame
			CHECK_HR(hr = pDevice->Present(NULL, NULL, NULL, NULL));
		}

		Sleep(10);
	}

bail:
	SafeRelease(&pDevice);
	SafeRelease(&pD3D);
	SafeRelease(&pSwapChain);
	SafeRelease(&pSurf);
	SafeRelease(&pBB);
}