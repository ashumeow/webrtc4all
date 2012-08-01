/*
* Copyright (C) 2012 Doubango Telecom <http://www.doubango.org>
*
* Contact: Mamadou Diop <diopmamadou(at)doubango[dot]org>
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
#include <atlbase.h>
#include <atlcom.h>
#include <atlctl.h>

class Cwebrtc4ieModule : public CAtlDllModuleT< Cwebrtc4ieModule >
{
public :
	DECLARE_LIBID(LIBID_webrtc4ieLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_WEBRTC4IE, "{1B6E1BA5-9220-46EE-975B-5A85E31781E2}")
};

extern class Cwebrtc4ieModule _AtlModule;
