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
#include "_NetTransport.h"
#include "_Utils.h"

_NetTransport::_NetTransport():
m_pTransport(NULL),
m_DefaultDestAddr(NULL),
m_DefaultDestPort(0)
{
	TSK_DEBUG_INFO("_NetTransport::NetTransport");
	_Utils::Initialize();
}

_NetTransport::~_NetTransport()
{
	_Stop();
	TSK_OBJECT_SAFE_FREE(m_pTransport);
	TSK_FREE(m_DefaultDestAddr);
}

bool _NetTransport::_SetDomain(const char* domain)
{
	tnet_dns_ctx_t* dnsCtx = tnet_dns_ctx_create();
	if(!dnsCtx){
		TSK_DEBUG_ERROR("Failed to create SND context");
		return false;
	}
	
	tnet_port_t port;
	TSK_FREE(m_DefaultDestAddr);
	
	// char* addr = NULL;
	if(tnet_dns_query_naptr_srv(dnsCtx, domain, "SIP+D2U", &m_DefaultDestAddr, &port) == 0){
		m_DefaultDestPort = port;
	}
	
	TSK_OBJECT_SAFE_FREE(dnsCtx);

	return true;
}

bool _NetTransport::_Start(tnet_transport_cb_f cb_f)
{
	TSK_DEBUG_INFO("CNetTransport::Start");

	if(m_pTransport && TSK_RUNNABLE(m_pTransport)->started){
		TSK_DEBUG_WARN("Already started");
		return true;
	}

	int iRet;
	tnet_ip_t bestsource;
	if((iRet = tnet_getbestsource("sipml5.org", 5060, tnet_socket_type_udp_ipv4, &bestsource))){
		TSK_DEBUG_ERROR("Failed to get best source [%d]", iRet);
		memcpy(bestsource, "0.0.0.0\0", 8);
	}
	TSK_DEBUG_INFO("CNetTransport::bestsource=%s", bestsource);

	if(!m_pTransport){
		m_pTransport = tnet_transport_create(bestsource, TNET_SOCKET_PORT_ANY, tnet_socket_type_udp_ipv4, "NetTransport");
		if(!m_pTransport){
			TSK_DEBUG_ERROR("Failed to create network transport");
			return false;
		}
		tnet_transport_set_callback(m_pTransport, cb_f, this);
	}

	if((iRet = tnet_transport_start(m_pTransport)) != 0){
		TSK_DEBUG_ERROR("Failed to start transport [%d]", iRet);
		return false;
	}

	return true;
}

bool _NetTransport::_Stop()
{
	if(m_pTransport && TSK_RUNNABLE(m_pTransport)->started){
		int iRet;
		if((iRet = tnet_transport_shutdown(m_pTransport)) != 0){
			TSK_DEBUG_ERROR("Failed to stop transport [%d]", iRet);
			return false;
		}
	}

	return true;
}

bool _NetTransport::_SendTo(const void* msg, int msg_len, const char* addr, int port)
{
	if(!m_pTransport || !TSK_RUNNABLE(m_pTransport)->started){
		TSK_DEBUG_ERROR("Not started");
		return false;
	}

	struct sockaddr_storage addrO;
	if(tnet_sockaddr_init(addr, port, m_pTransport->master->type, &addrO)){
		TNET_PRINT_LAST_ERROR("tnet_sockaddr_init() failed");
		return false;
	}
	return (tnet_transport_sendto(m_pTransport, m_pTransport->master->fd, (const struct sockaddr *)&addrO, msg, msg_len) > 0);
}

int _NetTransport::DgramCb(const struct tnet_transport_event_s* e)
{
	_NetTransport* This =  (_NetTransport*)e->callback_data;

	TSK_DEBUG_INFO("W4A::NetTransport::DgramCb");

	switch(e->type){
		case event_data: {
			TSK_DEBUG_INFO("W4A::NetTransport::RECV:%.*s\n\n", e->size, (const char*)e->data);
			break;
						 }
		case event_closed:
		case event_connected:
		default:{
			return 0;
				}
	}

	NetTransportEvent* oEvent = new NetTransportEvent(e->type, e->data, e->size);
	if (oEvent) {
		if (!_Utils::PostMessage(This->GetWindowHandle(), WM_NET_EVENT, This, (void**)&oEvent)) {
            TSK_DEBUG_ERROR("PostMessage(WM_NET_EVENT) failed");
            This->DgramCbFire(oEvent);
        }
        if (oEvent) {
            delete oEvent, oEvent = NULL;
        }
	}

	return 0;
}