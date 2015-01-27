#include "tnet_transport.h"

#include "tinyhttp.h"

#include "tsk_buffer.h"
#include "tsk_memory.h"
#include "tsk_base64.h"
#include "tsk_string.h"
#include "tsk_debug.h"

#include "tinydav/tdav_win32.h"

#include <assert.h>
#include <Shlwapi.h> /* PathRemoveFileSpec */

#include "GmtSincity.h"
#include "GmtJson.h"

#define kSCMobuleNameNetTransport "NetTransport"
#define kSCMobuleNameWsTransport "WebSocketTransport"
#define kSCMobuleNameSignaling "Signaling"
#define kSCMobuleNameFakeDisplay "Fake display"
#define kSCMobuleNameWEC7 "WEC7"

#if !defined(kGmtMaxStreamBufferSize)
#	define kGmtMaxStreamBufferSize 0xFFFF
#endif

//
//	GmtNetPeer
//

GmtNetPeer::GmtNetPeer(GmtNetFd nFd, bool bConnected /*= false*/, const void* pcData /*= NULL*/, size_t nDataSize /*= 0*/)
{
	m_bConnected = bConnected;
	m_nFd = nFd;
	m_pWrappedBuffer = tsk_buffer_create(pcData, nDataSize);
	m_bRawContent = false;
	m_pWsKey = tsk_null;
}

GmtNetPeer::~GmtNetPeer()
{
	TSK_OBJECT_SAFE_FREE(m_pWrappedBuffer);
	TSK_FREE(m_pWsKey);
}

const void* GmtNetPeer::getDataPtr()
{
	return m_pWrappedBuffer ? m_pWrappedBuffer->data : NULL;
}

size_t GmtNetPeer::getDataSize()
{
	return m_pWrappedBuffer ? m_pWrappedBuffer->size : 0;
}

#if 0
// IMPORTANT: data sent using this function will never be encrypted
bool GmtNetPeer::sendData(const void* pcDataPtr, size_t nDataSize)
{
	if (!pcDataPtr || !nDataSize) {
		SC_DEBUG_ERROR_EX(kSCMobuleNameNetTransport, "Invalid parameter");
		return false;
	}
	return (tnet_sockfd_send(getFd(), pcDataPtr, nDataSize, 0) == nDataSize);
}
#endif

bool GmtNetPeer::buildWsKey()
{
	char WsKey[30];
	const int count = sizeof(WsKey) / sizeof(WsKey[0]);
	for (int i = 0; i < count - 1; ++i) {
		WsKey[i] = rand() % 0xFF;
	}
	WsKey[count - 1] = '\0';

	return tsk_base64_encode((const uint8_t*)WsKey, (count - 1), &m_pWsKey) > 0;
}


//
//	GmtNetPeerStream
//

bool GmtNetPeerStream::appenData(const void* pcData, size_t nDataSize)
{
	return m_pWrappedBuffer ? tsk_buffer_append(m_pWrappedBuffer, pcData, nDataSize) == 0 : false;
}

bool GmtNetPeerStream::remoteData(size_t nPosition, size_t nSize)
{
	return m_pWrappedBuffer ? tsk_buffer_remove(m_pWrappedBuffer, nPosition, nSize) == 0 : false;
}

bool GmtNetPeerStream::cleanupData()
{
	return m_pWrappedBuffer ? tsk_buffer_cleanup(m_pWrappedBuffer) == 0 : false;
}


//
//	GmtNetTransport
//
GmtNetTransport::GmtNetTransport(GmtNetTransporType_t eType, const char* pcLocalIP, unsigned short nLocalPort)
	: m_bValid(false)
	, m_bStarted(false)
{
	m_eType = eType;
	const char *pcDescription;
	tnet_socket_type_t eSocketType;
	bool bIsIPv6 = false;

	if (pcLocalIP && nLocalPort)
	{
		bIsIPv6 = (tnet_get_family(pcLocalIP, nLocalPort) == AF_INET6);
	}

	switch (eType) {
	case GmtNetTransporType_TCP:
	case GmtNetTransporType_WS: {
		pcDescription = bIsIPv6 ? "TCP/IPv6 transport" : "TCP/IPv4 transport";
		eSocketType = bIsIPv6 ? tnet_socket_type_tcp_ipv6 : tnet_socket_type_tcp_ipv4;
		break;
	}
	case GmtNetTransporType_TLS:
	case GmtNetTransporType_WSS: {
		pcDescription = bIsIPv6 ? "TLS/IPv6 transport" : "TLS/IPv4 transport";
		eSocketType = bIsIPv6 ? tnet_socket_type_tls_ipv6 : tnet_socket_type_tls_ipv4;
		break;
	}
	default: {
		GMT_ASSERT(false);
		return;
	}
	}

	if ((m_pWrappedTransport = tnet_transport_create(pcLocalIP, nLocalPort, eSocketType, pcDescription))) {
		if (TNET_SOCKET_TYPE_IS_STREAM(eSocketType)) {
			tnet_transport_set_callback(m_pWrappedTransport, GmtNetTransport::GmtNetTransportCb_Stream, this);
		}
		else {
			GMT_ASSERT(false);
			return;
		}
	}

	InitializeCriticalSection(&m_PeersCritSec);

	m_bValid = (m_pWrappedTransport != NULL);
}

GmtNetTransport::~GmtNetTransport()
{
	stop();
	TSK_OBJECT_SAFE_FREE(m_pWrappedTransport);
	DeleteCriticalSection(&m_PeersCritSec);
	SC_DEBUG_INFO("*** GmtNetTransport destroyed ***");
}

bool GmtNetTransport::setSSLCertificates(const char* pcPrivateKey, const char* pcPublicKey, const char* pcCA, bool bVerify /*= false*/)
{
	return (tnet_transport_tls_set_certs(m_pWrappedTransport, pcCA, pcPublicKey, pcPrivateKey, (bVerify ? tsk_true : tsk_false)) == 0);
}

bool GmtNetTransport::start()
{
	m_bStarted = (tnet_transport_start(m_pWrappedTransport) == 0);
	return m_bStarted;
}

