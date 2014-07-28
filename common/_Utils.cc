/*
* Copyright (C) 2012 Doubango Telecom <http://www.doubango.org>
*	
* This file is part of Open Source webrtc4all project <http://code.google.com/p/webrtc4all/>
*
* webrtc4all is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as publishd by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*	
* webrtc4all is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*	
* You should have received a copy of the GNU General Public License
* along with webrtc4ie.
*/
#include "_Utils.h"
#include "_PeerConnection.h"
#include "_NetTransport.h"
#if !W4A_IE
#include "../npapi/PeerConnection.h"
#include "../npapi/NetTransport.h"
#endif
#if W4A_UNDER_APPLE
#include <dispatch/dispatch.h>
#endif

#if W4A_UNDER_WINDOWS
#include <OleCtl.h>
#include <Shlwapi.h>
#endif

#include "tsk_base64.h"

static bool g_bInitialized = false;
static bool g_bHasDebugConsole = false;
/*static*/ char* g_sNullTerminated = NULL;
/*static*/ unsigned g_iNullTerminated = 0;
static signed g_nEchoTail = 100;
#if W4A_UNDER_WINDOWS
CRITICAL_SECTION g_CS;
static const HMODULE GetCurrentModule();
static BOOL CALLBACK _UtilsEnumWindowsProc( __in  HWND hWnd, __in  LPARAM lParam);
#endif

_Utils::_Utils()
{
	
}

_Utils::~_Utils()
{
	
}

