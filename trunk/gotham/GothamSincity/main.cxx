#include <windows.h>
#include <stdlib.h>
#include <tchar.h>
#include <assert.h>

// local headers to add SinCity signaling features
#include "GmtSincity.h"
#include "GmtJson.h"
#include "GmtUrl.h"

// API headers to add Media sink features
#include "GothamSinkGuids.h"
#include "GothamWebRTC.h"

// Microsoft MF API
#include <mfapi.h>
#include <mfidl.h>
#include <Mfreadwrite.h>
#include <mferror.h>

// ATL/COM
#include <comutil.h>
#include <AtlConv.h>
#include <atlexcept.h>

#include "tsk_debug.h"

#define GMT_WS_URL	"ws://ns313841.ovh.net:9000/wsStringStaticMulti?roomId=0"
#define GMT_LOCAL_IP NULL
#define LOCAL_PORT 0

#if !defined(ICE_STUN_UDP_ENABLED)
#	define ICE_STUN_UDP_ENABLED 1
#endif /* ICE_STUN_UDP_ENABLED */
#if !defined(ICE_TURN_UDP_ENABLED)
#	define ICE_TURN_UDP_ENABLED 0
#endif /* ICE_TURN_UDP_ENABLED */
#if !defined(ICE_TURN_TCP_ENABLED)
#	define ICE_TURN_TCP_ENABLED 0
#endif /* ICE_TURN_TCP_ENABLED */
#if !defined(ICE_TURN_TLS_ENABLED)
#	define ICE_TURN_TLS_ENABLED 0
#endif /*  */ICE_TURN_TLS_ENABLED

static const UINT32 VIDEO_WIDTH = 640;
static const UINT32 VIDEO_HEIGHT = 480;
static const UINT32 VIDEO_SIZE = ((VIDEO_WIDTH * VIDEO_HEIGHT * 3) >> 1); // NV12
static const UINT32 VIDEO_FPS = 25;
static const UINT32 VIDEO_BIT_RATE = 600000; // ~585kbps. ZERO to let the engine choose the best one

#define SSL_PATH_PUBLIC _T("SSL_Pub.pem")
#define SSL_PATH_PRIVATE _T("SSL_Priv.pem")
#define SSL_PATH_CA _T("SSL_CA.pem")

#define GMT_DEBUG_INFO(FMT, ...) TSK_DEBUG_INFO("[GOTHAM SINCITY TEST] " FMT, ##__VA_ARGS__)
#define GMT_DEBUG_WARN(FMT, ...) TSK_DEBUG_WARN("[GOTHAM SINCITY TEST] " FMT, ##__VA_ARGS__)
#define GMT_DEBUG_ERROR(FMT, ...) TSK_DEBUG_ERROR("[GOTHAM SINCITY TEST] " FMT, ##__VA_ARGS__)
#define GMT_DEBUG_FATAL(FMT, ...) TSK_DEBUG_FATAL("[GOTHAM SINCITY TEST] " FMT, ##__VA_ARGS__)
#define GMT_CHECK_HR(x) { HRESULT __hr__ = (x); if (FAILED(__hr__)) { GMT_DEBUG_ERROR("Operation Failed (%08x)", __hr__); goto bail; } }

#define GmtSafeRelease(ppT) \
{ \
    if (*ppT) \
			    { \
        (*ppT)->Release(); \
        *ppT = NULL; \
			    } \
}

enum
{
	GMT_MODE_NONE = 0,
	GMT_MODE_OPTIONAL,
	GMT_MODE_MANDATORY
};

enum
{
	GMT_SRTP_TYPE_NONE = 0x00,
	GMT_SRTP_TYPE_SDES = (0x01 << 0),
	GMT_SRTP_TYPE_DTLS = (0x01 << 1),
	GMT_SRTP_TYPE_BOTH = (GMT_SRTP_TYPE_SDES | GMT_SRTP_TYPE_DTLS)
};

static const struct {
	LPCWSTR protocol;
	bool enable_stun;
	bool enable_turn;
	LPCWSTR host;
	unsigned short port;
	LPCWSTR login;
	LPCWSTR password;
} GMT_ICE_SERVERS[] = {
#if ICE_STUN_UDP_ENABLED
	{ _T("udp"), true, false, _T("stun.l.google.com"), 19302, _T(""), _T("") },
#endif
#if ICE_TURN_UDP_ENABLED
	{ _T("udp"), true, true, _T("ns313841.ovh.net"), 3478, _T("ge.com"), _T("sincity") },
#endif
#if ICE_TURN_TCP_ENABLED
	{ _T("tcp"), true, true, _T("ns313841.ovh.net"), 3478, _T("ge.com"), _T("sincity") },
#endif
#if ICE_TURN_TLS_ENABLED
	{ _T("tls"), true, true, _T("ns313841.ovh.net"), 5349, _T("ge.com"), _T("sincity") },
#endif
};

