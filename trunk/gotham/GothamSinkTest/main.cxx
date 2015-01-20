#include <windows.h>
#include <stdlib.h>
#include <tchar.h>

#include <mfapi.h>
#include <mfidl.h>
#include <Mfreadwrite.h>
#include <mferror.h>

#include "GothamSinkGuids.h" // GUIDs definitions. ONLY this header need to be included in your app

#include "tsk_debug.h" // For debug message. NOT required to include this header

enum
{
	GMT_MODE_NONE = 0,
	GMT_MODE_OPTIONAL,
	GMT_MODE_MANDATORY
};

static const UINT32 VIDEO_WIDTH = 640;
static const UINT32 VIDEO_HEIGHT = 480;
static const UINT32 VIDEO_FPS = 25;
static const UINT32 VIDEO_BIT_RATE = 800000;
static const UINT32 VIDEO_LOCAL_PORT = 0; // binds to any available port
static const UINT32 VIDEO_FRAMES_COUNT = 10; // number of frames to send for testing

extern HRESULT CGmSink_CreateInstance(REFIID iid, void **ppMFT);
extern HRESULT CreateGmSink(IMFMediaSink **ppSink);

#define GMT_DEBUG_INFO(FMT, ...) TSK_DEBUG_INFO("[GOTHAM SINK TEST] " FMT, ##__VA_ARGS__)
#define GMT_DEBUG_WARN(FMT, ...) TSK_DEBUG_WARN("[GOTHAM SINK TEST] " FMT, ##__VA_ARGS__)
#define GMT_DEBUG_ERROR(FMT, ...) TSK_DEBUG_ERROR("[GOTHAM SINK TEST] " FMT, ##__VA_ARGS__)
#define GMT_DEBUG_FATAL(FMT, ...) TSK_DEBUG_FATAL("[GOTHAM SINK TEST] " FMT, ##__VA_ARGS__)
#define GMT_CHECK_HR(x) { HRESULT __hr__ = (x); if (FAILED(__hr__)) { GMT_DEBUG_ERROR("Operation Failed (%08x)", __hr__); goto bail; } }

#define GmtSafeRelease(ppT) \
{ \
    if (*ppT) \
		    { \
        (*ppT)->Release(); \
        *ppT = NULL; \
		    } \
}

#if !defined(GMT_WANT_TO_TALK_TO_BROWSER) // Generate SDP compatible with WebRTC (Chrome, FF, Safari, IE...)
#	define GMT_WANT_TO_TALK_TO_BROWSER		0
#endif

#if !defined(GMT_LOOPBACK)
#	define GMT_LOOPBACK		0 
#endif

#define GMT_IP_REMOTE				_T("192.168.0.28")
#define GMT_PORT_REMOTE_VIDEO		_T("1601")

#define GMT_REMOTE_SDP _T("v=0\r\n") \
	_T("o=doubango 1983 678901 IN IP4 ") GMT_IP_REMOTE _T("\r\n") \
	_T("s=-\r\n") \
	_T("c=IN IP4 ") GMT_IP_REMOTE _T("\r\n") \
	_T("t=0 0\r\n") \
	_T("a=tcap:1 RTP/AVPF\r\n") \
	_T("m=video ") GMT_PORT_REMOTE_VIDEO _T(" RTP/AVP 104 100\r\n") \
	_T("a=rtcp-fb:* ccm fir\r\n") \
	_T("a=rtcp-fb:* nack\r\n") \
	_T("a=rtcp-fb:* goog-remb\r\n") \
	_T("a=label:1\r\n") \
	_T("a=content:main\r\n") \
	_T("a=rtpmap:104 H264/90000\r\n") \
	_T("a=imageattr:104 recv [x=[128:16:640],y=[96:16:480]] send [x=[128:16:640],y=[96:16:480]]\r\n") \
	_T("a=fmtp:104 profile-level-id=4d8016; max-mbps=20250; max-fs=1200; packetization-mode=1; impl=intel\r\n") \
	_T("a=rtpmap:100 VP8/90000\r\n") \
	_T("a=imageattr:100 recv [x=[128:16:640],y=[96:16:480]] send [x=[128:16:640],y=[96:16:480]]\r\n") \
	_T("a=pcfg:1 t=1\r\n") \
	_T("a=sendrecv\r\n") \
	_T("a=rtcp-mux\r\n") \
	_T("a=ssrc:82176777 cname:38e8cc681dd07cff0753f07bcb73b2d4\r\n") \
	_T("a=ssrc:82176777 mslabel:6994f7d1-6ce9-4fbd-acfd-84e5131ca2e2\r\n") \
	_T("a=ssrc:82176777 label:doubango@video\r\n")