void _Utils::Initialize(void)
{
	if (!g_bInitialized) {

#if 0
		StartDebug();
#endif

		int iRet;
		if ((iRet = tnet_startup()) != 0) {
			TSK_DEBUG_ERROR("tnet_startup failed with error code=%d", iRet);
			return;
		}
		if (tdav_init() == 0) {
			g_bInitialized = TRUE;
			TSK_DEBUG_INFO("Library succeesfully initilized");
		}
		else {
			TSK_DEBUG_ERROR("Failed to initialize");
		}
#if W4A_UNDER_WINDOWS
		InitializeCriticalSection(&g_CS);
#endif
    
#if W4A_UNDER_MAC
        extern const tmedia_producer_plugin_def_t *w4a_producer_video_qt_plugin_def_t;
        extern const tmedia_producer_plugin_def_t *w4a_producer_screencast_osx_plugin_def_t;
        extern const tmedia_consumer_plugin_def_t *w4a_consumer_video_osx_plugin_def_t;
        tmedia_producer_plugin_register(w4a_producer_video_qt_plugin_def_t);
        tmedia_producer_plugin_register(w4a_producer_screencast_osx_plugin_def_t);
        tmedia_consumer_plugin_register(w4a_consumer_video_osx_plugin_def_t);
#endif
		
#if METROPOLIS /* G2J.COM TelePresence client */
		static tmedia_srtp_mode_t g_eSrtpMode = tmedia_srtp_mode_optional; // "TANDBERG/4120 (X7.2.2.2) returns SAVPF without crypto lines"
		static tmedia_srtp_type_t g_eSrtpType = tmedia_srtp_type_sdes;
		static tsk_bool_t g_bIceEnabled = tsk_true;
		static tsk_bool_t g_bZeroartifactsEnabled = tsk_true;
		static tmedia_mode_t g_eAvpfMode = tmedia_mode_optional;
		// See issue 382: https://code.google.com/p/doubango/issues/detail?id=382
		static int g_nMotionRank = 1; // low(1),medium(2),high(4)
		static int g_nFps = 25;
		static tmedia_pref_video_size_t g_ePrefVideoSize = tmedia_pref_video_size_720p;
#else
		static tmedia_srtp_mode_t g_eSrtpMode = tmedia_srtp_mode_optional;
		static tmedia_srtp_type_t g_eSrtpType = (tmedia_srtp_type_t)(tmedia_srtp_type_sdes | tmedia_srtp_type_dtls);
		static tsk_bool_t g_bIceEnabled = tsk_true;
		static tsk_bool_t g_bZeroartifactsEnabled = tsk_true;
		static tmedia_mode_t g_eAvpfMode = tmedia_mode_optional;
		static int g_nMotionRank = 2; // low(1),medium(2),high(4)
		static int g_nFps = 15;
		static tmedia_pref_video_size_t g_ePrefVideoSize = tmedia_pref_video_size_vga;
#endif

		// Disable AMR, G.729, H.261 codecs
		tdav_set_codecs((tdav_codec_id_t)(
				tdav_codec_id_gsm |
				tdav_codec_id_pcma |
				tdav_codec_id_pcmu |
				/*tdav_codec_id_opus |*/ // FIXME: direct call to chrome sometimes produce noise. Why?
				tdav_codec_id_ilbc |
				tdav_codec_id_speex_nb |
				tdav_codec_id_speex_wb |
				tdav_codec_id_speex_uwb |
#if !METROPOLIS
				tdav_codec_id_g722 | // robotic voice on Polycom TelePresence (701021) - looks like regression...to be checked
				tdav_codec_id_h263 |
				tdav_codec_id_h263p |
				tdav_codec_id_h263pp |
#endif
				tdav_codec_id_h264_bp |
				tdav_codec_id_h264_mp |
				tdav_codec_id_h264_hp |
#if !METROPOLIS
				tdav_codec_id_theora |
				tdav_codec_id_mp4ves_es |
#endif
				tdav_codec_id_vp8
		));

		// Priority: VP8, H.264 (BP then MP), PCMA/PCMU, OPUS...
		int prio = 0;
		tdav_codec_set_priority(tdav_codec_id_vp8, prio++);
		tdav_codec_set_priority(tdav_codec_id_h264_bp, prio++);
		tdav_codec_set_priority(tdav_codec_id_h264_mp, prio++);
		tdav_codec_set_priority(tdav_codec_id_pcma, prio++);
		tdav_codec_set_priority(tdav_codec_id_pcmu, prio++);
		tdav_codec_set_priority(tdav_codec_id_opus, prio++);

#if 0 /* RTCWeb optional to allow interop with other SIP implementations */
		tmedia_defaults_set_profile(tmedia_profile_rtcweb);
#else
		tmedia_defaults_set_ice_enabled(g_bIceEnabled); // Use ICE only if supported by remote party
		tmedia_defaults_set_srtp_mode(g_eSrtpMode); // Use SRTP only if supported by remote party
		tmedia_defaults_set_srtp_type(g_eSrtpType); // Negotiate the type of SRTP to use (SDES or DTLS)
#endif
		tmedia_defaults_set_rtcp_enabled(tsk_true);
		tmedia_defaults_set_rtcpmux_enabled(tsk_true);

		tmedia_defaults_set_avpf_mode(g_eAvpfMode);

		tmedia_defaults_set_echo_supp_enabled(tsk_true);
		tmedia_defaults_set_echo_tail(g_nEchoTail);
		tmedia_defaults_set_echo_skew(0);
		tmedia_defaults_set_agc_enabled(tsk_true);
		tmedia_defaults_set_vad_enabled(tsk_false);
		tmedia_defaults_set_noise_supp_enabled(tsk_true);
		tmedia_defaults_set_jb_margin(0);
		tmedia_defaults_set_jb_max_late_rate(1);

		tmedia_defaults_set_video_fps(g_nFps);
		tmedia_defaults_set_video_zeroartifacts_enabled(g_bZeroartifactsEnabled);
		tmedia_defaults_set_pref_video_size(g_ePrefVideoSize);
		
		tmedia_defaults_set_opus_maxcapturerate(16000); /* Because of WebRTC AEC: only 8000 and 16000 are supported */
		tmedia_defaults_set_opus_maxplaybackrate(48000);

		tmedia_defaults_set_video_motion_rank(g_nMotionRank);
		tmedia_defaults_set_bandwidth_video_download_max(-1);
		tmedia_defaults_set_bandwidth_video_upload_max(-1);
	}
}

bool _Utils::StartDebug(void)
{
#if W4A_UNDER_WINDOWS
	if (AllocConsole()){
		freopen("CONIN$", "r", stdin); 
		freopen("CONOUT$", "w", stdout); 
		freopen("CONOUT$", "w", stderr);
		SetConsoleTitleA("WebRTC extension for Safari, Opera, FireFox and IE");
		return TRUE;
	}
#endif
	return false;
}