class GmtSessionCall;
class GmtIceCallback : public IGmIceCallback
{
public:
	GmtIceCallback(const GmtSessionCall* pcCall);
	virtual ~GmtIceCallback();

	// IUnknown
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();
	STDMETHODIMP QueryInterface(REFIID iid, void** ppv);

	// IGmIceCallback
	STDMETHODIMP OnEvent(GM_ICE_EVENT_TYPE type);

private:
	long m_nRefCount;
	const GmtSessionCall* m_pcCall;
};

class GmtSessionCall
{
	friend class GmtAutoLock<GmtSessionCall>;
	friend class GmtIceCallback;
public:
	GmtSessionCall(std::shared_ptr<GmtSignaling> oSignaling, std::string strCallId = "")
		: m_signaling(oSignaling)
		, m_strCallId(strCallId)
		, m_bConnected(false)
		, m_pSinkWriter(NULL)
		, m_pSinkWriterEx(NULL)
		, m_pAttributes(NULL)
		, m_pMediaSink(NULL)
		, m_pMediaTypeIn(NULL)
		, m_pSample(NULL)
		, m_pBuffer(NULL)
	{
		InitializeCriticalSection(&m_critSec);

		if (m_strCallId.empty())
		{
			m_strCallId = randomString();
		}
		m_pIceCallback = new GmtIceCallback(this);
		GMT_ASSERT(m_pIceCallback != NULL);
	}

	virtual ~GmtSessionCall()
	{
		cleanup();
		GmtSafeRelease(&m_pIceCallback);
		DeleteCriticalSection(&m_critSec);
	}


	virtual bool call(std::string strDestUserId)
	{
		GmtAutoLock<GmtSessionCall> autoLock(this);
		bool delayed = false;

		if (!m_signaling->isReady())
		{
			GMT_DEBUG_ERROR("Signaling layer not ready yet");
			return false;
		}

		// Set local SDP type
		m_strLocalSdpType = "offer";

		// Update destination id
		m_strDestUserId = strDestUserId;

		if (!createSinkWriter())
		{
			return false;
		}

		return sendSdp(&delayed) || delayed;
	}

	virtual bool acceptEvent(std::shared_ptr<GmtSignalingCallEvent>& e)
	{
		GmtAutoLock<GmtSessionCall> autoLock(this);

		if (e->getCallId().compare(m_strCallId) != 0)
		{
			GMT_DEBUG_ERROR("CallId mismatch: '%s'<>'%s'", e->getCallId().c_str(), m_strCallId.c_str());
			return false;
		}

		if (e->getType() == "hangup")
		{
			return cleanup();
		}

		if (e->getType() == "ack")
		{
			GMT_DEBUG_ERROR("Not implemented yet");
			return false;
		}

		if (e->getType() == "offer" || e->getType() == "answer" || e->getType() == "pranswer")
		{
			bool pending = false;
			std::wstring wstrSDP;
			GMT_ASSERT(!e->getSdp().empty());
			HRESULT hr = convertStringWString(e->getSdp(), wstrSDP);
			if (FAILED(hr) || wstrSDP.empty())
			{
				GMT_ASSERT(false);
				return false;
			}
			if (!createSinkWriter())
			{
				return false;
			}
			hr = m_pAttributes->SetString(e->getType() == "pranswer" ? GM_PARAM_REMOTE_SDP_PRANSWER : (e->getType() == "answer" ? GM_PARAM_REMOTE_SDP_ANSWER : GM_PARAM_REMOTE_SDP_OFFER),
				wstrSDP.c_str());
			if (FAILED(hr))
			{
				GMT_ASSERT(false);
				return false;
			}
			if (m_strLocalSdpType.empty() || e->getType() == "offer")
			{
				m_strLocalSdpType = e->getType() == "offer" ? "answer" : "offer";
				return sendSdp(&pending) || pending;
			}
		}

		return true;
	}

	static int VP8ClipUV(int v)
	{
#define YUV_FIX 16
		v = (v + (257 << (YUV_FIX + 2 - 1))) >> (YUV_FIX + 2);
		return ((v & ~0xff) == 0) ? v : (v < 0) ? 0 : 255;
	}

