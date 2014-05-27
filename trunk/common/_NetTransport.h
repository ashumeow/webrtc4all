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
#ifndef _WEBRTC4ALL__NETTRANSPORT_H_
#define _WEBRTC4ALL__NETTRANSPORT_H_

#include "_Config.h"
#include "_Utils.h"

class NetTransportEvent
{
public:
	NetTransportEvent(int type, const void* data, int dataLen)
		:mDataLen(0)
	{
		mType = type;
		if ((mData = _Utils::MemDup(data, dataLen))) {
			mDataLen = dataLen;
		}
	}

	~NetTransportEvent()
	{
		_Utils::MemFree(&mData);
	}

	int GetType()const{ return mType; }
	const void* GetData()const{ return mData; }
	int GetDataLen()const{ return mDataLen; }

private:
	int mType;
	void* mData;
	int mDataLen;
};

class _NetTransport {
public:
	_NetTransport();
	virtual ~_NetTransport();

public:
	virtual void DgramCbFire(const NetTransportEvent* e) = 0;
	virtual LONGLONG GetWindowHandle() = 0;

protected:
	virtual bool _SetDomain(const char* domain);
	virtual bool _Start(tnet_transport_cb_f cb_f);
	virtual bool _Stop();
	virtual bool _SendTo(const void* msg, int msg_len, const char* addr, int port);

	static int DgramCb(const struct tnet_transport_event_s* e);

protected:
	struct tnet_transport_s* m_pTransport;
	char* m_DefaultDestAddr;
	int m_DefaultDestPort;
};

#endif /* _WEBRTC4ALL__NETTRANSPORT_H_ */

