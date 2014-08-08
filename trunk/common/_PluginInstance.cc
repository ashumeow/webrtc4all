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

#include "_PluginInstance.h"
#include "_Utils.h"

_PluginInstance::_PluginInstance()
{
	_Utils::Initialize();
}

_PluginInstance::~_PluginInstance()
{

}

const char* _PluginInstance::GetVersion()
{
	static const char* g_Version = THIS_VERSION;
	return g_Version;
}

bool _PluginInstance::GetSupportsPeerConnection()
{
	return true;
}

bool _PluginInstance::GetSupportsSessionDescription()
{
	return true;
}

bool _PluginInstance::GetSupportsNetTransport()
{
	return true;
}

long _PluginInstance::GetFps()
{
	return (long)tmedia_defaults_get_video_fps();
}

bool _PluginInstance::SetFps(long fps)
{
	return (tmedia_defaults_set_video_fps((int32_t)fps) == 0);
}

struct pref_video_size { const char* name; tmedia_pref_video_size_t size; };
static const pref_video_size pref_video_sizes[] =
{
	{"sqcif", tmedia_pref_video_size_sqcif}, // 128 x 98
	{"qcif", tmedia_pref_video_size_qcif}, // 176 x 144
	{"qvga", tmedia_pref_video_size_qvga}, // 320 x 240
	{"cif", tmedia_pref_video_size_cif}, // 352 x 288
	{"hvga", tmedia_pref_video_size_hvga}, // 480 x 320
	{"vga", tmedia_pref_video_size_vga}, // 640 x 480
	{"4cif", tmedia_pref_video_size_4cif}, // 704 x 576
	{"svga", tmedia_pref_video_size_svga}, // 800 x 600
	{"480p", tmedia_pref_video_size_480p}, // 852 x 480
	{"720p", tmedia_pref_video_size_720p}, // 1280 x 720
	{"16cif", tmedia_pref_video_size_16cif}, // 1408 x 1152
	{"1080p", tmedia_pref_video_size_1080p}, // 1920 x 1080
	{"2160p", tmedia_pref_video_size_2160p}, // 3840 x 2160
};
static const int pref_video_sizes_count = sizeof(pref_video_sizes)/sizeof(pref_video_sizes[0]);

const char* _PluginInstance::GetMaxVideoSize()
{
	int i;
	tmedia_pref_video_size_t size = tmedia_defaults_get_pref_video_size();
	for (i = 0; i < pref_video_sizes_count; ++i) {
		if (pref_video_sizes[i].size == size) {
			return pref_video_sizes[i].name;
		}
	}
	return "unknown";
}

bool _PluginInstance::SetMaxVideoSize(const char* maxSize)
{
	int i;
	for (i = 0; i < pref_video_sizes_count; ++i) {
		if (tsk_striequals(pref_video_sizes[i].name, maxSize)) {
			return (tmedia_defaults_set_pref_video_size(pref_video_sizes[i].size) == 0);
		}
	}
	return false;
}

long _PluginInstance::GetMaxBandwidthUp()
{
	return (long)tmedia_defaults_get_bandwidth_video_upload_max();
}

bool _PluginInstance::SetMaxBandwidthUp(long maxBandwidthUp)
{
	return (tmedia_defaults_set_bandwidth_video_upload_max((int32_t)maxBandwidthUp) == 0);
}

long _PluginInstance::GetMaxBandwidthDown()
{
	return (long)tmedia_defaults_get_bandwidth_video_download_max();
}

bool _PluginInstance::SetMaxBandwidthDown(long maxBandwidthDown)
{
	return (tmedia_defaults_set_bandwidth_video_download_max((int32_t)maxBandwidthDown) == 0);
}

bool _PluginInstance::GetZeroArtifacts()
{
	return (tmedia_defaults_get_video_zeroartifacts_enabled() == tsk_true);
}

bool _PluginInstance::SetZeroArtifacts(bool zeroArtifacts)
{
	return (tmedia_defaults_set_video_zeroartifacts_enabled(zeroArtifacts ? tsk_true : tsk_false) == 0);
}