	bool sendSample()
	{
		HRESULT hr = E_FAIL;

		GmtAutoLock<GmtSessionCall> autoLock(this);

		if (!m_bConnected)
		{
			GMT_DEBUG_INFO("Media not connected...do not send sample");
			return false;
		}

		if (m_pSinkWriter)
		{
			BYTE *pSamplesDataPtr_ = NULL;

			// Copy random data into the sample
			GMT_CHECK_HR(hr = m_pBuffer->Lock(&pSamplesDataPtr_, NULL, NULL));

			for (DWORD i = 0; i < VIDEO_SIZE; ++i)
			{
				if (i && i % 2 == 0)
				{
					pSamplesDataPtr_[i] = VP8ClipUV(pSamplesDataPtr_[i - 1]);
				}
				else if (i && i % 3 == 0)
				{
					pSamplesDataPtr_[i] = VP8ClipUV(pSamplesDataPtr_[i - 2]);
				}
				else
				{
					pSamplesDataPtr_[i] = rand() % 0xFF;
				}
			}
			
			GMT_CHECK_HR(hr = m_pBuffer->Unlock());
			GMT_CHECK_HR(hr = m_pBuffer->SetCurrentLength(VIDEO_SIZE));

			// Update start time
			GMT_CHECK_HR(hr = m_pSample->SetSampleTime(m_rtStart));
			// Send samples over the network
			GMT_CHECK_HR(hr = m_pSinkWriter->WriteSample(0, m_pSample));

			m_rtStart += m_rtDuration;
		}

	bail:
		return SUCCEEDED(hr);
	}

	static bool rejectEvent(std::shared_ptr<GmtSignaling> oSignaling, std::shared_ptr<GmtSignalingCallEvent>& e)
	{
		if (!oSignaling || !e)
		{
			GMT_DEBUG_ERROR("Invalid argument");
			return false;
		}

		if (e->getType() == "offer")
		{
			Json::Value message;

			message["type"] = "hangup";
			message["cid"] = e->getCallId();
			message["tid"] = randomString();
			message["from"] = e->getTo();
			message["to"] = e->getFrom();

			Json::StyledWriter writer;
			std::string output = writer.write(message);
			if (output.empty()) 
			{
				GMT_DEBUG_ERROR("Failed serialize JSON content");
				return false;
			}

			if (!oSignaling->sendData(output.c_str(), output.length()))
			{
				return false;
			}
		}
		else 
		{
			GMT_DEBUG_WARN("Event with type='%s' cannot be rejected", e->getType().c_str());
		}

		return true;
	}

	virtual bool hangup()
	{
		GmtAutoLock<GmtSessionCall> autoLock(this);

		Json::Value message;

		message["type"] = "hangup";
		message["cid"] = m_strCallId;
		message["tid"] = randomString();
		message["from"] = std::string(kGmtUserLocalId);
		message["to"] = m_strDestUserId;

		Json::StyledWriter writer;
		std::string output = writer.write(message);
		if (output.empty()) 
		{
			GMT_DEBUG_ERROR("Failed serialize JSON content");
			return false;
		}

		if (!m_signaling->sendData(output.c_str(), output.length())) 
		{
			return false;
		}

		cleanup();

		return true;
	}

	virtual _inline std::string getCallId()
	{
		GmtAutoLock<GmtSessionCall> autoLock(this);

		return m_strCallId;    /**< Gets the call identifier */
	}

	static std::shared_ptr<GmtSessionCall> newObj(std::shared_ptr<GmtSignaling> signalingSession)
	{
		if (!signalingSession) 
		{
			GMT_DEBUG_ERROR("Invalid argument");
			GMT_ASSERT(false);
			return NULL;
		}

		std::shared_ptr<GmtSessionCall> call = std::make_shared<GmtSessionCall>(signalingSession);
		GMT_ASSERT(call != NULL);
		return call;
	}

	static std::shared_ptr<GmtSessionCall> newObj(std::shared_ptr<GmtSignaling> signalingSession, std::shared_ptr<GmtSignalingCallEvent>& offer)
	{
		if (!signalingSession)
		{
			GMT_DEBUG_ERROR("Invalid argument");
			GMT_ASSERT(false);
			return NULL;
		}
		if (offer->getType() != "offer") 
		{
			GMT_DEBUG_ERROR("Call event with type='%s' cannot be used to create a call session", offer->getType().c_str());
			GMT_ASSERT(false);
			return NULL;
		}

		std::shared_ptr<GmtSessionCall> call = std::make_shared<GmtSessionCall>(signalingSession, offer->getCallId());
		GMT_ASSERT(call != NULL);
		call->m_strDestUserId = offer->getFrom();
		return call;
	}

private:
	void lock()
	{
		EnterCriticalSection(&m_critSec);
	}