GmtNetFd GmtNetTransport::connectTo(const char* pcHost, unsigned short nPort)
{
	if (!pcHost || !nPort)
	{
		SC_DEBUG_ERROR_EX(kSCMobuleNameNetTransport, "Invalid parameter");
		return (GmtNetFd)TNET_INVALID_FD;
	}
	if (!isValid())
	{
		SC_DEBUG_ERROR_EX(kSCMobuleNameNetTransport, "Transport not valid");
		return (GmtNetFd)TNET_INVALID_FD;
	}
	if (!isStarted())
	{
		SC_DEBUG_ERROR_EX(kSCMobuleNameNetTransport, "Transport not started");
		return (GmtNetFd)TNET_INVALID_FD;
	}

	return tnet_transport_connectto_2(m_pWrappedTransport, pcHost, nPort);
}

bool GmtNetTransport::isConnected(GmtNetFd nFd)
{
	std::shared_ptr<GmtNetPeer> oPeer = getPeerByFd(nFd);
	return (oPeer && oPeer->isConnected());
}

bool GmtNetTransport::sendData(GmtNetFd nFdFrom, const void* pcDataPtr, size_t nDataSize)
{
	if (!pcDataPtr || !nDataSize || !GmtNetFd_IsValid(nFdFrom))
	{
		SC_DEBUG_ERROR_EX(kSCMobuleNameNetTransport, "Invalid parameter");
		return false;
	}
	return (tnet_transport_send(m_pWrappedTransport, nFdFrom, pcDataPtr, nDataSize) == nDataSize);
}

bool GmtNetTransport::sendData(std::shared_ptr<GmtNetPeer> oPeer, const void* pcDataPtr, size_t nDataSize)
{
	return sendData(oPeer->getFd(), pcDataPtr, nDataSize);
}

bool GmtNetTransport::close(GmtNetFd nFd)
{
	return (tnet_transport_remove_socket(m_pWrappedTransport, &nFd) == 0);
}

bool GmtNetTransport::stop()
{
	m_bStarted = false;
	return (tnet_transport_shutdown(m_pWrappedTransport) == 0);
}

std::shared_ptr<GmtNetPeer> GmtNetTransport::getPeerByFd(GmtNetFd nFd)
{
	EnterCriticalSection(&m_PeersCritSec);
	std::shared_ptr<GmtNetPeer> m_Peer = NULL;

	std::map<GmtNetFd, std::shared_ptr<GmtNetPeer> >::iterator iter = m_Peers.find(nFd);
	if (iter != m_Peers.end())
	{
		m_Peer = iter->second;
	}
	LeaveCriticalSection(&m_PeersCritSec);

	return m_Peer;
}

void GmtNetTransport::insertPeer(std::shared_ptr<GmtNetPeer> oPeer)
{
	EnterCriticalSection(&m_PeersCritSec);
	if (oPeer)
	{
		m_Peers.insert(std::pair<GmtNetFd, std::shared_ptr<GmtNetPeer> >(oPeer->getFd(), oPeer));
	}
	LeaveCriticalSection(&m_PeersCritSec);
}

void GmtNetTransport::removePeer(GmtNetFd nFd)
{
	EnterCriticalSection(&m_PeersCritSec);
	std::map<GmtNetFd, std::shared_ptr<GmtNetPeer> >::iterator iter;
	if ((iter = m_Peers.find(nFd)) != m_Peers.end())
	{
		std::shared_ptr<GmtNetPeer> oPeer = iter->second;
		m_Peers.erase(iter);
	}
	LeaveCriticalSection(&m_PeersCritSec);
}

int GmtNetTransport::GmtNetTransportCb_Stream(const tnet_transport_event_t* e)
{
	std::shared_ptr<GmtNetPeer> oPeer = NULL;
	GmtNetTransport* This = (GmtNetTransport*)e->callback_data;

	switch (e->type) {
	case event_removed:
	case event_closed: {
		oPeer = (This)->getPeerByFd(e->local_fd);
		if (oPeer) {
			oPeer->setConnected(false);
			(This)->removePeer(e->local_fd);
			if ((This)->m_oCallback)
			{
				(This)->m_oCallback->onConnectionStateChanged(oPeer);
			}
		}
		break;
	}
	case event_connected:
	case event_accepted: {
		oPeer = (This)->getPeerByFd(e->local_fd);
		if (oPeer)
		{
			oPeer->setConnected(true);
		}
		else
		{
			oPeer = std::make_shared<GmtNetPeerStream>(e->local_fd, true);
			(This)->insertPeer(oPeer);
		}
		if ((This)->m_oCallback)
		{
			(This)->m_oCallback->onConnectionStateChanged(oPeer);
		}
		break;
	}

	case event_data: {
		oPeer = (This)->getPeerByFd(e->local_fd);
		if (!oPeer)
		{
			SC_DEBUG_ERROR_EX(kSCMobuleNameNetTransport, "Data event but no peer found!");
			return -1;
		}

		size_t nConsumedBytes = oPeer->getDataSize();
		if ((nConsumedBytes + e->size) > kGmtMaxStreamBufferSize)
		{
			SC_DEBUG_ERROR_EX(kSCMobuleNameNetTransport, "Stream buffer too large[%u > %u]. Did you forget to consume the bytes?", (nConsumedBytes + e->size), kGmtMaxStreamBufferSize);
			dynamic_cast<GmtNetPeerStream*>(oPeer.get())->cleanupData();
		}
		else
		{
			if ((This)->m_oCallback)
			{
				if (dynamic_cast<GmtNetPeerStream*>(oPeer.get())->appenData(e->data, e->size))
				{
					nConsumedBytes += e->size;
				}
				(This)->m_oCallback->onData(oPeer, nConsumedBytes);
			}
			if (nConsumedBytes)
			{
				dynamic_cast<GmtNetPeerStream*>(oPeer.get())->remoteData(0, nConsumedBytes);
			}
		}
		break;
	}

	case event_error:
	default: {
		break;
	}
	}

	return 0;
}


//
//	WebSocket Transport implementation
//


/* min size of a stream chunck to form a valid HTTP message */
#define kStreamChunckMinSize 0x32
#define kHttpMethodOptions "OPTIONS"
#define kHttpMethodPost "POST"

#define kJsonContentType "application/json"

#define kGmtWsResultCode_Provisional		100
#define kGmtWsResultCode_Success			200
#define kGmtWsResultCode_Unauthorized		403
#define kGmtWsResultCode_NotFound			404
#define kGmtWsResultCode_ParsingFailed		420
#define kGmtWsResultCode_InvalidDataType	483
#define kGmtWsResultCode_InvalidData		450
#define kGmtWsResultCode_InternalError		603

