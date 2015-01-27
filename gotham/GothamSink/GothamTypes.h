#pragma once

#define GM_ERR_PRODUCER_NOT_STARTED MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0200)

typedef void GmNativeMutexHandle_t; // Mapping to "tsk_mutex_handle_t"
typedef void GmNativeNetTransportHandle_t; // Mapping to "tnet_transport_handle_t"
typedef void GmNativeThreadHandle_t; // Mapping to "tsk_thread_handle_t"

typedef enum GmSessionType_e
{
	GmSessionType_None,
	GmSessionType_Call
}
GmSessionType_t;

typedef enum GmMode_e
{
	GmMode_None = 0,
	GmMode_Optional,
	GmMode_Mandatory
}
GmMode_t;

typedef enum GmRoType_e
{
	GmRoType_Unknown = 0,
	GmRoType_Offer = 1,
	GmRoType_Answer = 2,
	GmRoType_PrAnswer = 6
}
GmRoType_t;

typedef enum GmMediaType_e
{
	GmMediaType_None = 0x00,
	GmMediaType_Video = (0x01 << 1),
	GmMediaType_Audio = (0x01 << 0),
	GmMediaType_ScreenCast = (0x01 << 2),
	GmMediaType_AudioVideo = (GmMediaType_Audio | GmMediaType_Video),

	GmMediaType_All = 0xFF,
}
GmMediaType_t;