	void unlock()
	{
		LeaveCriticalSection(&m_critSec);
	}

	bool start()
	{
		GmtAutoLock<GmtSessionCall> autoLock(this);

		HRESULT hr = E_FAIL;
		if (m_pSinkWriter)
		{
			GMT_CHECK_HR(hr = m_pSinkWriter->BeginWriting()); // required! - will start media streaming
		}
		bail:
		return SUCCEEDED(hr);
	}

	bool stop()
	{
		GmtAutoLock<GmtSessionCall> autoLock(this);

		if (m_pMediaSink)
		{
			m_pMediaSink->Shutdown();
		}
		return true;
	}

	bool cleanup()
	{
		GmtAutoLock<GmtSessionCall> autoLock(this);

		stop();
		if (m_pAttributes)
		{
			m_pAttributes->SetUnknown(GM_PARAM_ICE_CALLBACK, NULL);
		}

		GmtSafeRelease(&m_pSinkWriter);
		GmtSafeRelease(&m_pSinkWriterEx);
		GmtSafeRelease(&m_pAttributes);
		GmtSafeRelease(&m_pMediaSink);
		GmtSafeRelease(&m_pMediaTypeIn);
		GmtSafeRelease(&m_pSample);
		GmtSafeRelease(&m_pBuffer);

		m_strLocalSdpType = "";

		return true;
	}

