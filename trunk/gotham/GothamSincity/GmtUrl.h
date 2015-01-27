#pragma once

#include <memory>
#include <string>

class GmtUrl
{	
public:
	GmtUrl(enum GmtUrlType_e eType, std::string strScheme, std::string strHost, std::string strHPath, std::string strSearch, unsigned short nPort, enum GmtUrlHostType_e eHostType);
	virtual ~GmtUrl();

	virtual _inline enum GmtUrlType_e getType()const
	{
		return m_eType;
	}
	virtual _inline enum GmtUrlHostType_e getHostType()const
	{
		return m_eHostType;
	}
	virtual _inline std::string getScheme()const
	{
		return m_strScheme;
	}
	virtual _inline std::string getHost()const
	{
		return m_strHost;
	}
	virtual _inline std::string getHPath()const
	{
		return m_strHPath;
	}
	virtual _inline std::string getSearch()const
	{
		return m_strSearch;
	}
	virtual _inline unsigned short getPort()const
	{
		return m_nPort;
	}

	static std::shared_ptr<GmtUrl> newObj(enum GmtUrlType_e eType, const char* pcScheme, const char* pcHost, const char* pcHPath, const char* pcSearch, unsigned short port, enum GmtUrlHostType_e eHostType);

private:
	enum GmtUrlType_e m_eType;
	enum GmtUrlHostType_e m_eHostType;
	std::string m_strScheme;
	std::string m_strHost;
	std::string m_strHPath;
	std::string m_strSearch;
	unsigned short m_nPort;
};

std::shared_ptr<GmtUrl> GmtParseUrl(const char *urlstring, size_t length);