bool _Utils::StopDebug(void)
{
#if W4A_UNDER_WINDOWS
	return (FreeConsole() == TRUE);
#else
    return false;
#endif
}

const char* _Utils::GetCurrentDirectoryPath()
{
#if W4A_UNDER_WINDOWS
	static char CURRENT_DIR_PATH[MAX_PATH] = { 0 };
	static DWORD CURRENT_DIR_PATH_LEN = 0;
	if(CURRENT_DIR_PATH_LEN == 0) {
		// NULL HMODULE will get the path to the executable not the DLL. When runing the code in Internet Explorer this is a BIG issue as the path is where IE.exe is installed.
		if((CURRENT_DIR_PATH_LEN = GetModuleFileNameA(GetCurrentModule(), CURRENT_DIR_PATH, MAX_PATH))) {
			if(!PathRemoveFileSpecA(CURRENT_DIR_PATH)) {
				TSK_DEBUG_ERROR("PathRemoveFileSpecA(%s) failed: %x", CURRENT_DIR_PATH, GetLastError());
				memset(CURRENT_DIR_PATH, 0, MAX_PATH);
			}
		}
		else {
			TSK_DEBUG_ERROR("GetModuleFileNameA() failed: %x", GetLastError());
		}
	}
	return CURRENT_DIR_PATH;
#else
    return ".";
#endif
}

class _NPAsyncData {
public:
    _NPAsyncData(HWND _hWnd, UINT _uMsg, WPARAM _wParam, LPARAM _lParam)
    : hWnd(_hWnd)
    , uMsg(_uMsg)
    , wParam(_wParam)
    , lParam(_lParam)
    {}
    HWND hWnd;
    UINT uMsg;
    WPARAM wParam;
    LPARAM lParam;
};
static void _NPWndProc(void *npdata)
{
    _NPAsyncData* _npdata = dynamic_cast<_NPAsyncData*>((_NPAsyncData*)npdata);
    _Utils::WndProc(_npdata->hWnd, _npdata->uMsg, _npdata->wParam, _npdata->lParam);
    delete _npdata;
}

bool _Utils::PostMessage(LONGLONG handle, unsigned msg, void* wParam, void** lParam)
{
    HWND hWnd = reinterpret_cast<HWND>(handle);
    LPARAM _lParam = lParam ? reinterpret_cast<LPARAM>(*lParam) : (LPARAM)NULL;
    bool ret = false;
#if W4A_UNDER_WINDOWS
    ret = (PostMessageA(hWnd, msg, reinterpret_cast<WPARAM>(wParam), _lParam) == TRUE);
#else
    _NPAsyncData* _npdata = new _NPAsyncData(hWnd, msg, reinterpret_cast<WPARAM>(wParam), _lParam);
    if (_npdata) {
#if 0 // NPN_PluginThreadAsyncCall doesn't work on OSX
        NPP npplugin = NULL;
        switch(msg){
            case WM_ICE_EVENT_CANDIDATE:
            case WM_ICE_EVENT_CONNECTED:
            case WM_ICE_EVENT_FAILED:
            case WM_ICE_EVENT_CANCELLED:
            case WM_RFC5168_EVENT:
			case WM_BFCP_EVENT:
            default:
            {
                PeerConnection* pc = reinterpret_cast<PeerConnection*>(wParam);
                npplugin = pc->GetInstance();
            }
            case WM_NET_EVENT:
            {
                NetTransport* nt = reinterpret_cast<NetTransport*>(wParam);
                npplugin = nt->GetInstance();
            }
                break;
        }
        extern NPNetscapeFuncs* BrowserFuncs;
        BrowserFuncs->pluginthreadasynccall(npplugin, _NPWndProc, _npdata);
#else
        dispatch_async(dispatch_get_main_queue(), ^{
            _NPWndProc(_npdata);
        });
#endif
        ret = true;
    }

#endif
    if (ret) {
        if (msg != WM_RFC5168_EVENT) {
            if (lParam) {
                *lParam = NULL;
            }
        }
    }
    return ret;
}