	static std::string randomString()
	{
		GUID guid;
		GMT_ASSERT(SUCCEEDED(CoCreateGuid(&guid)));
		char guidString[37];
		sprintf_s(
			guidString,
			"%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX",
			guid.Data1, guid.Data2, guid.Data3,
			guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
			guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
		return std::string(guidString);
	}
	static HRESULT convertStringWString(const std::string & in, std::wstring &out)
	{
		USES_CONVERSION_EX;
		const wchar_t* _out = A2W_EX(in.c_str(), in.length());
		out = std::wstring(_out);
		return S_OK;
	}

	static HRESULT convertWStringString(const std::wstring & in, std::string &out)
	{
		USES_CONVERSION_EX;
		const char* _out = W2A_EX(in.c_str(), in.length());
		out = std::string(_out);
		return S_OK;
	}

	bool sendSdp(bool *delayed = NULL)
	{
		GmtAutoLock<GmtSessionCall> autoLock(this);

		Json::Value message;

		std::string stringLocalSDP_;
		TCHAR szLocalSDP_[4092];
		UINT32 uint32_ = uint32_ = 0;
		HRESULT hr;
		
		hr = m_pAttributes->GetString(GM_PARAM_LOCAL_SDP, szLocalSDP_, sizeof(szLocalSDP_) / sizeof(szLocalSDP_[0]), &uint32_); // Will start ICE process if not already done
		if (delayed)
		{
			*delayed = (hr == E_PENDING);
		}
		if (delayed && *delayed)
		{
			// SDP not ready yet -> we have to retry later
			return false;
		}

		if (FAILED(hr))
		{
			GMT_ASSERT(false);
			return false;
		}
		GMT_ASSERT(uint32_ < (sizeof(szLocalSDP_) / sizeof(szLocalSDP_[0])));
		hr = convertWStringString(szLocalSDP_, stringLocalSDP_);
		if (FAILED(hr))
		{
			GMT_ASSERT(false);
			return false;
		}


		// Compute when transaction identifier for the offer
		m_strTidOffer = randomString();

		message["type"] = m_strLocalSdpType.empty() ? "offer" : m_strLocalSdpType;
		message["cid"] = m_strCallId;
		message["tid"] = m_strTidOffer.empty() ? randomString() : m_strTidOffer;
		message["from"] = std::string(kGmtUserLocalId);
		message["to"] = m_strDestUserId;
		message["sdp"] = stringLocalSDP_;

		Json::StyledWriter writer;
		std::string output = writer.write(message);
		if (output.empty()) 
		{
			GMT_DEBUG_ERROR("Failed serialize JSON content");
			return false;
		}

		GMT_DEBUG_INFO("Sending...:%.*s", output.length(), output.c_str());

		if (!m_signaling->sendData(output.c_str(), output.length())) 
		{
			return false;
		}
		return true;
	}

	bool createSinkWriter()
	{
		GmtAutoLock<GmtSessionCall> autoLock(this);

		HRESULT hr = S_OK;
		std::wstring wstrSSLPathPub, wstrSSLPathPriv, wstrSSLPathCA;
		TCHAR szIceServer_[4092];
		extern HRESULT CGmSink_CreateInstance(REFIID iid, void **ppMFT);
		extern HRESULT CreateGmSink(IMFMediaSink **ppSink);

		if (m_pSinkWriter)
		{
			return true;
		}

		/* Create MediaSink and SinkWriter */
		GMT_CHECK_HR(hr = CGmSink_CreateInstance(IID_IMFMediaSink, (void**)&m_pMediaSink));
		GMT_CHECK_HR(hr = MFCreateAttributes(&m_pAttributes, 1));
		GMT_CHECK_HR(hr = m_pAttributes->SetUINT32(MF_SINK_WRITER_DISABLE_THROTTLING, TRUE));
		GMT_CHECK_HR(hr = MFCreateSinkWriterFromMediaSink(m_pMediaSink, m_pAttributes, &m_pSinkWriter));
		GmtSafeRelease(&m_pAttributes);
		GMT_CHECK_HR(hr = m_pMediaSink->QueryInterface(&m_pAttributes));
		hr = m_pSinkWriter->QueryInterface(&m_pSinkWriterEx); // Win8+ only
		if (SUCCEEDED(hr))
		{
			GMT_DEBUG_INFO("Got pSinkWriterEx!");
		}

		/* Set MediaTypes */
		GMT_CHECK_HR(hr = MFCreateMediaType(&m_pMediaTypeIn));
		GMT_CHECK_HR(hr = m_pMediaTypeIn->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
		GMT_CHECK_HR(hr = m_pMediaTypeIn->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12));
		GMT_CHECK_HR(hr = m_pMediaTypeIn->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE));
		GMT_CHECK_HR(hr = m_pMediaTypeIn->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive));
		GMT_CHECK_HR(hr = MFSetAttributeSize(m_pMediaTypeIn, MF_MT_FRAME_SIZE, VIDEO_WIDTH, VIDEO_HEIGHT));
		GMT_CHECK_HR(hr = m_pMediaTypeIn->SetUINT32(MF_MT_AVG_BITRATE, VIDEO_BIT_RATE));
		GMT_CHECK_HR(hr = MFSetAttributeRatio(m_pMediaTypeIn, MF_MT_FRAME_RATE, VIDEO_FPS, 1));
		GMT_CHECK_HR(hr = MFSetAttributeRatio(m_pMediaTypeIn, MF_MT_PIXEL_ASPECT_RATIO, 1, 1));
		GMT_CHECK_HR(hr = m_pSinkWriter->SetInputMediaType(0, m_pMediaTypeIn, NULL));

		/* Create Sample */
		m_rtStart = 0;
		GMT_CHECK_HR(hr = MFFrameRateToAverageTimePerFrame(VIDEO_FPS, 1, &m_rtDuration));
		GMT_CHECK_HR(hr = MFCreateSample(&m_pSample));
		GMT_CHECK_HR(hr = MFCreateMemoryBuffer(VIDEO_SIZE, &m_pBuffer));
		GMT_CHECK_HR(hr = m_pSample->AddBuffer(m_pBuffer));
		GMT_CHECK_HR(hr = m_pSample->SetSampleDuration(m_rtDuration));

		// SSL certificate full paths
		GMT_CHECK_HR(hr = GmtGetFullPath(SSL_PATH_PUBLIC, wstrSSLPathPub));
		GMT_CHECK_HR(hr = GmtGetFullPath(SSL_PATH_PRIVATE, wstrSSLPathPriv));
		GMT_CHECK_HR(hr = GmtGetFullPath(SSL_PATH_CA, wstrSSLPathCA));