#define kGmtWsResultPhrase_Success					"OK"
#define kGmtWsResultPhrase_Unauthorized			"Unauthorized"
#define kGmtWsResultPhrase_NotFound				"Not Found"
#define kGmtWsResultPhrase_ParsingFailed			"Parsing failed"
#define kGmtWsResultPhrase_InvalidDataType			"Invalid data type"
#define kGmtWsResultPhrase_FailedToCreateLocalFile	"Failed to create local file"
#define kGmtWsResultPhrase_FailedTransferPending	"Failed transfer pending"
#define kGmtWsResultPhrase_InternalError			"Internal Error"

#define kJsonField_Action "action"
#define kJsonField_Name "name"
#define kJsonField_Type "type"



static _inline bool isJsonContentType(const thttp_message_t *pcHttpMessage);
static const char* getHttpContentType(const thttp_message_t *pcHttpMessage);

#define SC_JSON_GET(fieldParent, fieldVarName, fieldName, typeTestFun, couldBeNull) \
	if(!fieldParent.isObject()){ \
		SC_DEBUG_ERROR_EX(kSCMobuleNameWsTransport, "JSON '%s' not an object", (fieldName)); \
		return new GmtWsResult(kGmtWsResultCode_InvalidDataType, kGmtWsResultPhrase_InvalidDataType); \
		} \
	const Json::Value fieldVarName = (fieldParent)[(fieldName)]; \
	if((fieldVarName).isNull()) \
		{ \
		if(!(couldBeNull)){ \
				SC_DEBUG_ERROR_EX(kSCMobuleNameWsTransport, "JSON '%s' is null", (fieldName)); \
				return new GmtWsResult(kGmtWsResultCode_InvalidDataType, "Required field is missing"); \
				}\
		} \
	if(!(fieldVarName).typeTestFun()) \
		{ \
		SC_DEBUG_ERROR_EX(kSCMobuleNameWsTransport, "JSON '%s' has invalid type", (fieldName)); \
		return new GmtWsResult(kGmtWsResultCode_InvalidDataType, kGmtWsResultPhrase_InvalidDataType); \
		}

//
//	GmtWsResult
//

GmtWsResult::GmtWsResult(unsigned short nCode, const char* pcPhrase, const void* pcDataPtr, size_t nDataSize, GmtWsActionType_t eActionType)
	: m_pDataPtr(NULL)
	, m_nDataSize(0)
	, m_eActionType(eActionType)
{
	m_nCode = nCode;
	m_pPhrase = tsk_strdup(pcPhrase);
	if (pcDataPtr && nDataSize)
	{
		if ((m_pDataPtr = tsk_malloc(nDataSize)))
		{
			memcpy(m_pDataPtr, pcDataPtr, nDataSize);
			m_nDataSize = nDataSize;
		}
	}
}

GmtWsResult::~GmtWsResult()
{
	TSK_FREE(m_pPhrase);
	TSK_FREE(m_pDataPtr);

	SC_DEBUG_INFO("*** GmtWsResult destroyed ***");
}

//
//	GmtWsTransport
//

GmtWsTransport::GmtWsTransport(bool isSecure, const char* pcLocalIP /*= NULL*/, unsigned short nLocalPort /*= 0*/)
	: GmtNetTransport(isSecure ? GmtNetTransporType_WSS : GmtNetTransporType_WS, pcLocalIP, nLocalPort)
{
	m_oCallback = std::make_shared<GmtWsTransportCallback>(this);
	setCallback(m_oCallback);
}

GmtWsTransport::~GmtWsTransport()
{
	setCallback(NULL);

	SC_DEBUG_INFO("*** GmtWsTransport destroyed ***");
}

bool GmtWsTransport::handshaking(std::shared_ptr<GmtNetPeer> oPeer, std::shared_ptr<GmtUrl> oUrl)
{
	if (!isConnected(oPeer->getFd()))
	{
		SC_DEBUG_ERROR_EX(kSCMobuleNameWsTransport, "Peer with fd=%d not connected yet", oPeer->getFd());
		return false;
	}
	if (!oPeer->buildWsKey())
	{
		return false;
	}

	tnet_ip_t localIP;
	tnet_port_t local_port;
	if (tnet_transport_get_ip_n_port(m_pWrappedTransport, oPeer->getFd(), &localIP, &local_port) != 0)
	{
		return false;
	}

	char* requestUri = tsk_null;
	if (oUrl->getHPath().empty())
	{
		requestUri = tsk_strdup("/");
	}
	else
	{
		tsk_sprintf(&requestUri, "/%s", oUrl->getHPath().c_str());
		if (!oUrl->getSearch().empty())
		{
			tsk_strcat_2(&requestUri, "?%s", oUrl->getSearch().c_str());
		}
	}

#define WS_REQUEST_GET_FORMAT "GET %s HTTP/1.1\r\n" \
	   "Host: %s\r\n" \
	   "Upgrade: websocket\r\n" \
	   "Connection: Upgrade\r\n" \
	   "Sec-WebSocket-Key: %s\r\n" \
	   "Origin: %s\r\n" \
	   "Sec-WebSocket-Protocol: ge-webrtc-signaling\r\n" \
	   "Sec-WebSocket-Version: 13\r\n" \
		"\r\n"

	char* request = tsk_null;
	tsk_sprintf(&request, WS_REQUEST_GET_FORMAT,
		requestUri,
		oUrl->getHost().c_str(),
		oPeer->getWsKey(),
		localIP);

	TSK_FREE(requestUri);

	bool ret = sendData(oPeer, request, tsk_strlen(request));
	TSK_FREE(request);
	return ret;
}

std::shared_ptr<GmtWsResult> GmtWsTransport::handleJsonContent(std::shared_ptr<GmtNetPeer> oPeer, const void* pcDataPtr, size_t nDataSize)const
{
	SC_DEBUG_ERROR_EX(kSCMobuleNameWsTransport, "Not implemented");
	return NULL;
}

bool GmtWsTransport::sendResult(std::shared_ptr<GmtNetPeer> oPeer, std::shared_ptr<GmtWsResult> oResult)const
{
	SC_DEBUG_ERROR_EX(kSCMobuleNameWsTransport, "Not implemented");
	return false;
}

//
//	GmtWsTransportCallback
//
GmtWsTransportCallback::GmtWsTransportCallback(const GmtWsTransport* pcTransport)
{
	m_pcTransport = pcTransport;
}

GmtWsTransportCallback::~GmtWsTransportCallback()
{

}

