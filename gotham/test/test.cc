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

#define RUN_TEST_RTP				1
#define RUN_TEST_RTP_AS_CLIENT		1

#define DEFAULT_VIDEO_WIDTH					640
#define DEFAULT_VIDEO_HEIGHT				480
#define DEFAULT_VIDEO_FPS					30 // up to 120
#define DEFAULT_VIDEO_REMOTE_WINDOW_NAME	"Remote video window (Decoded RTP)" // Remote window is where the decoded video frames are displayed
#define DEFAULT_VIDEO_LOCAL_WINDOW_NAME		"Local video window" // Local window is where the encoded video frames are displayed before sending (preview, PIP mode).

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

static HWND GetLocalHwnd(void)
{
	return __hWndLocal;
}

static HWND GetRemoteHwnd(void)
{
	if(!__hWndRemote)
	{
		assert(__hWndRemote = ::CreateWindowA(
			"STATIC", 
			DEFAULT_VIDEO_REMOTE_WINDOW_NAME, 
			WS_VISIBLE, 
			0, 
			0, 
			DEFAULT_VIDEO_WIDTH, 
			DEFAULT_VIDEO_HEIGHT, 
			NULL/*GetConsoleHwnd()*//*Parent*/, 
			NULL, 
			NULL,
			NULL));
		::SetWindowTextA(__hWndRemote, DEFAULT_VIDEO_REMOTE_WINDOW_NAME);
	}
	return __hWndRemote;
}

#endif /* TMEDIA_UNDER_WINDOWS */

#if RUN_TEST_RTP
#include "test_rtp.h"
#endif /* RUN_TEST_RTP */

int _tmain(int argc, _TCHAR* argv[])
{
	int ret = 0;

    assert((ret = tnet_startup()) == 0);
    assert((ret = tdav_init()) == 0);

	assert((ret = tmedia_defaults_set_pref_video_size(tmedia_pref_video_size_vga)) == 0);
	assert((ret = tmedia_defaults_set_video_fps(DEFAULT_VIDEO_FPS)) == 0);

	// On my PC: 2 video cameras
	// 0: FaceTime HD Camera (Built-in)
	// 1: Logitech HD Pro Webcam C920
	// If none is connected then the default device will be selected
	assert((ret = tmedia_producer_set_friendly_name(tmedia_video, "Logitech HD Pro Webcam C920")) == 0);

#if RUN_TEST_RTP
    test_rtp(RUN_TEST_RTP_AS_CLIENT);
#endif

    tdav_deinit();
    tnet_cleanup();

    return ret;
}
