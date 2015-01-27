#pragma once

#include <atlbase.h>
#include <string>
#include <list>

#include "GothamSession.h"
#include "GothamSinkUtils.h"
#include "GothamMutex.h"
#include "GothamIceServer.h"
#include "GothamWebRTC.h"

typedef int GmRoType; // mapped to "enum tmedia_ro_type_e"
class GmProducer;

class GmSessionCall : public GmSession
{
	friend class GmAutoLock<GmSessionCall>;
	friend class GmObj;

private:
	GmSessionCall();
public:
	virtual ~GmSessionCall();

	HRESULT Start();
	HRESULT Pause();
	HRESULT Stop();

	HRESULT SetInputBitrate(UINT32 nBitrateKbps);
	HRESULT SetInputFPS(UINT32 nFps);
	HRESULT SetInputSize(UINT32 nWidth, UINT32 nHeight);
	HRESULT SetInputFormat(const GUID& subType);
	
	HRESULT AddIceServer(LPCWSTR wszServer);
	HRESULT SetIceCallback(IGmIceCallback* pCallback);
	HRESULT SetVideoDisplays(GmMediaType_t eVideoType, HWND displayLocal = NULL, HWND displayRemote = NULL);
	HRESULT GetLocalSDP(std::string &strSDP);
	HRESULT SetRemoteSDP(const std::string &strSDP, GmRoType_t eType = GmRoType_Unknown);

	HRESULT SetSSLPublic(const std::string & strPath);
	HRESULT SetSSLPrivate(const std::string & strPath);
	HRESULT SetSSLCA(const std::string & strPath);

	HRESULT SetSRTPMode(GmMode_t eMode);
	HRESULT SetSRTPType(UINT32 uType);
	HRESULT SetRTCWebProfileEnabled(bool bEnabled);
	HRESULT SetNattIceEnabled(bool bEnabled);
	HRESULT SetNattIceStunEnabled(bool bEnabled);
	HRESULT SetNattIceTurnEnabled(bool bEnabled);
	HRESULT SetRTCPEnabled(bool bEnabled);
	HRESULT SetRTCPMuxEnabled(bool bEnabled);
	HRESULT SetAVPFMode(GmMode_t eMode);
	HRESULT AddNattIceServer(std::string strTransportProto, std::string strServerHost, unsigned short serverPort, bool useTurn = false, bool useStun = true, std::string strUsername = "", std::string strPassword = "");
	HRESULT WriteSamples(const BYTE* pcBytesPtr, UINT32 nBytesCount);

	static CComPtr<GmSessionCall> New(const std::string &strRemoteSDP="");


private:
	void Lock();
	void Unlock();

	HRESULT Cleanup();

	HRESULT CreateSessionMgr();
	HRESULT CreateLocalOffer(const struct tsdp_message_s* pc_Ro = NULL, GmRoType eRoType = 0/*None*/);
	HRESULT AttachVideoDisplays();

	struct tnet_ice_ctx_s* IceCreateCtx(bool bVideo);
	HRESULT IceCreateCtxAll();
	HRESULT IceSetTimeout(INT32 timeout);
	bool IceGotLocalCandidates(struct tnet_ice_ctx_s *p_IceCtx);
	bool IceGotLocalCandidatesAll();
	HRESULT IceProcessRo(const struct tsdp_message_s* pc_SdpRo, bool isOffer);
	bool IceIsDone();
	bool IceIsEnabled(const struct tsdp_message_s* pc_Sdp);
	HRESULT IceStart();
	static int IceCallback(const struct tnet_ice_event_s *e);

private:
	GmProducer* m_pProducer;
	CComPtr<GmMutex> m_objMutex;
	CComPtr<IGmIceCallback> m_objIceCallback;
	bool m_bRTCWebProfileEnabled;
	bool m_bNattIceEnabled; // host candidates
	bool m_bNattIceStunEnabled; // reflexive candidates
	bool m_bNattIceTurnEnabled; // relay candidates
	GmMode_t m_eAVPFMode;
	GmMode_t m_eSRTPMode;
	UINT32 m_SRTPType;
	bool m_bRTCPEnabled;
	bool m_bRTCPMuxEnabled;

	bool m_bStarted;
	bool m_bStartDeferred;

	GUID m_guidInputFormat;
	UINT32 m_nInputBitrateKbps;
	UINT32 m_nInputWidth;
	UINT32 m_nInputHeight;
	UINT32 m_nFps;

	GmMediaType_t m_eMediaType;

	HWND m_VideoDisplayLocal;
	HWND m_VideoDisplayRemote;
	HWND m_ScreenCastDisplayLocal;
	HWND m_ScreenCastDisplayRemote;

	struct tnet_ice_ctx_s *m_pIceCtxVideo;
	struct tnet_ice_ctx_s *m_pIceCtxScreenCast;
	struct tnet_ice_ctx_s *m_pIceCtxAudio;

	struct tmedia_session_mgr_s* m_pSessionMgr;

	std::list<CComPtr<GmIceServer> > m_listIceServers;

	std::string m_strLocalSdpType;

	std::string m_strSSLPublic;
	std::string m_strSSLPrivate;
	std::string m_strSSLCA;
	bool m_bSSLMutualAuth;
};