bool GmtWsTransportCallback::onData(std::shared_ptr<GmtNetPeer> oPeer, size_t &nConsumedBytes)
{
	SC_DEBUG_INFO_EX(kSCMobuleNameWsTransport, "Incoming data = %.*s", oPeer->getDataSize(), (const char*)oPeer->getDataPtr());
	SC_DEBUG_ERROR_EX(kSCMobuleNameWsTransport, "Not implemented");
	return false;
}

bool GmtWsTransportCallback::onConnectionStateChanged(std::shared_ptr<GmtNetPeer> oPeer)
{
	if (!oPeer->isConnected()) {

	}
	return true;
}

static const char* getHttpContentType(const thttp_message_t *pcHttpMessage)
{
	const thttp_header_Content_Type_t* contentType;

	if (!pcHttpMessage)
	{
		SC_DEBUG_ERROR_EX(kSCMobuleNameWsTransport, "Invalid parameter");
		return NULL;
	}

	if ((contentType = (const thttp_header_Content_Type_t*)thttp_message_get_header(pcHttpMessage, thttp_htype_Content_Type)))
	{
		return contentType->type;
	}
	return NULL;
}

static _inline bool isContentType(const thttp_message_t *pcHttpMessage, const char* pcContentTypeToCompare)
{
	// content-type without parameters
	const char* pcContentType = getHttpContentType(pcHttpMessage);
	if (pcContentType)
	{
		return tsk_striequals(pcContentTypeToCompare, pcContentType);
	}
	return false;
}

static _inline bool isJsonContentType(const thttp_message_t *pcHttpMessage)
{
	return isContentType(pcHttpMessage, kJsonContentType);
}







//
//	Sincity signaling
//


/**@defgroup _Group_CPP_Signaling Signaling
* @brief Signaling session class.
*/
#undef SC_JSON_GET
#define SC_JSON_GET(fieldParent, fieldVarName, fieldName, typeTestFun, couldBeNull) \
	if(!fieldParent.isObject()) \
		{ \
		SC_DEBUG_ERROR_EX(kSCMobuleNameSignaling, "JSON '%s' not an object", (fieldName)); \
		return false; \
		} \
	const Json::Value fieldVarName = (fieldParent)[(fieldName)]; \
	if((fieldVarName).isNull()) \
		{ \
		if(!(couldBeNull)) \
				{ \
			SC_DEBUG_ERROR_EX(kSCMobuleNameSignaling, "JSON '%s' is null", (fieldName)); \
			return false; \
				}\
		} \
		else if(!(fieldVarName).typeTestFun()) \
	{ \
		SC_DEBUG_ERROR_EX(kSCMobuleNameSignaling, "JSON '%s' has invalid type", (fieldName)); \
		return false; \
	}

/*
* Protected constructor to create new signaling session.
*/
GmtSignaling::GmtSignaling(std::shared_ptr<GmtNetTransport>& oNetTransport, std::shared_ptr<GmtUrl>& oConnectionUrl)
	: m_oNetTransport(oNetTransport)
	, m_oConnectionUrl(oConnectionUrl)
	, m_Fd(kGmtNetFdInvalid)
	, m_bWsHandshakingDone(false)
	, m_pWsSendBufPtr(NULL)
	, m_nWsSendBuffSize(0)
{
	GMT_ASSERT(m_oNetTransport != NULL);
	GMT_ASSERT(m_oConnectionUrl != NULL);

	m_oNetCallback = std::make_shared<GmtSignalingTransportCallback>(this);
	m_oNetTransport->setCallback(m_oNetCallback);

	InitializeCriticalSection(&m_critSec);
}

/*
* Signaling session destructor.
*/
GmtSignaling::~GmtSignaling()
{
	m_oNetTransport->setCallback(NULL);

	TSK_FREE(m_pWsSendBufPtr);
	DeleteCriticalSection(&m_critSec);
}

/*
* Locks the signaling session object to avoid conccurent access. The mutex is recurssive.
*/
void GmtSignaling::lock()
{
	EnterCriticalSection(&m_critSec);
}

/*
* Unlocks the signaling session object. The mutex is recurssive.
*/
void GmtSignaling::unlock()
{
	LeaveCriticalSection(&m_critSec);
}

/**@ingroup _Group_CPP_Signaling
* Sets the callback object.
* @param callback Callback object.
* @retval <b>true</b> if no error; otherwise <b>false</b>.
*/
bool GmtSignaling::setCallback(std::shared_ptr<GmtSignalingCallback> callback)
{
	GmtAutoLock<GmtSignaling> autoLock(this);

	m_oSignCallback = callback;
	return true;
}

/**@ingroup _Group_CPP_Signaling
* Checks whether the signaling session is connected to the network. Being connected doesn't mean the handshaking is done.
* Use @ref isReady() to check if the session is ready to send/receive data.
* @retval <b>true</b> if connected; otherwise <b>false</b>.
* @sa @ref isReady()
*/
bool GmtSignaling::isConnected()
{
	GmtAutoLock<GmtSignaling> autoLock(this);

	return m_oNetTransport->isConnected(m_Fd);
}

/**@ingroup _Group_CPP_Signaling
* Checks whether the signaling session is ready to send/receive data. A signaling session using WebSocket transport will be ready when the handshaking is done.
* @retval <b>true</b> if ready; otherwise <b>false</b>.
* @sa @ref isConnected()
*/
bool GmtSignaling::isReady()
{
	GmtAutoLock<GmtSignaling> autoLock(this);

	if (!isConnected())
	{
		return false;
	}
	if (m_oConnectionUrl->getType() == GmtUrlType_WS || m_oConnectionUrl->getType() == GmtUrlType_WSS)
	{
		return m_bWsHandshakingDone;
	}
	return true;
}

/**@ingroup _Group_CPP_Signaling
* Connects the signaling session to the remove server. The connection will be done asynchronously and you need to use @ref setCallback() to register a new callback
* object to listen for network events. When WebSocket transport is used this function will start the handshaking phase once the TCP/TLS socket is connected.
* @retval <b>true</b> if no error; otherwise <b>false</b>.
* @sa @ref disConnect()
*/
bool GmtSignaling::connect()
{
	GmtAutoLock<GmtSignaling> autoLock(this);

	if (isConnected())
	{
		SC_DEBUG_INFO_EX(kSCMobuleNameSignaling, "Already connected");
		return true;
	}

	if (!m_oNetTransport->isStarted())
	{
		if (!m_oNetTransport->start())
		{
			return false;
		}
	}

	SC_DEBUG_INFO_EX(kSCMobuleNameSignaling, "Connecting to [%s:%u]", m_oConnectionUrl->getHost().c_str(), m_oConnectionUrl->getPort());
	m_Fd = m_oNetTransport->connectTo(m_oConnectionUrl->getHost().c_str(), m_oConnectionUrl->getPort());
	return GmtNetFd_IsValid(m_Fd);
}