#if GMT_WANT_TO_TALK_TO_BROWSER // Remote party = Chrome, FF, IE, Safari or Opera
	/* YOU MUST NOT CHANGE NEXT VALUES UNLESS YOU NOW WHAT YOUR'RE DOING */
	static const BOOL PARAM_ICE_ENABLED = TRUE;
	static const BOOL PARAM_ICE_STUN_ENABLED = TRUE;
	static const BOOL PARAM_ICE_TURN_ENABLED = FALSE;

	static const BOOL PARAM_RTCP_ENABLED = TRUE;
	static const BOOL PARAM_RTCPMUX_ENABLED = TRUE;

	static const UINT32 PARAM_AVPF_MODE = GMT_MODE_MANDATORY;
#else
	/* YOU CAN CHANGE NEXT VALUES FOR TESTING */
	static const BOOL PARAM_ICE_ENABLED = FALSE;
	static const BOOL PARAM_ICE_STUN_ENABLED = TRUE;
	static const BOOL PARAM_ICE_TURN_ENABLED = FALSE;

	static const BOOL PARAM_RTCP_ENABLED = TRUE;
	static const BOOL PARAM_RTCPMUX_ENABLED = TRUE;

	static const UINT32 PARAM_AVPF_MODE = GMT_MODE_OPTIONAL;
#endif /* GMT_WANT_TO_TALK_TO_BROWSER */


int _tmain(int argc, _TCHAR* argv[], _TCHAR* envp[])
{
	HRESULT hr = S_OK;
	IMFSinkWriter *pSinkWriter = NULL;
	IMFSinkWriterEx *pSinkWriterEx = NULL;
	IMFAttributes *pAttributes = NULL;
	IMFMediaSink *pMediaSink = NULL;
	IMFMediaType *pMediaTypeIn = NULL;
	IMFSample *pSample = NULL;
	IMFMediaBuffer *pBuffer = NULL;

	LONGLONG rtStart_ = 0;
	UINT64 rtDuration_;
	DWORD samplesCount_ = ((VIDEO_WIDTH * VIDEO_HEIGHT * 3) >> 1); // NV12 size
	BYTE *pSamplesDataPtr_ = NULL;

	UINT32 frames_count_ = 0;
	UINT32 uint32_;
	TCHAR szLocalSDP_[2048];

	uint32_;
	szLocalSDP_;

	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (hr == 0x80010106) // 0x80010106 when called from managed code (e.g. Boghe) - More info: http://support.microsoft.com/kb/824480
	{
		hr = S_OK;
	}
	GMT_CHECK_HR(hr);
	GMT_CHECK_HR(hr = MFStartup(MF_VERSION));
	
	/* Create MediaSink and SinkWriter */
	GMT_CHECK_HR(hr = CGmSink_CreateInstance(IID_IMFMediaSink, (void**)&pMediaSink));
	GMT_CHECK_HR(hr = MFCreateAttributes(&pAttributes, 1));
	GMT_CHECK_HR(hr = pAttributes->SetUINT32(MF_SINK_WRITER_DISABLE_THROTTLING, TRUE));
	GMT_CHECK_HR(hr = MFCreateSinkWriterFromMediaSink(pMediaSink, pAttributes, &pSinkWriter));
	GmtSafeRelease(&pAttributes);
	GMT_CHECK_HR(hr = pMediaSink->QueryInterface(&pAttributes));
	hr = pSinkWriter->QueryInterface(&pSinkWriterEx); // Win8+ only
	if (SUCCEEDED(hr))
	{
		GMT_DEBUG_INFO("Got pSinkWriterEx!");
	}

	/* Set MediaTypes */
	GMT_CHECK_HR(hr = MFCreateMediaType(&pMediaTypeIn));
	GMT_CHECK_HR(hr = pMediaTypeIn->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
	GMT_CHECK_HR(hr = pMediaTypeIn->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12));
	GMT_CHECK_HR(hr = pMediaTypeIn->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE));
	GMT_CHECK_HR(hr = pMediaTypeIn->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive));
	GMT_CHECK_HR(hr = MFSetAttributeSize(pMediaTypeIn, MF_MT_FRAME_SIZE, VIDEO_WIDTH, VIDEO_HEIGHT));
	GMT_CHECK_HR(hr = pMediaTypeIn->SetUINT32(MF_MT_AVG_BITRATE, VIDEO_BIT_RATE));
	GMT_CHECK_HR(hr = MFSetAttributeRatio(pMediaTypeIn, MF_MT_FRAME_RATE, VIDEO_FPS, 1));
	GMT_CHECK_HR(hr = MFSetAttributeRatio(pMediaTypeIn, MF_MT_PIXEL_ASPECT_RATIO, 1, 1));
	GMT_CHECK_HR(hr = pSinkWriter->SetInputMediaType(0, pMediaTypeIn, NULL));

	/* Create Sample */
	GMT_CHECK_HR(hr = MFFrameRateToAverageTimePerFrame(VIDEO_FPS, 1, &rtDuration_));
	GMT_CHECK_HR(hr = MFCreateSample(&pSample));
	GMT_CHECK_HR(hr = MFCreateMemoryBuffer(samplesCount_, &pBuffer));
	GMT_CHECK_HR(hr = pSample->AddBuffer(pBuffer));
	GMT_CHECK_HR(hr = pSample->SetSampleDuration(rtDuration_));

	// Configure the MediaSink
	GMT_CHECK_HR(hr = pAttributes->SetUINT32(GM_PARAM_AVPF_MODE, PARAM_AVPF_MODE));
	GMT_CHECK_HR(hr = pAttributes->SetUINT32(GM_PARAM_ICE_ENABLED, PARAM_ICE_ENABLED));
	GMT_CHECK_HR(hr = pAttributes->SetUINT32(GM_PARAM_ICE_STUN_ENABLED, PARAM_ICE_STUN_ENABLED));
	GMT_CHECK_HR(hr = pAttributes->SetUINT32(GM_PARAM_ICE_TURN_ENABLED, PARAM_ICE_TURN_ENABLED));
	GMT_CHECK_HR(hr = pAttributes->SetUINT32(GM_PARAM_RTCP_ENABLED, PARAM_RTCP_ENABLED));
	GMT_CHECK_HR(hr = pAttributes->SetUINT32(GM_PARAM_RTCPMUX_ENABLED, PARAM_RTCPMUX_ENABLED));

	// Get local SDP to be sent to the remote party. Must be done after configuration.
	GMT_CHECK_HR(hr = pAttributes->GetString(GM_PARAM_LOCAL_SDP, szLocalSDP_, sizeof(szLocalSDP_) / sizeof(szLocalSDP_[0]), &uint32_));
	GMT_DEBUG_INFO("Local SDP to send to the remote party/viewer:%.*ls", uint32_, szLocalSDP_);

	// Set remote SDP received from the remote party. Must be done after configuration.