		// Configure the MediaSink
		GMT_CHECK_HR(hr = m_pAttributes->SetUINT32(GM_PARAM_RTCWEB_PROFILE_ENABLED, TRUE));
		GMT_CHECK_HR(hr = m_pAttributes->SetUINT32(GM_PARAM_SRTP_MODE, GMT_MODE_MANDATORY));
		GMT_CHECK_HR(hr = m_pAttributes->SetUINT32(GM_PARAM_SRTP_TYPE, GMT_SRTP_TYPE_DTLS));
		GMT_CHECK_HR(hr = m_pAttributes->SetUINT32(GM_PARAM_AVPF_MODE, GMT_MODE_MANDATORY));
		GMT_CHECK_HR(hr = m_pAttributes->SetString(GM_PARAM_SSL_PATH_PUBLIC, wstrSSLPathPub.c_str()));
		GMT_CHECK_HR(hr = m_pAttributes->SetString(GM_PARAM_SSL_PATH_PRIVATE, wstrSSLPathPriv.c_str()));
		GMT_CHECK_HR(hr = m_pAttributes->SetString(GM_PARAM_SSL_PATH_CA, wstrSSLPathCA.c_str()));
		GMT_CHECK_HR(hr = m_pAttributes->SetUINT32(GM_PARAM_ICE_ENABLED, TRUE));
		GMT_CHECK_HR(hr = m_pAttributes->SetUINT32(GM_PARAM_ICE_STUN_ENABLED, TRUE));
		GMT_CHECK_HR(hr = m_pAttributes->SetUINT32(GM_PARAM_ICE_TURN_ENABLED, TRUE));
		GMT_CHECK_HR(hr = m_pAttributes->SetUnknown(GM_PARAM_ICE_CALLBACK, m_pIceCallback));
		GMT_CHECK_HR(hr = m_pAttributes->SetUINT32(GM_PARAM_RTCP_ENABLED, TRUE));
		GMT_CHECK_HR(hr = m_pAttributes->SetUINT32(GM_PARAM_RTCPMUX_ENABLED, TRUE));
		for (size_t i = 0; i < sizeof(GMT_ICE_SERVERS) / sizeof(GMT_ICE_SERVERS[0]); ++i)
		{
			GMT_CHECK_HR(swprintf_s(szIceServer_, sizeof(szIceServer_) / sizeof(szIceServer_[0]),
				_T("protocol=%ls;enable_stun=%ls;enable_turn=%ls;host=%ls;port=%hu;login=%ls;password=%ls"),
				GMT_ICE_SERVERS[i].protocol,
				GMT_ICE_SERVERS[i].enable_stun ? _T("true") : _T("false"),
				GMT_ICE_SERVERS[i].enable_turn ? _T("true") : _T("false"),
				GMT_ICE_SERVERS[i].host,
				GMT_ICE_SERVERS[i].port,
				GMT_ICE_SERVERS[i].login,
				GMT_ICE_SERVERS[i].password) != EOF);
			GMT_CHECK_HR(hr = m_pAttributes->SetString(GM_PARAM_ICE_SERVER, szIceServer_));
		}
		
	bail:
		if (FAILED(hr))
		{
			cleanup();
			return false;
		}
		return true;
	}

private:
	bool m_bConnected;
	std::shared_ptr<GmtSignaling> m_signaling;
	GmtIceCallback* m_pIceCallback;
	CRITICAL_SECTION m_critSec;

	std::string m_strDestUserId;
	std::string m_strCallId;
	std::string m_strTidOffer;
	std::string m_strLocalSdpType;

	LONGLONG m_rtStart;
	UINT64 m_rtDuration;

	IMFSinkWriter *m_pSinkWriter;
	IMFSinkWriterEx *m_pSinkWriterEx;
	IMFAttributes *m_pAttributes;
	IMFMediaSink *m_pMediaSink;
	IMFMediaType *m_pMediaTypeIn;
	IMFSample *m_pSample;
	IMFMediaBuffer *m_pBuffer;
};


GmtIceCallback::GmtIceCallback(const GmtSessionCall* pcCall)
	: m_nRefCount(1)
	, m_pcCall(pcCall)
{
	GMT_ASSERT(m_pcCall != NULL);
}
GmtIceCallback::~GmtIceCallback()
{
	GMT_ASSERT(m_nRefCount == 0);
}

// IUnknown
STDMETHODIMP_(ULONG) GmtIceCallback::AddRef()
{
	return InterlockedIncrement(&m_nRefCount);
}

STDMETHODIMP_(ULONG) GmtIceCallback::Release()
{
	ULONG uCount = InterlockedDecrement(&m_nRefCount);
	if (uCount == 0)
	{
		delete this;
	}
	return uCount;
}

STDMETHODIMP GmtIceCallback::QueryInterface(REFIID iid, void** ppv)
{
	if (!ppv)
	{
		return E_POINTER;
	}
	if (iid == IID_IUnknown)
	{
		*ppv = static_cast<IUnknown*>(static_cast<IUnknown*>(this));
	}
	else if (iid == __uuidof(IGmIceCallback))
	{
		*ppv = static_cast<IGmIceCallback*>(this);
	}
	else
	{
		*ppv = NULL;
		return E_NOINTERFACE;
	}
	AddRef();
	return S_OK;
}