/**@ingroup _Group_CPP_Signaling
* Disconnect the signaling session.
* @retval <b>true</b> if no error; otherwise <b>false</b>.
* @sa @ref connect()
*/
bool GmtSignaling::disConnect()
{
	GmtAutoLock<GmtSignaling> autoLock(this);

	if (isConnected())
	{
		bool ret = m_oNetTransport->close(m_Fd);
		if (ret)
		{
			// m_Fd = kGmtNetFdInvalid;
		}
		return ret;
	}
	return true;
}

/**@ingroup _Group_CPP_Signaling
* Sends data to the server.
* @param _pcData Pointer to the data to send.
* @param _nDataSize Size (in bytes) of the data to send.
* @retval <b>true</b> if no error; otherwise <b>false</b>.
*/
bool GmtSignaling::sendData(const void* _pcData, tsk_size_t _nDataSize)
{
	GmtAutoLock<GmtSignaling> autoLock(this);

	if (!_pcData || !_nDataSize)
	{
		SC_DEBUG_ERROR_EX(kSCMobuleNameSignaling, "Invalid argument");
		return false;
	}

	if (!isReady())
	{
		SC_DEBUG_ERROR_EX(kSCMobuleNameSignaling, "Not ready yet");
		return false;
	}

	SC_DEBUG_INFO_EX(kSCMobuleNameSignaling, "Send DATA:%.*s", _nDataSize, _pcData);

	if (m_oConnectionUrl->getType() == GmtUrlType_WS || m_oConnectionUrl->getType() == GmtUrlType_WSS)
	{
		if (!m_bWsHandshakingDone)
		{
			SC_DEBUG_ERROR_EX(kSCMobuleNameSignaling, "WebSocket handshaking not done yet");
			return false;
		}
		uint8_t mask_key[4] = { 0x00, 0x00, 0x00, 0x00 };
		const uint8_t* pcData = (const uint8_t*)_pcData;
		uint64_t nDataSize = 1 + 1 + 4/*mask*/ + _nDataSize;
		uint64_t lsize = (uint64_t)_nDataSize;
		uint8_t* pws_snd_buffer;

		if (lsize > 0x7D && lsize <= 0xFFFF)
		{
			nDataSize += 2;
		}
		else if (lsize > 0xFFFF)
		{
			nDataSize += 8;
		}
		if (m_nWsSendBuffSize < nDataSize)
		{
			if (!(m_pWsSendBufPtr = tsk_realloc(m_pWsSendBufPtr, (tsk_size_t)nDataSize)))
			{
				SC_DEBUG_ERROR_EX(kSCMobuleNameSignaling, "Failed to allocate buffer with size = %llu", nDataSize);
				m_nWsSendBuffSize = 0;
				return 0;
			}
			m_nWsSendBuffSize = (tsk_size_t)nDataSize;
		}
		pws_snd_buffer = (uint8_t*)m_pWsSendBufPtr;

		pws_snd_buffer[0] = 0x81; // FIN | opcode-non-control::text
		pws_snd_buffer[1] = 0x80; // Set Mask flag (required for data from client->sever)

		if (lsize <= 0x7D)
		{
			pws_snd_buffer[1] |= (uint8_t)lsize;
			pws_snd_buffer = &pws_snd_buffer[2];
		}
		else if (lsize <= 0xFFFF)
		{
			pws_snd_buffer[1] |= 0x7E;
			pws_snd_buffer[2] = (lsize >> 8) & 0xFF;
			pws_snd_buffer[3] = (lsize & 0xFF);
			pws_snd_buffer = &pws_snd_buffer[4];
		}
		else 
		{
			pws_snd_buffer[1] |= 0x7F;
			pws_snd_buffer[2] = (lsize >> 56) & 0xFF;
			pws_snd_buffer[3] = (lsize >> 48) & 0xFF;
			pws_snd_buffer[4] = (lsize >> 40) & 0xFF;
			pws_snd_buffer[5] = (lsize >> 32) & 0xFF;
			pws_snd_buffer[6] = (lsize >> 24) & 0xFF;
			pws_snd_buffer[7] = (lsize >> 16) & 0xFF;
			pws_snd_buffer[8] = (lsize >> 8) & 0xFF;
			pws_snd_buffer[9] = (lsize & 0xFF);
			pws_snd_buffer = &pws_snd_buffer[10];
		}

		// Mask Key
		pws_snd_buffer[0] = mask_key[0];
		pws_snd_buffer[1] = mask_key[1];
		pws_snd_buffer[2] = mask_key[2];
		pws_snd_buffer[3] = mask_key[3];
		pws_snd_buffer = &pws_snd_buffer[4];
		// Mask dat
		// ... nothing done because key is always zeros

		// append payload to headers
		memcpy(pws_snd_buffer, pcData, (size_t)lsize);
		// send() data
		return m_oNetTransport->sendData(m_Fd, m_pWsSendBufPtr, (tsk_size_t)nDataSize);
	}
	else 
	{
		SC_DEBUG_ERROR_EX(kSCMobuleNameSignaling, "Not implemented yet");
		return false;
	}
}

