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
#include "Utils.h"

#include "../thirdparties/source/npapi-sdk/headers/npapi.h"
#include "../thirdparties/source/npapi-sdk/headers/npfunctions.h"

extern NPNetscapeFuncs* BrowserFuncs;

// http://code.google.com/p/chromium/issues/detail?id=13564
void* Utils::MemDup(const void* mem, unsigned n)
{
	void *np_ret = NULL;
	if(mem && n){
		if((np_ret = BrowserFuncs->memalloc((n + 1)))){
			memcpy(np_ret, mem, n);
			((uint8_t*)np_ret)[n] = '\0';
		}
	}

	return np_ret;
}