LRESULT CALLBACK _Utils::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg){
#if 0
		case WM_PAINT:
			{
				PAINTSTRUCT ps;
				HDC hdc = BeginPaint(hWnd, &ps);

				SetBkMode(hdc, TRANSPARENT);
				TextOut(hdc, 50, 50, L"Fake text", lstrlen(L"Fake text"));

				EndPaint(hWnd, &ps);
				break;
			}
#endif
		case WM_ICE_EVENT_CANDIDATE:
			{
				TSK_DEBUG_INFO("_Utils::WndProc::WM_ICE_EVENT");
				_PeerConnection* This = reinterpret_cast<_PeerConnection*>(wParam);
				PeerConnectionEvent* oEvent = reinterpret_cast<PeerConnectionEvent*>(lParam);
				if(oEvent){
					This->IceCallbackFire(oEvent);
					delete oEvent;
				}
				break;
			}
		case WM_ICE_EVENT_CONNECTED: /* Both parties support ICE and ConnCheck succeed */
		case WM_ICE_EVENT_FAILED: /* Both parties support ICE but ConnCheck failed: Use Address in SDP "c=" line */
		case WM_ICE_EVENT_CANCELLED: /* Remote paty doesn't support: Use Address in SDP "c=" line */
			{
				const char* pcMsg = uMsg == WM_ICE_EVENT_CONNECTED ? "WM_ICE_EVENT_CONNECTED"
				: uMsg == WM_ICE_EVENT_FAILED ? "WM_ICE_EVENT_FAILED"
				: uMsg == WM_ICE_EVENT_CANCELLED ? "WM_ICE_EVENT_CANCELLED"
				: "WM_ICE_EVENT_UNKNOWN";
				TSK_DEBUG_INFO("_Utils::WndProc::%s", pcMsg);
				_PeerConnection* This = reinterpret_cast<_PeerConnection*>(wParam);
				This->StartMedia(); // Start() must not be called on worker thread
				break;
			}
		case WM_NET_EVENT:
			{
				TSK_DEBUG_INFO("_Utils::WndProc::WM_NET_EVENT");
				_NetTransport* This = reinterpret_cast<_NetTransport*>(wParam);
				NetTransportEvent* oEvent = reinterpret_cast<NetTransportEvent*>(lParam);
				if(oEvent){
					This->DgramCbFire(oEvent);
					delete oEvent;
				}
				break;
			}
            
		case WM_RFC5168_EVENT:
			{
				TSK_DEBUG_INFO("_Utils::WndProc::WM_RFC5168_EVENT");
				_PeerConnection* This = reinterpret_cast<_PeerConnection*>(wParam);
				const char* sCommand = reinterpret_cast<const char*>(lParam);
				if (sCommand) {
					This->Rfc5168CallbackFire(sCommand);
					// must not free (it's a static string)
				}
				break;
			}
		case WM_BFCP_EVENT:
			{
				_PeerConnection* This = reinterpret_cast<_PeerConnection*>(wParam);
				char* npDescription = reinterpret_cast<char*>(lParam);
				TSK_DEBUG_INFO("_Utils::WndProc::WM_BFCP_EVENT: %s", npDescription);
				if (npDescription) {
					This->BfcpCallbackFire(npDescription);
					_Utils::MemFree((void**)&npDescription);
				}
			}
	}
#if W4A_UNDER_WINDOWS
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
#else
    return 0;
#endif
}

void* _Utils::MemAlloc(unsigned n)
{
	extern void* _UtilsMemAlloc(unsigned n);
	return _UtilsMemAlloc(n);
}

void* _Utils::MemDup(const void* mem, unsigned n)
{
    void *ret = NULL;
	if(mem && n){
		if((ret = _Utils::MemAlloc((n + 1)))){
			memcpy(ret, mem, n);
			((uint8_t*)ret)[n] = '\0';
		}
	}
    return ret;
}
#if 0
void* _Utils::MemReAlloc(void* mem, unsigned n)
{
	extern void* _UtilsMemReAlloc(void* mem, unsigned n);
	return _UtilsMemReAlloc(mem, n);
}
#endif

void _Utils::MemFree(void** mem)
{
	extern void _UtilsMemFree(void** mem);
    return _UtilsMemFree(mem);
}