/**@ingroup _Group_CPP_Signaling
* Creates new signaling session object.
* @param pcConnectionUri A valid request URI (e.g. <b>ws://localhost:9000/wsStringStaticMulti?roomId=0</b>).
* @param pcLocalIP Local IP address to bind to. Best one will be used if not defined.
* @param nLocalPort Local Port to bind to. Best one will be used if not defined.
* @retval <b>newobject</b> if no error; otherwise <b>NULL</b>.
*/
std::shared_ptr<GmtSignaling> GmtSignaling::newObj(const char* pcConnectionUri, const char* pcLocalIP /*= NULL*/, unsigned short nLocalPort /*= 0*/)
{
	std::shared_ptr<GmtSignaling> oSignaling;
	std::shared_ptr<GmtUrl> oUrl;
	std::shared_ptr<GmtNetTransport> oTransport;
	static bool sNetInitialized = false;

	if (!sNetInitialized)
	{
		GMT_ASSERT(tnet_startup() == 0);
		sNetInitialized = true;
	}

	if (tsk_strnullORempty(pcConnectionUri)) 
	{
		SC_DEBUG_ERROR_EX(kSCMobuleNameSignaling, "RequestUri is null or empty");
		goto bail;
	}

	oUrl = GmtParseUrl(pcConnectionUri, tsk_strlen(pcConnectionUri));
	if (!oUrl)
	{
		SC_DEBUG_ERROR_EX(kSCMobuleNameSignaling, "Failed to parse request Uri: %s", pcConnectionUri);
		goto bail;
	}
	if (oUrl->getHostType() == GmtUrlHostType_None)
	{
		SC_DEBUG_ERROR_EX(kSCMobuleNameSignaling, "Invalid host type: %s // %d", pcConnectionUri, oUrl->getHostType());
		goto bail;
	}
	switch (oUrl->getType()) {
	case GmtUrlType_WS:
	case GmtUrlType_WSS: {
		oTransport = std::make_shared<GmtWsTransport>((oUrl->getType() == GmtUrlType_WSS), pcLocalIP, nLocalPort);
		if (!oTransport) 
		{
			goto bail;
		}
		break;
	}
	default: {
		SC_DEBUG_ERROR_EX(kSCMobuleNameSignaling, "Url type=%d not supported yet. Url=%s", oUrl->getType(), pcConnectionUri);
		goto bail;
	}
	}

	oSignaling = std::make_shared<GmtSignaling>(oTransport, oUrl);

bail:
	return oSignaling;
}

/*
* Process incoming data
* @param pcData
* @param nDataSize
* @retval <b>true</b> if no error; otherwise <b>false</b>.
*/
bool GmtSignaling::handleData(const char* pcData, tsk_size_t nDataSize)
{
	GmtAutoLock<GmtSignaling> autoLock(this);

	Json::Value root;
	Json::Reader reader;

	// Parse JSON content
	bool parsingSuccessful = reader.parse((const char*)pcData, (((const char*)pcData) + nDataSize), root);
	if (!parsingSuccessful)
	{
		SC_DEBUG_ERROR_EX(kSCMobuleNameSignaling, "Failed to parse JSON content: %.*s", nDataSize, pcData);
		return false;
	}

	SC_JSON_GET(root, passthrough, "passthrough", isBool, true);

	if (passthrough.isBool() && passthrough.asBool() == true)
	{
		if (m_oSignCallback) 
		{
			return raiseEvent(GmtSignalingEventType_NetData, "'passthrough' JSON data", (const void*)pcData, nDataSize);
		}
		return false;
	}

	SC_JSON_GET(root, type, "type", isString, false);
	SC_JSON_GET(root, cid, "cid", isString, false);
	SC_JSON_GET(root, tid, "tid", isString, false);
	SC_JSON_GET(root, from, "from", isString, false);
	SC_JSON_GET(root, to, "to", isString, false);

	if (to.asString() != kGmtUserLocalId)
	{
		if (from.asString() == kGmtUserLocalId)
		{
			SC_DEBUG_INFO_EX(kSCMobuleNameSignaling, "Ignoring loopback message with type='%s', call-id='%s', to='%s'", type.asCString(), cid.asCString(), to.asCString());
		}
		else 
		{
			SC_DEBUG_INFO_EX(kSCMobuleNameSignaling, "Failed to match destination id: '%s'<>'%s'", to.asCString(), kGmtUserLocalId);
		}
		return false;
	}

	bool bIsCallEventRequiringSdp = (type.asString().compare("offer") == 0 || type.asString().compare("answer") == 0 || type.asString().compare("pranswer") == 0);
	if (bIsCallEventRequiringSdp || type.asString().compare("hangup") == 0)
	{
		if (m_oSignCallback)
		{
			std::shared_ptr<GmtSignalingCallEvent> oCallEvent = std::make_shared<GmtSignalingCallEvent>(type.asString());
			if (bIsCallEventRequiringSdp)
			{
				SC_JSON_GET(root, sdp, "sdp", isString, false);
				oCallEvent->m_strSdp = sdp.asString();
			}
			oCallEvent->m_strType = type.asString();
			oCallEvent->m_strCallId = cid.asString();
			oCallEvent->m_strTransacId = tid.asString();
			oCallEvent->m_strFrom = from.asString();
			oCallEvent->m_strTo = to.asString();
			return m_oSignCallback->onEventCall(oCallEvent);
		}
	}
	else
	{
		SC_DEBUG_ERROR_EX(kSCMobuleNameSignaling, "Message with type='%s' not supported yet", type.asString().c_str());
		return false;
	}

	return true;
}

/*
* Raises an event.
* @param eType
* @param strDescription
* @retval <b>true</b> if no error; otherwise <b>false</b>.
*/
bool GmtSignaling::raiseEvent(GmtSignalingEventType_t eType, std::string strDescription, const void* pcDataPtr /*= NULL*/, size_t nDataSize /*= 0*/)
{
	GmtAutoLock<GmtSignaling> autoLock(this);

	if (m_oSignCallback) 
	{
		std::shared_ptr<GmtSignalingEvent> e = std::make_shared<GmtSignalingEvent>(eType, strDescription, pcDataPtr, nDataSize);
		return m_oSignCallback->onEventNet(e);
	}
	return true;
}

//
//	GmtSignalingTransportCallback
//

GmtSignalingTransportCallback::GmtSignalingTransportCallback(const GmtSignaling* pcGmtSignaling)
	: m_pcGmtSignaling(pcGmtSignaling)
{

}

GmtSignalingTransportCallback::~GmtSignalingTransportCallback()
{
	m_pcGmtSignaling = NULL;
}

