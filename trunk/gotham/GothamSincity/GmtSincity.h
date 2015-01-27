#pragma once

#include <windows.h>
#include <map>
#include <string>
#include <memory>

#include "GmtUrl.h"

#define kGmtUserLocalId		"001"
#define kGmtUserRemoteId	"002"


#define kSchemeWSS "wss"
#define kSchemeWS "ws"

typedef int GmtNetFd;
#define GmtNetFd_IsValid(self)	((self) > 0)
#define kGmtNetFdInvalid			-1

#define GMT_ASSERT(x) { \
	bool __b_ret = (x); \
	assert(__b_ret); \
}

#define SC_DEBUG_INFO(FMT, ...) TSK_DEBUG_INFO("[SINCITY] " FMT, ##__VA_ARGS__)
#define SC_DEBUG_WARN(FMT, ...) TSK_DEBUG_WARN("[SINCITY] " FMT, ##__VA_ARGS__)
#define SC_DEBUG_ERROR(FMT, ...) TSK_DEBUG_ERROR("[SINCITY] " FMT, ##__VA_ARGS__)
#define SC_DEBUG_FATAL(FMT, ...) TSK_DEBUG_FATAL("[SINCITY] " FMT, ##__VA_ARGS__)

#define SC_DEBUG_INFO_EX(MODULE, FMT, ...) SC_DEBUG_INFO("[" MODULE "] " FMT, ##__VA_ARGS__)
#define SC_DEBUG_WARN_EX(MODULE, FMT, ...) SC_DEBUG_WARN("[" MODULE "] " FMT, ##__VA_ARGS__)
#define SC_DEBUG_ERROR_EX(MODULE, FMT, ...) SC_DEBUG_ERROR("[" MODULE "] " FMT, ##__VA_ARGS__)
#define SC_DEBUG_FATAL_EX(MODULE, FMT, ...) SC_DEBUG_FATAL("[" MODULE "] " FMT, ##__VA_ARGS__)

typedef enum GmtNetTransporType_e {
	GmtNetTransporType_None = 0x00,
	GmtNetTransporType_TCP = (0x01 << 0),
	GmtNetTransporType_TLS = (0x01 << 1),
	GmtNetTransporType_WS = ((0x01 << 2) | GmtNetTransporType_TCP),
	GmtNetTransporType_WSS = ((0x01 << 3) | GmtNetTransporType_TLS),
	GmtNetTransporType_HTTP = ((0x01 << 4) | GmtNetTransporType_TCP),
	GmtNetTransporType_HTTPS = ((0x01 << 5) | GmtNetTransporType_TLS),
}
GmtNetTransporType_t;

typedef enum GmtUrlHostType_e {
	GmtUrlHostType_None = 0,
	GmtUrlHostType_Hostname,
	GmtUrlHostType_IPv4,
	GmtUrlHostType_IPv6
}
GmtUrlHostType_t;

typedef enum GmtUrlType_e {
	GmtUrlType_None = GmtNetTransporType_None,
	GmtUrlType_TCP = GmtNetTransporType_TCP,
	GmtUrlType_TLS = GmtNetTransporType_TLS,
	GmtUrlType_WS = GmtNetTransporType_WS,
	GmtUrlType_WSS = GmtNetTransporType_WSS,
	GmtUrlType_HTTP = GmtNetTransporType_HTTP,
	GmtUrlType_HTTPS = GmtNetTransporType_HTTPS,
}
GmtUrlType_t;

typedef enum GmtWsActionType_e {
	GmtWsActionType_None = 0,
}
GmtWsActionType_t;

template <typename T>
class GmtAutoLock
{
public:
	explicit GmtAutoLock(T* obj) : obj_(obj) {
		obj_->lock();
	}
	virtual ~GmtAutoLock() {
		obj_->unlock();
	}
protected:
	T* obj_;
};

typedef void GmtNativeMutexHandle_t; // Mapping to "tsk_mutex_handle_t"
typedef void GmtNativeNetTransportHandle_t; // Mapping to "tnet_transport_handle_t"
typedef void GmtNativeThreadHandle_t; // Mapping to "tsk_thread_handle_t"

class GmtNetTransport;
class GmtNetTransportCallback;