_ActiveApps* _Utils::GetActiveApps()
{
	_ActiveApps* activeApps = new _ActiveApps();
	if (!activeApps) {
		TSK_DEBUG_ERROR("Failed to allocate '_ActiveApps' instance");
		return NULL;
	}
#if W4A_UNDER_WINDOWS
	if (!::EnumWindows( _UtilsEnumWindowsProc, reinterpret_cast<LPARAM>(activeApps) )) {
		TSK_DEBUG_ERROR("EnumWindows failed with error code = %x", GetLastError());
	}
#elif W4A_UNDER_MAC
#endif
	
	return activeApps;
}

#if W4A_UNDER_WINDOWS

static const HMODULE GetCurrentModule()
{
	HMODULE hm = {0};
    GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCTSTR)GetCurrentModule, &hm);   
    return hm;
}

// http://blogs.msdn.com/b/oldnewthing/archive/2007/10/08/5351207.aspx
static BOOL _UtilsIsAltTabWindow(HWND hwnd)
{
	TITLEBARINFO ti = { 0 };
    HWND hwndTry, hwndWalk = NULL;

	if (!IsWindowVisible(hwnd)) {
        return FALSE;
	}

    hwndTry = GetAncestor(hwnd, GA_ROOTOWNER);
    while (hwndTry != hwndWalk) {
        hwndWalk = hwndTry;
        hwndTry = GetLastActivePopup(hwndWalk);
		if (IsWindowVisible(hwndTry)) {
            break;
		}
    }
	if (hwndWalk != hwnd) {
        return FALSE;
	}

    if (GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW) {
        return FALSE;
	}

    ti.cbSize = sizeof(ti);
    GetTitleBarInfo(hwnd, &ti);
	if (ti.rgstate[0] & STATE_SYSTEM_INVISIBLE) {
        return FALSE;
	}

    return TRUE;
}

// http://msdn.microsoft.com/en-us/library/ms997538.aspx
#pragma pack( push )
#pragma pack( 2 )
typedef struct
{
	BYTE	bWidth;               // Width of the image
	BYTE	bHeight;              // Height of the image (times 2)
	BYTE	bColorCount;          // Number of colors in image (0 if >=8bpp)
	BYTE	bReserved;            // Reserved
	WORD	wPlanes;              // Color Planes
	WORD	wBitCount;            // Bits per pixel
	DWORD	dwBytesInRes;         // how many bytes in this resource?
	DWORD	dwImageOffset;        // where in the file is this image
} ICONDIRENTRY, *LPICONDIRENTRY;
typedef struct 
{
	WORD			idReserved;   // Reserved
	WORD			idType;       // resource type (1 for icons)
	WORD			idCount;      // how many images?
} ICONDIR, *LPICONDIR;
typedef struct tagBITMAPINFO_ {
    BITMAPINFOHEADER    bmiHeader;
    RGBQUAD             bmiColors[256];
} BITMAPINFO_, FAR *LPBITMAPINFO_, *PBITMAPINFO_;
#pragma pack( pop )

