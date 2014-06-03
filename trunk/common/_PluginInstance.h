/*
* Copyright (C) 2014 Doubango Telecom <http://www.doubango.org>
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
#ifndef _WEBRTC4ALL__PLUGININSTANCE_H_
#define _WEBRTC4ALL__PLUGININSTANCE_H_

#include "_Config.h"

class _PluginInstance {
public:
	_PluginInstance();
	virtual ~_PluginInstance();

protected:
	static const char* GetVersion();
	static bool GetSupportsPeerConnection();
	static bool GetSupportsSessionDescription();
	static bool GetSupportsNetTransport();

	static long GetFps();
	static bool SetFps(long fps);
	static const char* GetMaxVideoSize();
	static bool SetMaxVideoSize(const char* maxSize);
	static long GetMaxBandwidthUp();
	static bool SetMaxBandwidthUp(long maxBandwidthUp);
	static long GetMaxBandwidthDown();
	static bool SetMaxBandwidthDown(long maxBandwidthDown);
};

#endif /* _WEBRTC4ALL__PLUGININSTANCE_H_ */