#if GMT_LOOPBACK
	GMT_CHECK_HR(hr = pAttributes->SetString(GM_PARAM_REMOTE_SDP, szLocalSDP_));
#else
	GMT_CHECK_HR(hr = pAttributes->SetString(GM_PARAM_REMOTE_SDP, GMT_REMOTE_SDP));
#endif /* GMT_LOOPBACK */

	//
	//
	// Remote SDP and all configuration parameters must be set before beginning to write samples
	//
	//

	GMT_CHECK_HR(hr = pSinkWriter->BeginWriting()); // required! - will start media streaming

	while (frames_count_++ < VIDEO_FRAMES_COUNT)
	{ 
		// Copy random data into the sample
		GMT_CHECK_HR(hr = pBuffer->Lock(&pSamplesDataPtr_, NULL, NULL));
		for (DWORD i = 0; i < samplesCount_; ++i)
		{
			pSamplesDataPtr_[i] = rand() % 0xFF;
		}
		GMT_CHECK_HR(hr = pBuffer->Unlock());
		GMT_CHECK_HR(hr = pBuffer->SetCurrentLength(samplesCount_));

		// Update start time
		GMT_CHECK_HR(hr = pSample->SetSampleTime(rtStart_));
		// Send samples over the network
		GMT_CHECK_HR(hr = pSinkWriter->WriteSample(0, pSample));

		rtStart_ += rtDuration_;

		// Simulate delay (not required)
		SleepEx((DWORD)(rtDuration_ / 10000), FALSE);
	}

	GMT_DEBUG_INFO("*** Sending frames done!! ***\nPress ENTER to shutdown.");

	getchar();

	GMT_CHECK_HR(hr = pMediaSink->Shutdown()); // required (strange) -> otherwise, memory leaks (Sink refCount never reach #0)

bail:
	GmtSafeRelease(&pSinkWriter);
	GmtSafeRelease(&pSinkWriterEx);
	GmtSafeRelease(&pAttributes);
	GmtSafeRelease(&pMediaSink);
	GmtSafeRelease(&pMediaTypeIn);
	GmtSafeRelease(&pSample);
	GmtSafeRelease(&pBuffer);

	MFShutdown();
	CoUninitialize();

	return SUCCEEDED(hr) ? 0 : -1;
}