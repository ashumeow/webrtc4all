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
#ifndef _WEBRTC4ALL_CONFIG_H_
#define _WEBRTC4ALL_CONFIG_H_

#if defined(WIN32)|| defined(_WIN32) || defined(_WIN32_WCE)
#	define W4A_UNDER_WINDOWS	1
#else
typedef int64_t LONGLONG;
typedef unsigned int UINT;
typedef long LRESULT;
typedef void* WPARAM;
typedef void* LPARAM;
typedef void* HWND;
#define CALLBACK
#endif

// OS X or iOS
#if defined(__APPLE__)
#	define W4A_UNDER_APPLE				1
#endif
#if TARGET_OS_MAC
#	define W4A_UNDER_MAC				1
#endif
#if TARGET_OS_IPHONE
#	define W4A_UNDER_IPHONE			1
#endif
#if TARGET_IPHONE_SIMULATOR
#	define W4A_UNDER_IPHONE_SIMULATOR	1
#endif

#ifdef _MSC_VER
#	define _CRT_SECURE_NO_WARNINGS
#	pragma warning( disable : 4996 )
#endif

#define THIS_VERSION	"1.35.981" // "1.37.1042"
#define USE_IPV6		FALSE
#define USE_RTCP_MUX	TRUE
#define USE_ICE_RTCP	TRUE
#define USE_ICE_JINGLE	FALSE
#define ICE_TIMEOUT_VAL			6000
#define ICE_TIMEOUT_ENDLESS		0

#define NP_MEMFREE(addr) \
	if((addr)){ \
		BrowserFuncs->memfree((addr)), (addr) = NULL; \
	}

#define NP_OBJECT_RELEASE(obj) \
	if((obj)){ \
		if((obj)->referenceCount > 0) BrowserFuncs->releaseobject((obj)); \
		(obj) = NULL; \
	}


#if W4A_IE
#ifndef STRICT
#define STRICT
#endif

#define _ATL_APARTMENT_THREADED
#define _ATL_NO_AUTOMATIC_NAMESPACE

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

#include <atlbase.h>
#include <atlcom.h>
#include <atlctl.h>

using namespace ATL;
#endif /* W4A_IE */

#if W4A_UNDER_APPLE
#import <WebKit/npapi.h>
#import <WebKit/npfunctions.h>
#import <WebKit/npruntime.h>
#else
#include "../thirdparties/source/npapi-sdk/headers/npapi.h"
#include "../thirdparties/source/npapi-sdk/headers/npfunctions.h"
#endif

#if !defined(NP_EXPORT)
#define NP_EXPORT(__type) __type
#endif

#if !defined(OSCALL)
#	if W4A_UNDER_WINDOWS
#       define OSCALL WINAPI
#   else
#       define OSCALL
#   endif
#endif

#if !defined(WM_USER)
#define WM_USER 0x0400
#endif
#define WM_NET_EVENT			(WM_USER + 201)
#define WM_ICE_EVENT_CANDIDATE	(WM_NET_EVENT + 1)
#define WM_ICE_EVENT_CONNECTED	(WM_NET_EVENT + 2)
#define WM_ICE_EVENT_CANCELLED	(WM_NET_EVENT + 3)
#define WM_ICE_EVENT_FAILED		(WM_NET_EVENT + 4)
#define WM_RFC5168_EVENT		(WM_NET_EVENT + 5)

typedef enum BrowserType_e
{
	BrowserType_None,
	BrowserType_Safari,
	BrowserType_Firefox,
	BrowserType_IE,
	BrowserType_Opera,
	BrowserType_Chrome
}
BrowserType_t;

#include "tsk.h"
#include "tinynet.h"
#include "tinymedia.h"
#include "tinydav/tdav.h"

#endif /* _WEBRTC4ALL_CONFIG_H_ */