static tsk_size_t _UtilsGetBase64Icon(__in HICON hIcon, HWND hWnd, __out void** bufferPtr)
{
	tsk_size_t ret = 0;
	if (!hIcon || !bufferPtr || *bufferPtr) {
		TSK_DEBUG_ERROR("Invalid parameter");
		return 0;
	}

	ICONINFO info = { 0 };
    if (!GetIconInfo(hIcon, &info)) {
        return 0;
    }
	BITMAP bmpColor = { 0 }, bmpMask = { 0 };
	byte *dataColorPtr = NULL, *dataMaskPtr = NULL;
	DWORD dataColorSize = 0, dataMaskSize = 0;
	
	if (GetObject(info.hbmColor, sizeof(bmpColor), &bmpColor) &&
		GetObject(info.hbmMask, sizeof(bmpMask), &bmpMask) &&
        (bmpColor.bmWidth && bmpColor.bmWidth == bmpMask.bmWidth) &&
        (bmpColor.bmHeight && bmpColor.bmHeight == bmpMask.bmHeight) &&
		(bmpColor.bmPlanes == bmpMask.bmPlanes)
		){
 
        BITMAPINFO_ bmi_ = { 0 };
        HDC hDC = GetDC(hWnd);
		
        bmi_.bmiHeader.biSize = sizeof(bmi_.bmiHeader);
		bmi_.bmiHeader.biCompression = BI_RGB;
        bmi_.bmiHeader.biWidth = bmpMask.bmWidth;
        bmi_.bmiHeader.biHeight = bmpMask.bmHeight;
		bmi_.bmiHeader.biPlanes = bmpMask.bmPlanes;
        bmi_.bmiHeader.biBitCount = bmpMask.bmBitsPixel;
		// bmi_.bmiHeader.biSizeImage = (bmi_.bmiHeader.biWidth * bmi_.bmiHeader.biHeight * bmi_.bmiHeader.biBitCount) >> 3; // must >> 3 after MUL because "biBitCount" could be equal to 1 ((1 >> 3) == 0)
		GetDIBits(hDC, info.hbmColor, 0, bmi_.bmiHeader.biHeight, NULL, (LPBITMAPINFO)&bmi_, DIB_RGB_COLORS);
		if ((dataMaskPtr = (byte*)_Utils::MemAlloc(bmi_.bmiHeader.biSizeImage * sizeof(byte)))) {
			if (GetDIBits(hDC, info.hbmMask, 0, bmi_.bmiHeader.biHeight, dataMaskPtr, (LPBITMAPINFO)&bmi_, DIB_RGB_COLORS)) {
				dataMaskSize = bmi_.bmiHeader.biSizeImage;
			}
		}
		bmi_.bmiHeader.biSize = sizeof(bmi_.bmiHeader);
        bmi_.bmiHeader.biWidth = bmpColor.bmWidth;
        bmi_.bmiHeader.biHeight = bmpColor.bmHeight;
		bmi_.bmiHeader.biPlanes = bmpColor.bmPlanes;
        bmi_.bmiHeader.biBitCount = bmpColor.bmBitsPixel;
		// bmi_.bmiHeader.biSizeImage = (bmi_.bmiHeader.biWidth * bmi_.bmiHeader.biHeight * bmi_.bmiHeader.biBitCount) >> 3;
		GetDIBits(hDC, info.hbmColor, 0, bmi_.bmiHeader.biHeight, NULL, (LPBITMAPINFO)&bmi_, DIB_RGB_COLORS);
		if ((dataColorPtr = (byte*)_Utils::MemAlloc(bmi_.bmiHeader.biSizeImage * sizeof(byte)))) {
			if (GetDIBits(hDC, info.hbmColor, 0, bmi_.bmiHeader.biHeight, dataColorPtr, (LPBITMAPINFO)&bmi_, DIB_RGB_COLORS)) {
				dataColorSize = bmi_.bmiHeader.biSizeImage;
			}
		}
 
		ICONDIR dir = 
		{ 
			0, // idReserved
			1, // idType
			1 // idCount
		};
		ICONDIRENTRY direntry = 
		{ 
			(BYTE)bmi_.bmiHeader.biWidth, //	bWidth;
			(BYTE)bmi_.bmiHeader.biHeight, //	bHeight;
			(BYTE)(((bmi_.bmiHeader.biPlanes * bmpColor.bmBitsPixel) >= 8) ?  0 : (1 << (bmi_.bmiHeader.biPlanes * bmpColor.bmBitsPixel))), //	bColorCount;
			(BYTE)0, //	bReserved;
			bmi_.bmiHeader.biPlanes, //	wPlanes;
			bmpColor.bmBitsPixel, //	wBitCount;
			sizeof(BITMAPINFOHEADER) + (dataColorSize +  dataMaskSize),//	dwBytesInRes;
			sizeof(ICONDIR) + sizeof(ICONDIRENTRY) //	dwImageOffset;
		};

		bmi_.bmiHeader.biSize = sizeof(bmi_.bmiHeader);
        bmi_.bmiHeader.biWidth = bmpColor.bmWidth;
        bmi_.bmiHeader.biHeight = bmpColor.bmHeight + bmpMask.bmHeight;
        bmi_.bmiHeader.biPlanes = bmi_.bmiHeader.biPlanes;
        bmi_.bmiHeader.biBitCount = bmpColor.bmBitsPixel;
        bmi_.bmiHeader.biSizeImage = 0;

		if(dataColorPtr && dataColorSize && dataMaskPtr && dataMaskSize) {
			size_t size = sizeof(dir) + sizeof(direntry) + sizeof(bmi_.bmiHeader) + dataColorSize + dataMaskSize;
			uint8_t *ptr = (uint8_t*)tsk_malloc(size), *_ptr;
			if ((_ptr = ptr)) {
				memcpy(_ptr, &dir, sizeof(dir)), _ptr += sizeof(dir);
				memcpy(_ptr, &direntry, sizeof(direntry)), _ptr += sizeof(direntry);
				memcpy(_ptr, &bmi_.bmiHeader, sizeof(bmi_.bmiHeader)), _ptr += sizeof(bmi_.bmiHeader);
				memcpy(_ptr, dataColorPtr, dataColorSize), _ptr += dataColorSize;
				memcpy(_ptr, dataMaskPtr, dataMaskSize);
				ret = tsk_base64_encode(ptr, (tsk_size_t)size, (char**)bufferPtr);
				TSK_FREE(ptr);
			}
		}
		
		if (hDC) {
			ReleaseDC(hWnd, hDC);
		}
		
		_Utils::MemFree((void**)&dataColorPtr);
		_Utils::MemFree((void**)&dataMaskPtr);
    }
	else {
		TSK_DEBUG_ERROR("Unexpected code called");
	}

	if(info.hbmColor) {
		DeleteObject(info.hbmColor);
	}
	if(info.hbmMask) {
		DeleteObject(info.hbmMask);
	}

	return ret;
}