//
//	GmtNetPeer
//
class GmtNetPeer
{
	friend class GmtNetTransport;
	friend class GmtNetTransportCallback;
public:
	GmtNetPeer(GmtNetFd nFd, bool bConnected = false, const void* pcData = NULL, size_t nDataSize = 0);
	virtual ~GmtNetPeer();
	virtual _inline GmtNetFd getFd()
	{
		return  m_nFd;
	}
	virtual _inline bool isConnected()
	{
		return  m_bConnected;
	}
	virtual const void* getDataPtr();
	virtual size_t getDataSize();
	virtual _inline bool isRawContent()
	{
		return  m_bRawContent;
	}
	virtual _inline void setRawContent(bool bRawContent)
	{
		m_bRawContent = bRawContent;
	}
	virtual bool buildWsKey();
	virtual const char* getWsKey()
	{
		return m_pWsKey;
	}
	virtual _inline bool isStream() = 0;

protected:
	virtual _inline void setConnected(bool bConnected) {
		m_bConnected = bConnected;
	}

protected:
	bool m_bConnected;
	bool m_bRawContent;
	GmtNetFd m_nFd;
	struct tsk_buffer_s* m_pWrappedBuffer;
	char* m_pWsKey;
};


//
//	GmtNetPeerDgram
//
class GmtNetPeerDgram : public GmtNetPeer
{
public:
	GmtNetPeerDgram(GmtNetFd nFd, const void* pcData = NULL, size_t nDataSize = 0)
		:GmtNetPeer(nFd, false, pcData, nDataSize) {
	}
	virtual ~GmtNetPeerDgram() {
	}
	virtual _inline bool isStream()
	{
		return false;
	}
};

//
//	GmtNetPeerStream
//
class GmtNetPeerStream : public GmtNetPeer
{
public:
	GmtNetPeerStream(GmtNetFd nFd, bool bConnected = false, const void* pcData = NULL, size_t nDataSize = 0)
		:GmtNetPeer(nFd, bConnected, pcData, nDataSize)
	{
	}
	virtual ~GmtNetPeerStream()
	{
	}
	virtual _inline bool isStream()
	{
		return true;
	}
	virtual bool appenData(const void* pcData, size_t nDataSize);
	virtual bool remoteData(size_t nPosition, size_t nSize);
	virtual bool cleanupData();
};

//
//	GmtNetTransport
//
class GmtNetTransportCallback
{
public:
	GmtNetTransportCallback() {
	}
	virtual ~GmtNetTransportCallback() {
	}
	virtual bool onData(std::shared_ptr<GmtNetPeer> oPeer, size_t &nConsumedBytes) = 0;
	virtual bool onConnectionStateChanged(std::shared_ptr<GmtNetPeer> oPeer) = 0;
};

//
//	GmtNetTransport
//
class GmtNetTransport
{
protected:
	GmtNetTransport(GmtNetTransporType_t eType, const char* pcLocalIP, unsigned short nLocalPort);
public:
	virtual ~GmtNetTransport();
	virtual _inline GmtNetTransporType_t getType()
	{
		return m_eType;
	}
	virtual _inline bool isStarted()
	{
		return m_bStarted;
	}
	virtual _inline bool isValid()
	{
		return m_bValid;
	}
	virtual _inline void setCallback(std::shared_ptr<GmtNetTransportCallback> oCallback)
	{
		m_oCallback = oCallback;
	}

	virtual bool setSSLCertificates(const char* pcPrivateKey, const char* pcPublicKey, const char* pcCA, bool bVerify = false);
	virtual bool start();
	virtual GmtNetFd connectTo(const char* pcHost, unsigned short nPort);
	virtual bool isConnected(GmtNetFd nFd);
	virtual bool sendData(GmtNetFd nFdFrom, const void* pcDataPtr, size_t nDataSize);
	virtual bool sendData(std::shared_ptr<GmtNetPeer> oPeer, const void* pcDataPtr, size_t nDataSize);
	virtual bool close(GmtNetFd nFd);
	virtual bool stop();

private:
	std::shared_ptr<GmtNetPeer> getPeerByFd(GmtNetFd nFd);
	void insertPeer(std::shared_ptr<GmtNetPeer> oPeer);
	void removePeer(GmtNetFd nFd);
	static int GmtNetTransportCb_Stream(const struct tnet_transport_event_s* e);

protected:
	GmtNativeNetTransportHandle_t* m_pWrappedTransport;
	GmtNetTransporType_t m_eType;
	bool m_bValid, m_bStarted;
	std::map<GmtNetFd, std::shared_ptr<GmtNetPeer> > m_Peers;
	std::shared_ptr<GmtNetTransportCallback> m_oCallback;
	CRITICAL_SECTION m_PeersCritSec;
};



