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
#include "_SessionDescription.h"
#include "_Utils.h"

_SessionDescription::_SessionDescription():
m_Sdp(NULL)
{
	_Utils::Initialize();
}

_SessionDescription::~_SessionDescription()
{
	TSK_OBJECT_SAFE_FREE(m_Sdp);
}

bool _SessionDescription::Init(const char* sdpStr, int sdpLen)
{
	if(!sdpStr || !sdpLen){
		TSK_DEBUG_ERROR("Invalid argument");
		return false;
	}
	TSK_OBJECT_SAFE_FREE(m_Sdp);

	if(!(m_Sdp = tsdp_message_parse(sdpStr, sdpLen))){
		TSK_DEBUG_ERROR("Initialization failed");
		return false;
	}
	return true;
}

bool _SessionDescription::AddCandidate(const char* media, const char* candidate)
{
	bool bRet = true;
	tnet_ice_candidate_t* oCandidate = NULL;

	if(!media || !candidate){
		TSK_DEBUG_ERROR("Invalid argument");
		bRet = false;
		goto bail;
	}
	if(!m_Sdp){
		TSK_DEBUG_ERROR("Not initialized");
		bRet = false;
		goto bail;
	}
	
	const tsdp_header_M_t* M = tsdp_message_find_media(m_Sdp, media);
	if(!M){
		TSK_DEBUG_ERROR("Failed to find mediaType=%s", media);
		bRet = false;
		goto bail;
	}
	
	
	oCandidate = tnet_ice_candidate_parse(candidate);
	if(oCandidate){
		if(!((tsdp_header_M_t*)M)->port){
			((tsdp_header_M_t*)M)->port = oCandidate->port;
		}
		if(!((tsdp_header_M_t*)M)->C){
			tsdp_header_M_add_headers((tsdp_header_M_t*)M,
				TSDP_HEADER_C_VA_ARGS("IN", (FALSE ? "IP6" : "IP4"), oCandidate->connection_addr),
				TSDP_HEADER_A_VA_ARGS("ice-ufrag", tsk_params_get_param_value(oCandidate->extension_att_list, "webrtc4ie-ufrag")),
				TSDP_HEADER_A_VA_ARGS("ice-pwd", tsk_params_get_param_value(oCandidate->extension_att_list, "webrtc4ie-pwd")),
					tsk_null);
		}
		tsk_params_remove_param(oCandidate->extension_att_list, "webrtc4ie-ufrag");
		tsk_params_remove_param(oCandidate->extension_att_list, "webrtc4ie-pwd");

		tsdp_header_M_add_headers((tsdp_header_M_t*)M, TSDP_HEADER_A_VA_ARGS("candidate", tnet_ice_candidate_tostring(oCandidate)), tsk_null);
	}
	

bail:
	TSK_OBJECT_SAFE_FREE(oCandidate);

	return bRet;
}

const struct tsdp_message_s* _SessionDescription::GetSdp()const
{
	return m_Sdp;
}