static BOOL CALLBACK _UtilsEnumWindowsProc( __in  HWND hWnd, __in  LPARAM lParam) 
{
	if (!::_UtilsIsAltTabWindow(hWnd)/*!::IsIconic(hWnd) || !::IsWindowVisible(hWnd)*/) {
		return TRUE;
	}

	int length;
	HICON hIcon = NULL;
	TCHAR* tmpBuffWindowText = NULL;
	IPicture* pPicture = NULL;
	void* base64dataPtr = NULL;
	HRESULT hr = S_OK;

	length = ::GetWindowTextLength(hWnd);
	if (length <= 0) {
		return TRUE;
	}

	_ActiveApps* activeApps = reinterpret_cast<_ActiveApps*>(lParam);

	tmpBuffWindowText = (TCHAR*)_Utils::MemAlloc((length + 1) * sizeof(TCHAR));
	if (!tmpBuffWindowText) {
		TSK_DEBUG_ERROR("Failed to allocate buffer with size = %d", (length + 1) * sizeof(TCHAR));
		return TRUE;
	}
	GetWindowText(hWnd, tmpBuffWindowText, length + 1);

	hIcon = (HICON)SendMessage(hWnd, WM_GETICON, ICON_BIG, NULL); 
	if (hIcon == NULL) {
		hIcon = (HICON)SendMessage(hWnd, WM_GETICON, ICON_SMALL2, NULL);
	}
	if (hIcon == NULL) {
		hIcon = (HICON)SendMessage(hWnd, WM_GETICON, ICON_SMALL, NULL);
	}
	if (hIcon == NULL) {
		hIcon = (HICON)GetClassLong(hWnd, GCL_HICON); 
	}
	if (hIcon == NULL) {
		hIcon = (HICON)GetClassLong(hWnd, GCL_HICONSM); 
	}
	if (hIcon == NULL) {
		hIcon = LoadIcon(NULL, IDI_APPLICATION);
	}

	tsk_size_t base64dataSize = _UtilsGetBase64Icon(hIcon, hWnd, &base64dataPtr);
	if (base64dataSize && base64dataPtr) {
		static const char __IconTypePtr[] = "image/x-icon;base64";
		static size_t __IconTypeSize = sizeof(__IconTypePtr) - 1;
		char windowTextPtr[MAX_PATH];
		size_t windowTextSize = wcstombs(windowTextPtr, tmpBuffWindowText, sizeof(windowTextPtr));
		activeApps->AppendApp(
			reinterpret_cast<LONGLONG>(hWnd),
			windowTextPtr, windowTextSize,
			base64dataPtr, (size_t)base64dataSize,
			__IconTypePtr, __IconTypeSize);
	}
	
	
	_Utils::MemFree((void**)&tmpBuffWindowText);
	TSK_FREE(base64dataPtr);
	return TRUE;
}

#endif /* W4A_UNDER_WINDOWS */

_ActiveApps::_ActiveApps()
: m_pDataPtr(NULL)
, m_nDataSize(0)
{
	
}