class GmtWsTransport;

//
//	GmtWsResult
//
class GmtWsResult
{
public:
	GmtWsResult(unsigned short nCode, const char* pcPhrase, const void* pcDataPtr = NULL, size_t nDataSize = 0, GmtWsActionType_t eActionType = GmtWsActionType_None);
	virtual ~GmtWsResult();

	virtual _inline unsigned short getCode()
	{
		return m_nCode;
	}
	virtual _inline const char* getPhrase()
	{
		return m_pPhrase;
	}
	virtual _inline const void* getDataPtr()
	{
		return m_pDataPtr;
	}
	virtual _inline size_t getDataSize()
	{
		return m_nDataSize;
	}
	virtual _inline GmtWsActionType_t getActionType()
	{
		return m_eActionType;
	}

private:
	unsigned short m_nCode;
	char* m_pPhrase;
	void* m_pDataPtr;
	size_t m_nDataSize;
	GmtWsActionType_t m_eActionType;
};

//
//	GmtWsTransportCallback
//
class GmtWsTransportCallback : public GmtNetTransportCallback
{
public:
	GmtWsTransportCallback(const GmtWsTransport* pcTransport);
	virtual ~GmtWsTransportCallback();
	virtual bool onData(std::shared_ptr<GmtNetPeer> oPeer, size_t &nConsumedBytes);
	virtual bool onConnectionStateChanged(std::shared_ptr<GmtNetPeer> oPeer);
private:
	const GmtWsTransport* m_pcTransport;
};


//
//	GmtWsTransport
//
class GmtWsTransport : public GmtNetTransport
{
	friend class GmtWsTransportCallback;
public:
	GmtWsTransport(bool isSecure, const char* pcLocalIP = NULL, unsigned short nLocalPort = 0);
	virtual ~GmtWsTransport();

	bool handshaking(std::shared_ptr<GmtNetPeer> oPeer, std::shared_ptr<GmtUrl> oUrl);

private:
	std::shared_ptr<GmtWsResult> handleJsonContent(std::shared_ptr<GmtNetPeer> oPeer, const void* pcDataPtr, size_t nDataSize)const;
	bool sendResult(std::shared_ptr<GmtNetPeer> oPeer, std::shared_ptr<GmtWsResult> oResult)const;

private:
	std::shared_ptr<GmtWsTransportCallback> m_oCallback;
};


class GmtSignaling;

typedef enum GmtSignalingEventType_e {
	GmtSignalingEventType_NetConnected,
	GmtSignalingEventType_NetReady,
	GmtSignalingEventType_NetData, // "passthrough" data: https://code.google.com/p/sincity/issues/detail?id=2
	GmtSignalingEventType_NetDisconnected,
	GmtSignalingEventType_NetError,

	GmtSignalingEventType_Call
}
GmtSignalingEventType_t;

typedef enum SCSessionState_e {
	SCSessionState_None,
	SCSessionState_Connecting,
	SCSessionState_Connected,
	SCSessionState_Terminated
}
SCSessionState_t;

/**@ingroup _Group_CPP_Signaling
* Signaling event.
*/
class GmtSignalingEvent
{
	friend class GmtSignaling;
public:
	GmtSignalingEvent(GmtSignalingEventType_t eType, std::string strDescription, const void* pcDataPtr = NULL, size_t nDataSize = 0);
	virtual ~GmtSignalingEvent();