bool GmtSignalingTransportCallback::onData(std::shared_ptr<GmtNetPeer> oPeer, size_t &nConsumedBytes)
{
	GmtAutoLock<GmtSignaling> autoLock(const_cast<GmtSignaling*>(m_pcGmtSignaling));

	SC_DEBUG_INFO_EX(kSCMobuleNameSignaling, "Incoming data = %.*s", oPeer->getDataSize(), (const char*)oPeer->getDataPtr());

	if (m_pcGmtSignaling->m_oConnectionUrl->getType() != GmtUrlType_WS && m_pcGmtSignaling->m_oConnectionUrl->getType() != GmtUrlType_WSS)
	{
		SC_DEBUG_ERROR_EX(kSCMobuleNameSignaling, "Not implemented yet");
		return false;
	}

	if (!m_pcGmtSignaling->m_bWsHandshakingDone)
	{
		/* WebSocket handshaking data */

		if (oPeer->getDataSize() > 4) 
		{
			const char* pcData = (const char*)oPeer->getDataPtr();
			if (pcData[0] == 'H' && pcData[1] == 'T' && pcData[2] == 'T' && pcData[3] == 'P')
			{
				int endOfMessage = tsk_strindexOf(pcData, oPeer->getDataSize(), "\r\n\r\n"/*2CRLF*/) + 4;
				if (endOfMessage > 4) 
				{
					thttp_message_t *p_msg = tsk_null;
					const thttp_header_Sec_WebSocket_Accept_t* http_hdr_accept;
					tsk_ragel_state_t ragel_state;

					tsk_ragel_state_init(&ragel_state, pcData, (tsk_size_t)endOfMessage);
					if (thttp_message_parse(&ragel_state, &p_msg, tsk_false) != 0)
					{
						SC_DEBUG_ERROR_EX(kSCMobuleNameSignaling, "Failed to parse HTTP message: %.*s", endOfMessage, pcData);
						TSK_OBJECT_SAFE_FREE(p_msg);
						return const_cast<GmtSignaling*>(m_pcGmtSignaling)->raiseEvent(GmtSignalingEventType_NetError, "WebSocket handshaking: Failed to parse HTTP message");
					}
					if (!THTTP_MESSAGE_IS_RESPONSE(p_msg))
					{
						SC_DEBUG_ERROR_EX(kSCMobuleNameSignaling, "Incoming HTTP message not a response: %.*s", endOfMessage, pcData);
						TSK_OBJECT_SAFE_FREE(p_msg);
						return const_cast<GmtSignaling*>(m_pcGmtSignaling)->raiseEvent(GmtSignalingEventType_NetError, "WebSocket handshaking: Incoming HTTP message not a response");
					}
					if (p_msg->line.response.status_code > 299)
					{
						SC_DEBUG_ERROR_EX(kSCMobuleNameSignaling, "Incoming HTTP response is an error: %.*s", endOfMessage, pcData);
						TSK_OBJECT_SAFE_FREE(p_msg);
						return const_cast<GmtSignaling*>(m_pcGmtSignaling)->raiseEvent(GmtSignalingEventType_NetError, "WebSocket handshaking: Incoming HTTP response is an error");
					}
					// Get Accept header
					if (!(http_hdr_accept = (const thttp_header_Sec_WebSocket_Accept_t*)thttp_message_get_header(p_msg, thttp_htype_Sec_WebSocket_Accept)))
					{
						SC_DEBUG_ERROR_EX(kSCMobuleNameSignaling, "No 'Sec-WebSocket-Accept' header: %.*s", endOfMessage, pcData);
						TSK_OBJECT_SAFE_FREE(p_msg);
						return const_cast<GmtSignaling*>(m_pcGmtSignaling)->raiseEvent(GmtSignalingEventType_NetError, "WebSocket handshaking: No 'Sec-WebSocket-Accept' header");
					}
					// Authenticate the response
					{
						thttp_auth_ws_keystring_t resp = { 0 };
						thttp_auth_ws_response(oPeer->getWsKey(), &resp);
						if (!tsk_striequals(http_hdr_accept->value, resp)) {
							SC_DEBUG_ERROR_EX(kSCMobuleNameSignaling, "Authentication failed: %.*s", endOfMessage, pcData);
							TSK_OBJECT_SAFE_FREE(p_msg);
							return const_cast<GmtSignaling*>(m_pcGmtSignaling)->raiseEvent(GmtSignalingEventType_NetError, "WebSocket handshaking: Authentication failed");
						}
					}
					TSK_OBJECT_SAFE_FREE(p_msg);

					const_cast<GmtSignaling*>(m_pcGmtSignaling)->m_bWsHandshakingDone = true;
					nConsumedBytes = endOfMessage;
					return const_cast<GmtSignaling*>(m_pcGmtSignaling)->raiseEvent(GmtSignalingEventType_NetReady, "WebSocket handshaking: done");
				}
			}
		}
	}
	else {
		/* WebSocket raw data */
		const uint8_t* pcData = (const uint8_t*)oPeer->getDataPtr();
		const uint8_t opcode = pcData[0] & 0x0F;
		if ((pcData[0] & 0x01)/* FIN */) 
		{
			const uint8_t mask_flag = (pcData[1] >> 7); // Must be "1" for "client -> server"
			uint8_t mask_key[4] = { 0x00 };
			uint64_t pay_idx;
			uint64_t data_len = 0;
			uint64_t pay_len = 0;

			if (pcData[0] & 0x40 || pcData[0] & 0x20 || pcData[0] & 0x10) 
			{
				SC_DEBUG_ERROR_EX(kSCMobuleNameSignaling, "Unknown extension: %d", (pcData[0] >> 4) & 0x07);
				return const_cast<GmtSignaling*>(m_pcGmtSignaling)->raiseEvent(GmtSignalingEventType_NetReady, "WebSocket data: Unknown extension");
			}

			pay_len = pcData[1] & 0x7F;
			data_len = 2;

			if (pay_len == 126) 
			{
				if (oPeer->getDataSize() < 4) 
				{
					SC_DEBUG_INFO_EX(kSCMobuleNameSignaling, "No all data in the WS buffer");
					nConsumedBytes = 0;
					return true;
				}
				pay_len = (pcData[2] << 8 | pcData[3]);
				pcData = &pcData[4];
				data_len += 2;
			}
			else if (pay_len == 127)
			{
				if ((oPeer->getDataSize() - data_len) < 8)
				{
					SC_DEBUG_INFO_EX(kSCMobuleNameSignaling, "No all data in the WS buffer");
					nConsumedBytes = 0;
					return true;
				}
				pay_len = (((uint64_t)pcData[2]) << 56 | ((uint64_t)pcData[3]) << 48 | ((uint64_t)pcData[4]) << 40 | ((uint64_t)pcData[5]) << 32 | ((uint64_t)pcData[6]) << 24 | ((uint64_t)pcData[7]) << 16 | ((uint64_t)pcData[8]) << 8 || ((uint64_t)pcData[9]));
				pcData = &pcData[10];
				data_len += 8;
			}
			else 
			{
				pcData = &pcData[2];
			}

			if (mask_flag) 
			{
				// must be "true"
				if ((oPeer->getDataSize() - data_len) < 4) 
				{
					SC_DEBUG_INFO_EX(kSCMobuleNameSignaling, "No all data in the WS buffer");
					nConsumedBytes = 0;
					return true;
				}
				mask_key[0] = pcData[0];
				mask_key[1] = pcData[1];
				mask_key[2] = pcData[2];
				mask_key[3] = pcData[3];
				pcData = &pcData[4];
				data_len += 4;
			}

			if ((oPeer->getDataSize() - data_len) < pay_len) 
			{
				SC_DEBUG_INFO_EX(kSCMobuleNameSignaling, "No all data in the WS buffer");
				nConsumedBytes = 0;
				return true;
			}

			data_len += pay_len;
			uint8_t* _pcData = const_cast<uint8_t*>(pcData);

			// unmasking the payload
			if (mask_flag)
			{
				for (pay_idx = 0; pay_idx < pay_len; ++pay_idx) 
				{
					_pcData[pay_idx] = (pcData[pay_idx] ^ mask_key[(pay_idx & 3)]);
				}
			}
			return const_cast<GmtSignaling*>(m_pcGmtSignaling)->handleData((const char*)_pcData, (tsk_size_t)pay_len);
		}
		else if (opcode == 0x08)
		{
			// RFC6455 - 5.5.1.  Close
			SC_DEBUG_INFO_EX(kSCMobuleNameSignaling, "WebSocket opcode 0x8 (Close)");
			const_cast<GmtSignaling*>(m_pcGmtSignaling)->m_oNetTransport->close(oPeer->getFd());
		}
	}
	return true;
}

