#pragma once

#include <string>

#include "GothamTypes.h"
#include "GothamObj.h"

class GmIceServer : public GmObj
{	
public:
	GmIceServer(
		std::string strTransport,
		std::string strServerHost,
		unsigned short serverPort,
		bool useTurn,
		bool useStun,
		std::string strUsername,
		std::string strPassword) 
		: m_strTransport(strTransport)
		, m_strServerHost(strServerHost)
		, m_uServerPort(serverPort)
		, m_bUseTurn(useTurn)
		, m_bUseStun(useStun)
		, m_strUsername(strUsername)
		, m_strPassword(strPassword)
	{
	}
	virtual ~GmIceServer() {}
	_inline const char* getTransport()const
	{
		return m_strTransport.c_str();
	}
	_inline const char* getServerHost()const
	{
		return m_strServerHost.c_str();
	}
	_inline unsigned short getServerPort()const
	{
		return m_uServerPort;
	}
	_inline bool isTurnEnabled()const
	{
		return m_bUseTurn;
	}
	_inline bool isStunEnabled()const
	{
		return m_bUseStun;
	}
	_inline const char* getUsername()const
	{
		return m_strUsername.c_str();
	}
	_inline const char* getPassword()const
	{
		return m_strPassword.c_str();
	}

private:
	std::string m_strTransport;
	std::string m_strServerHost;
	unsigned short m_uServerPort;
	bool m_bUseTurn;
	bool m_bUseStun;
	std::string m_strUsername;
	std::string m_strPassword;
};