	/**< The event type */
	virtual _inline GmtSignalingEventType_t getType()const 
	{
		return m_eType;
	}
	/**< The event description */
	virtual _inline std::string getDescription()const
	{
		return m_strDescription;
	}
	/**< The event data pointer */
	virtual _inline const void* getDataPtr()const
	{
		return m_pDataPtr;
	}
	virtual _inline size_t getDataSize()const
	{
		return m_nDataSize;
	}

private:
	GmtSignalingEventType_t m_eType;
	std::string m_strDescription;
	void* m_pDataPtr;
	size_t m_nDataSize;
};


/**@ingroup _Group_CPP_Signaling
* Signaling event for call sessions.
*/
class GmtSignalingCallEvent : public GmtSignalingEvent
{
	friend class GmtSignaling;
public:
	GmtSignalingCallEvent(std::string strDescription);
	virtual ~GmtSignalingCallEvent();

	/**< The event type. e.g. "offer", "answer", "hangup"... */
	_inline std::string getType()
	{
		return m_strType;
	}
	/**< The source identifier */
	_inline std::string getFrom()
	{
		return m_strFrom;
	}
	/**< The destination identifier */
	_inline std::string getTo()
	{
		return m_strTo;
	}
	/**< The call identifier */
	_inline std::string getCallId()
	{
		return m_strCallId;
	}
	/**< The transaction identifier */
	_inline std::string getTransacId()
	{
		return m_strTransacId;
	}
	/**< The session description. Could be NULL. */
	_inline std::string getSdp()
	{
		return m_strSdp;
	}

private:
	std::string m_strFrom;
	std::string m_strTo;
	std::string m_strSdp;
	std::string m_strType;
	std::string m_strCallId;
	std::string m_strTransacId;
};

/**@ingroup _Group_CPP_Signaling
* Callback class for the signaling session. You must override this call.
*/
class GmtSignalingCallback
{	
public:
	GmtSignalingCallback() {}
	virtual ~GmtSignalingCallback() {}
	/** Raised to signal events releated to the network connection states */
	virtual bool onEventNet(std::shared_ptr<GmtSignalingEvent>& e) = 0;
	/** Raised to signal events related to the call states */
	virtual bool onEventCall(std::shared_ptr<GmtSignalingCallEvent>& e) = 0;
};

class GmtSignalingTransportCallback : public GmtNetTransportCallback
{
public:
	GmtSignalingTransportCallback(const GmtSignaling* pcGmtSignaling);
	virtual ~GmtSignalingTransportCallback();

	virtual bool onData(std::shared_ptr<GmtNetPeer> oPeer, size_t &nConsumedBytes);

	virtual bool onConnectionStateChanged(std::shared_ptr<GmtNetPeer> oPeer);

private:
	const GmtSignaling* m_pcGmtSignaling;
};

class GmtSignaling
{
	friend class GmtSignalingTransportCallback;
	friend class GmtAutoLock<GmtSignaling>;
	
public:
	GmtSignaling(std::shared_ptr<GmtNetTransport>& oNetTransport, std::shared_ptr<GmtUrl>& oConnectionUrl);
	virtual ~GmtSignaling();

	bool setCallback(std::shared_ptr<GmtSignalingCallback> oCallback);
	bool isConnected();
	bool isReady();
	bool connect();
	bool sendData(const void* pcData, size_t nDataSize);
	bool disConnect();

	static std::shared_ptr<GmtSignaling> newObj(const char* pcConnectionUri, const char* pcLocalIP = NULL, unsigned short nLocalPort = 0);

private:
	bool handleData(const char* pcData, size_t nDataSize);
	bool raiseEvent(GmtSignalingEventType_t eType, std::string strDescription, const void* pcDataPtr = NULL, size_t nDataSize = 0);

	void lock();
	void unlock();

private:
	std::shared_ptr<GmtNetTransport> m_oNetTransport;
	std::shared_ptr<GmtUrl> m_oConnectionUrl;
	std::shared_ptr<GmtSignalingTransportCallback> m_oNetCallback;
	std::shared_ptr<GmtSignalingCallback> m_oSignCallback;
	GmtNetFd m_Fd;
	bool m_bWsHandshakingDone;
	void* m_pWsSendBufPtr;
	size_t m_nWsSendBuffSize;
	CRITICAL_SECTION m_critSec;
};


HRESULT GmtGetFullPath(LPWSTR szFileName, std::wstring & strFullPath);