_ActiveApps::~_ActiveApps()
{
	TSK_FREE(m_pDataPtr);
}

// Format: base64($$WindowID$$=...$$Description$$=...$$IconData$$=...$$IconType$$=...)
bool _ActiveApps::AppendApp(LONGLONG windowID, const void* descPtr, size_t descSize, const void* iconDataPtr, size_t iconDataSize, const void* iconTypePtr, size_t iconTypeSize)
{
	static const char __kWindowIDPtr[] = "$$WindowID$$=";
	static size_t __kWindowIDSize = sizeof(__kWindowIDPtr) - 1;
	static const char __kDescriptionPtr[] = "$$Description$$=";
	static size_t __kDescriptionSize = sizeof(__kDescriptionPtr) - 1;
	static const char __kIconDataPtr[] = "$$IconData$$=";
	static size_t __kIconDataSize = sizeof(__kIconDataPtr) - 1;
	static const char __kIconTypePtr[] = "$$IconType$$=";
	static size_t __kIconTypeSize = sizeof(__kIconTypePtr) - 1;
	static size_t __kKeysSize = __kWindowIDSize + __kDescriptionSize + __kIconDataSize + __kIconTypeSize;

	// "IconData" is base64 data and could be displayed on HTML page like this: 
	// <img src="data:image/x-icon;base64,YOUR BASE64 DATA HERE" />

	if (!descPtr || !descSize || !iconDataPtr || !iconDataSize || !iconTypePtr || !iconTypeSize) {
		TSK_DEBUG_ERROR("Invalid parameter");
		return false;
	}

	char windowIDStr[22];
	sprintf(windowIDStr, "%lld\0", windowID);
	size_t windowIDStrLen = strlen(windowIDStr);
	size_t appSize = __kKeysSize + windowIDStrLen + descSize + iconDataSize + iconTypeSize;
	uint8_t* pAppPtr = (uint8_t*)tsk_malloc(appSize);
	if (!pAppPtr) {
		TSK_DEBUG_ERROR("Failed to alloc memory with size = %lu", appSize);
		return false;
	}
	uint8_t* _pAppPtr = pAppPtr;
	
	// WindowID
	memcpy(pAppPtr, __kWindowIDPtr, __kWindowIDSize);
	pAppPtr += __kWindowIDSize;
	memcpy(pAppPtr, windowIDStr, windowIDStrLen);
	pAppPtr += windowIDStrLen;
	// Description
	memcpy(pAppPtr, __kDescriptionPtr, __kDescriptionSize);
	pAppPtr += __kDescriptionSize;
	memcpy(pAppPtr, descPtr, descSize);
	pAppPtr += descSize;
	// IconData
	memcpy(pAppPtr, __kIconDataPtr, __kIconDataSize);
	pAppPtr += __kIconDataSize;
	memcpy(pAppPtr, iconDataPtr, iconDataSize);
	pAppPtr += iconDataSize;
	// IconType
	memcpy(pAppPtr, __kIconTypePtr, __kIconTypeSize);
	pAppPtr += __kIconTypeSize;
	memcpy(pAppPtr, iconTypePtr, iconTypeSize);
	pAppPtr += iconTypeSize;

	char* pApp64Ptr = NULL;
	size_t app64Size = tsk_base64_encode(_pAppPtr, appSize, &pApp64Ptr);
	TSK_FREE(_pAppPtr);
	if (!app64Size || !pApp64Ptr) {
		TSK_DEBUG_ERROR("failed to encode buffer");
		return false;
	}

	size_t oldSize = m_nDataSize;
	size_t newSize = oldSize + app64Size;
	if (!(m_pDataPtr = tsk_realloc(m_pDataPtr, newSize + 1))) {
		TSK_DEBUG_ERROR("Failed to alloc memory with size = %lu", newSize);
		m_nDataSize = 0;
		TSK_FREE(pApp64Ptr);
		return false;
	}
	m_nDataSize = newSize;
	memcpy(((uint8_t*)m_pDataPtr) + oldSize, pApp64Ptr, app64Size);
	((uint8_t*)m_pDataPtr)[newSize] = '\0';
	TSK_FREE(pApp64Ptr);

	return true;
}
