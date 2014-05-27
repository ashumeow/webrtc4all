/*
 *  webrtc4all.cp
 *  webrtc4all
 *
 *  Created by Mamadou DIOP on 5/19/14.
 *  Copyright (c) 2014 Doubango Telecom. All rights reserved.
 *
 */

#include "webrtc4all.h"
#include "webrtc4allPriv.h"

CFStringRef webrtc4allUUID(void)
{
	Cwebrtc4all* theObj = new Cwebrtc4all;
	return theObj->UUID();
}

CFStringRef Cwebrtc4all::UUID()
{
	return CFSTR("0001020304050607");
}