STDMETHODIMP GmtIceCallback::OnEvent(GM_ICE_EVENT_TYPE type)
{
	GmtAutoLock<GmtSessionCall> autoLock(const_cast<GmtSessionCall*>(m_pcCall)); // ***Must***

	switch (type) {
	case GM_ICE_EVENT_TYPE_GATHERING_DONE:
		const_cast<GmtSessionCall*>(m_pcCall)->sendSdp();
		break;
	case GM_ICE_EVENT_TYPE_CONNECTED:
		const_cast<GmtSessionCall*>(m_pcCall)->start();
		const_cast<GmtSessionCall*>(m_pcCall)->m_bConnected = true;
		break;
	case GM_ICE_EVENT_TYPE_DISCONNECTED:
		const_cast<GmtSessionCall*>(m_pcCall)->m_bConnected = false;
		const_cast<GmtSessionCall*>(m_pcCall)->hangup();
		break;
	case GM_ICE_EVENT_TYPE_ERROR:
		const_cast<GmtSessionCall*>(m_pcCall)->m_bConnected = false;
		const_cast<GmtSessionCall*>(m_pcCall)->hangup();
		break;
	}
	return S_OK;
}

static bool isSignalingReady = false;
static std::shared_ptr<GmtSignaling> signalSession;
static std::shared_ptr<GmtSignalingCallEvent> pendingOffer;
static std::shared_ptr<GmtSessionCall> callSession;

class GmtSignalingCallbackDummy : public GmtSignalingCallback
{	
public:
	GmtSignalingCallbackDummy() {

	}
	virtual ~GmtSignalingCallbackDummy() {

	}

	virtual bool onEventNet(std::shared_ptr<GmtSignalingEvent>& e) {
		//!\Deadlock issue: You must not call any function from 'GmtSignaling' class unless you fork a new thread.
		switch (e->getType()) {
		case GmtSignalingEventType_NetReady: {
			isSignalingReady = true;
			GMT_DEBUG_INFO("***Signaling module connected ***");
			break;
		}
		case GmtSignalingEventType_NetDisconnected:
		case GmtSignalingEventType_NetError: {
			isSignalingReady = false;
			GMT_DEBUG_INFO("***Signaling module disconnected ***");
			break;
		}
		case GmtSignalingEventType_NetData: {
			GMT_DEBUG_INFO("***Signaling module passthrough DATA:%.*s ***", e->getDataSize(), (const char*)e->getDataPtr());
			break;
		}
		}

		return true;
	}
	virtual bool onEventCall(std::shared_ptr<GmtSignalingCallEvent>& e) {
		//!\Deadlock issue: You must not call any function from 'GmtSignaling' class unless you fork a new thread.
		if (callSession) 
		{
			if (callSession->getCallId() != e->getCallId())
			{
				GMT_DEBUG_ERROR("Call id mismatch: '%s'<>'%s'", callSession->getCallId().c_str(), e->getCallId().c_str());
				return GmtSessionCall::rejectEvent(signalSession, e);
			}
			bool ret = callSession->acceptEvent(e);
			if (e->getType() == "hangup")
			{
				callSession = NULL;
				GMT_DEBUG_INFO("+++Call ended +++");
			}
			return ret;
		}
		else 
		{
			if (e->getType() == "offer") 
			{
				if (callSession || pendingOffer) 
				{ // already in call?
					return GmtSessionCall::rejectEvent(signalSession, e);
				}
				pendingOffer = e;
				GMT_DEBUG_INFO("+++Incoming call: 'accept'/'reject'? +++");
			}
			if (e->getType() == "hangup")
			{
				if (pendingOffer && pendingOffer->getCallId() == e->getCallId())
				{
					pendingOffer = NULL;
					GMT_DEBUG_INFO("+++ pending call cancelled +++");
				}
			}

			// Silently ignore any other event type
		}

		return true;
	}
};

static void printHelp()
{
	printf("----------------------------------------------------------------------------\n"
		"                               COMMANDS                                     \n"
		"----------------------------------------------------------------------------\n"
		"\"help\"              Prints this message\n"
		"\"call [dest]\"       Video call to \"dest\" (opt., default is '2')\n"
		"\"sample\"            Sends a video sample\n"
		"\"hangup\"            Terminates the active call\n"
		"\"accept\"            Accepts the incoming call\n"
		"\"reject\"            Rejects the incoming call\n"
		"\"quit\"              Teminates the application\n"
		"\n\n"
		"*** Press ENTER with empty input to send video sample ***\n"
		"--------------------------------------------------------------------------\n\n"
		);
}