bool GmtSignalingTransportCallback::onConnectionStateChanged(std::shared_ptr<GmtNetPeer> oPeer)
{
	GmtAutoLock<GmtSignaling> autoLock(const_cast<GmtSignaling*>(m_pcGmtSignaling));

	if ((oPeer->getFd() == m_pcGmtSignaling->m_Fd || m_pcGmtSignaling->m_Fd == kGmtNetFdInvalid) && oPeer->isConnected())
	{
		const GmtWsTransport* pcTransport = dynamic_cast<const GmtWsTransport*>(m_pcGmtSignaling->m_oNetTransport.get());
		GMT_ASSERT(pcTransport != NULL);
		const_cast<GmtSignaling*>(m_pcGmtSignaling)->raiseEvent(GmtSignalingEventType_NetConnected, "Connected");
		if ((m_pcGmtSignaling->m_oConnectionUrl->getType() == GmtUrlType_WS || m_pcGmtSignaling->m_oConnectionUrl->getType() == GmtUrlType_WSS)) 
		{
			if (!m_pcGmtSignaling->m_bWsHandshakingDone)
			{
				return const_cast<GmtWsTransport*>(pcTransport)->handshaking(oPeer, const_cast<GmtSignaling*>(m_pcGmtSignaling)->m_oConnectionUrl);
			}
		}
		return const_cast<GmtSignaling*>(m_pcGmtSignaling)->raiseEvent(GmtSignalingEventType_NetReady, "Ready");
	}
	else if (m_pcGmtSignaling->m_Fd == oPeer->getFd() && !oPeer->isConnected()) 
	{
		const_cast<GmtSignaling*>(m_pcGmtSignaling)->m_Fd = kGmtNetFdInvalid;
		const_cast<GmtSignaling*>(m_pcGmtSignaling)->m_bWsHandshakingDone = false;
		return const_cast<GmtSignaling*>(m_pcGmtSignaling)->raiseEvent(GmtSignalingEventType_NetDisconnected, "Disconnected");
	}

	return true;
}

//
// GmtSignalingEvent
//

GmtSignalingEvent::GmtSignalingEvent(GmtSignalingEventType_t eType, std::string strDescription, const void* pcDataPtr /*= NULL*/, size_t nDataSize /*= 0*/)
	: m_eType(eType), m_strDescription(strDescription), m_pDataPtr(NULL), m_nDataSize(0)
{
	if (pcDataPtr && nDataSize)
	{
		if ((m_pDataPtr = tsk_malloc(nDataSize)))
		{
			memcpy(m_pDataPtr, pcDataPtr, nDataSize);
			m_nDataSize = nDataSize;
		}
	}
}

GmtSignalingEvent::~GmtSignalingEvent()
{
	TSK_FREE(m_pDataPtr);
}

//
//	SCCallEvent
//

GmtSignalingCallEvent::GmtSignalingCallEvent(std::string strDescription)
	: GmtSignalingEvent(GmtSignalingEventType_Call, strDescription)
{

}

GmtSignalingCallEvent::~GmtSignalingCallEvent()
{

}




static const HMODULE GmtGetCurrentModule()
{
	HMODULE hm = { 0 };
	GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCTSTR)GmtGetCurrentModule, &hm);
	return hm;
}

static const TCHAR* GmtGetCurrentDir()
{
	static TCHAR CURRENT_DIR_PATH[MAX_PATH] = { 0 };
	static DWORD CURRENT_DIR_PATH_LEN = 0;
	if (CURRENT_DIR_PATH_LEN == 0)
	{
		// NULL HMODULE will get the path to the executable not the DLL. When runing the code in Internet Explorer this is a BIG issue as the path is where IE.exe is installed.
		if ((CURRENT_DIR_PATH_LEN = GetModuleFileName(GmtGetCurrentModule(), CURRENT_DIR_PATH, sizeof(CURRENT_DIR_PATH))))
		{
			if (!PathRemoveFileSpec(CURRENT_DIR_PATH))
			{
				TSK_DEBUG_ERROR("PathRemoveFileSpec(%s) failed: %x", CURRENT_DIR_PATH, GetLastError());
				memset(CURRENT_DIR_PATH, 0, sizeof(CURRENT_DIR_PATH));
				CURRENT_DIR_PATH_LEN = 0;
			}
		}
		if (!CURRENT_DIR_PATH_LEN)
		{
			TSK_DEBUG_ERROR("GetModuleFileNameA() failed: %x", GetLastError());
		}
	}
	return CURRENT_DIR_PATH;
}

HRESULT GmtGetFullPath(LPWSTR szFileName, std::wstring & strFullPath)
{
	TCHAR szPath_[MAX_PATH];
	GMT_ASSERT(swprintf_s(szPath_, sizeof(szPath_) / sizeof(szPath_[0]), L"%s\\%s", GmtGetCurrentDir(), szFileName) > 0);
	strFullPath = std::wstring(szPath_);
	return S_OK;
}
