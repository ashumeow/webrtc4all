/* Copyright (C) 2013 Mamadou DIOP
* Copyright (C) 2013 Doubango Telecom <http://www.doubango.org>
*	
* This file is part of Open Source Doubango Framework.
*
* DOUBANGO is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*	
* DOUBANGO is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*	
* You should have received a copy of the GNU General Public License
* along with DOUBANGO.
*/

/*
This is an RTP streamer test application with encoding and decoding modules.
For now, the video stream will be displayed on the console itself which means you'll not be able to see the logs. To display the logs for decoding, just change GetConsoleHwnd() to return NULL.
There are two modes: LOOPBACK and REMOTE_STREAMING
*/

#include "stdafx.h"

#include "tinydav.h"

#include <assert.h>

#define RUN_TEST_RTP				1
#define RUN_TEST_RTP_AS_CLIENT		1

#define DEFAULT_CODECS						(tmedia_codec_id_h264_mp | tmedia_codec_id_h264_bp | tmedia_codec_id_vp8 | tmedia_codec_id_pcma | tmedia_codec_id_pcmu) // use "tmedia_codec_id_all" to enable all codecs
#define DEFAULT_VIDEO_SIZE					tmedia_pref_video_size_vga
#define DEFAULT_VIDEO_FPS					30 // up to 120
#define DEFAULT_VIDEO_REMOTE_WINDOW_NAME	L"Remote video window (Decoded RTP)" // Remote window is where the decoded video frames are displayed
#define DEFAULT_VIDEO_LOCAL_WINDOW_NAME		L"Local video window (Preview)" // Local window is where the encoded video frames are displayed before sending (preview, PIP mode).

// #if NDEBUG, assert(expression) won't execute expression
#if defined(NDEBUG)
#	define GOTHAM_ASSERT(x) (void)(x)
#else
#	define GOTHAM_ASSERT(x)	assert(x)
#endif

#if TMEDIA_UNDER_WINDOWS

static HWND __hWndLocal = NULL;
static HWND __hWndRemote = NULL;

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

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
		case WM_ERASEBKGND:
			return TRUE;
		case WM_CLOSE:
			exit(0);
			return TRUE;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

static HWND GetHwnd(BOOL bRemote)
{
	HWND* pHWND = bRemote ? &__hWndRemote : &__hWndLocal;
	if(!*pHWND)
	{
		WNDCLASS wc = {0};

		wc.lpfnWndProc   = WindowProc;
		wc.hInstance     = GetModuleHandle(NULL);
		wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
		wc.lpszClassName =  L"MFCapture Window Class";
		/*GOTHAM_ASSERT*/(RegisterClass(&wc));
		
		GOTHAM_ASSERT(*pHWND = ::CreateWindow(
			wc.lpszClassName, 
			bRemote ? DEFAULT_VIDEO_REMOTE_WINDOW_NAME : DEFAULT_VIDEO_LOCAL_WINDOW_NAME, 
			WS_OVERLAPPEDWINDOW, 
			CW_USEDEFAULT, 
			CW_USEDEFAULT, 
			bRemote ? CW_USEDEFAULT : 352, 
			bRemote ? CW_USEDEFAULT : 288, 
			NULL, 
			NULL, 
			GetModuleHandle(NULL),
			NULL));
		::SetWindowText(*pHWND, bRemote ? DEFAULT_VIDEO_REMOTE_WINDOW_NAME : DEFAULT_VIDEO_LOCAL_WINDOW_NAME);
		::ShowWindow(*pHWND, SW_SHOWDEFAULT);
		::UpdateWindow(*pHWND);
	}
	return *pHWND;
}

static HWND GetLocalHwnd(void)
{
	return GetHwnd(FALSE);
}

static HWND GetRemoteHwnd(void)
{
	return GetHwnd(TRUE);
}

#endif /* TMEDIA_UNDER_WINDOWS */

static void WaitUntilDone()
{
#if TMEDIA_UNDER_WINDOWS
	fprintf(stderr, "\n**** CLOSE THE CONSOLE TO STOP THE APP ****\n");
	MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
#else
	fprintf(stderr, "\n**** PRESS ENTER TO STOP THE APP ****\n");
	getchar();
#endif
}

#if RUN_TEST_RTP
#include "test_rtp.h"
#endif /* RUN_TEST_RTP */

int _tmain(int argc, _TCHAR* argv[])
{
	int ret = 0;

    GOTHAM_ASSERT((ret = tnet_startup()) == 0);
    GOTHAM_ASSERT((ret = tdav_init()) == 0);

	GOTHAM_ASSERT((ret = tmedia_defaults_set_pref_video_size(DEFAULT_VIDEO_SIZE)) == 0);
	GOTHAM_ASSERT((ret = tmedia_defaults_set_video_fps(DEFAULT_VIDEO_FPS)) == 0);

	GOTHAM_ASSERT((ret = tmedia_defaults_set_agc_enabled(tsk_true)) == 0);
	GOTHAM_ASSERT((ret = tmedia_defaults_set_echo_supp_enabled(tsk_true)) == 0);
	GOTHAM_ASSERT((ret = tmedia_defaults_set_echo_tail(100/*milliseconds*/)) == 0);
	GOTHAM_ASSERT((ret = tmedia_defaults_set_noise_supp_enabled(tsk_true)) == 0);

	GOTHAM_ASSERT((ret = tdav_set_codecs((tdav_codec_id_t)DEFAULT_CODECS)) == 0);
	GOTHAM_ASSERT((ret = tdav_codec_set_priority((tdav_codec_id_t)tmedia_codec_id_h264_mp, 0)) == 0);
	GOTHAM_ASSERT((ret = tdav_codec_set_priority((tdav_codec_id_t)tmedia_codec_id_h264_bp, 1)) == 0);
	
	
	
	// 4 video cameras for testing
	// 0: FaceTime HD Camera (Built-in) - iMAC 2003
	// 1: Logitech HD Pro Webcam C920 (mine)
	// 2: Rocketfish HD Webcam Pro
	// 3: Integrated Camera - Lenovo ThinkPad
	// 4: Logitech QuickCam Pro 9000 (g2j)
	// If none is connected then the default device will be selected
	GOTHAM_ASSERT((ret = tmedia_producer_set_friendly_name(tmedia_video, "Logitech QuickCam Pro 9000")) == 0);
	
#if RUN_TEST_RTP
    test_rtp(RUN_TEST_RTP_AS_CLIENT);
#endif
	
    tdav_deinit();
    tnet_cleanup();
	
    return ret;
}