int _tmain(int argc, _TCHAR* argv[], _TCHAR* envp[])
{
	HRESULT hr = S_OK;
	char command[1024] = { 0 };
	char remoteId[25];

	printf("*******************************************************************\n"
		"Copyright (C) 2015 Doubango Telecom (VoIP division)\n"
		"PRODUCT: GOTHAM SINK\n"
		"HOME PAGE: <void>\n"
		"CODE SOURCE: <void>\n"
		"LICENCE: <void>\n"
		"VERSION: %s\n"
		"'quit' to quit the application.\n"
		"*******************************************************************\n\n"
		, "1.0");


	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (hr == 0x80010106) // 0x80010106 when called from managed code (e.g. Boghe) - More info: http://support.microsoft.com/kb/824480
	{
		hr = S_OK;
	}
	GMT_CHECK_HR(hr);
	GMT_CHECK_HR(hr = MFStartup(MF_VERSION));

	signalSession = GmtSignaling::newObj(GMT_WS_URL, GMT_LOCAL_IP, LOCAL_PORT);
	GMT_ASSERT(signalSession != NULL);
	signalSession->setCallback(std::make_shared<GmtSignalingCallbackDummy>());

	if (signalSession->connect() != true)
	{
		GMT_CHECK_HR(hr = E_FAIL);
	}


	printHelp();

	while (fgets(command, sizeof(command), stdin) != NULL) 
	{
#define CHECK_CONNECTED() if (!isSignalingReady){ GMT_DEBUG_INFO("+++ not connected yet +++"); continue; }
		if (strnicmp(command, "quit", 4) == 0) 
		{
			GMT_DEBUG_INFO("+++ quit() +++");
			break;
		}
		else if (strnicmp(command, "help", 4) == 0) {
			GMT_DEBUG_INFO("+++ help() +++");
			printHelp();
		}
		else if (strnicmp(command, "call", 4) == 0 || strnicmp(command, "video", 5) == 0)
		{
			std::string strRemoteId = std::string(kGmtUserRemoteId);
			CHECK_CONNECTED();
			if (callSession) 
			{
				GMT_DEBUG_INFO("+++ already on call +++");
				continue;
			}
			if (sscanf(command, "%*s %24s", remoteId) > 0 && strlen(remoteId) > 0) 
			{
				strRemoteId = std::string(remoteId);
			}
			GMT_DEBUG_INFO("+++ call('%s') +++", strRemoteId.c_str());
			GMT_ASSERT((callSession = GmtSessionCall::newObj(signalSession)) != NULL);
			GMT_ASSERT(callSession->call(strRemoteId) == true);
		}
		else if (strnicmp(command, "hangup", 6) == 0 || strnicmp(command, "reject", 6) == 0)
		{
			CHECK_CONNECTED();
			if (callSession) 
			{
				GMT_DEBUG_INFO("+++ hangup() +++");
				GMT_ASSERT(callSession->hangup());
				callSession = nullptr;
			}
			else if (pendingOffer)
			{
				GMT_DEBUG_INFO("+++ reject() +++");
				GMT_ASSERT(GmtSessionCall::rejectEvent(signalSession, pendingOffer) == true);
				pendingOffer = nullptr;
			}
		}
		else if (strnicmp(command, "accept", 6) == 0)
		{
			CHECK_CONNECTED();
			if (!callSession && pendingOffer)
			{
				GMT_DEBUG_INFO("+++ accept() +++");
				GMT_ASSERT((callSession = GmtSessionCall::newObj(signalSession, pendingOffer)) != NULL);
				GMT_ASSERT(callSession->acceptEvent(pendingOffer) == true);
				pendingOffer = nullptr;
			}
		}
		else if (strnicmp(command, "sample", 6) == 0 || 1)
		{
			bool sent = false;
			if (callSession)
			{
				sent = callSession->sendSample();
			}
			GMT_DEBUG_INFO("sample sent? --> %s", sent ? "true" : "false");
		}
	}

	if (callSession)
	{
		GMT_ASSERT(callSession->hangup());
		callSession = nullptr;
	}
	signalSession = nullptr;


bail:
	MFShutdown();
	CoUninitialize();

	return SUCCEEDED(hr) ? 0 : -1;
}

