#include <tchar.h>
#include "GothamSinkUtils.h"

#include <aviriff.h>
#include <mferror.h>

#include <comutil.h>
#include <AtlConv.h>
#include <atlexcept.h>

#include "tinymedia/tmedia_common.h"
#include "tsk_debug.h"

static const struct {
	enum tmedia_chroma_e chroma;
	const GUID& fourcc;
} VideoFormatsMap[] = {
	{ tmedia_chroma_yuv420p, MFVideoFormat_I420 },
	{ tmedia_chroma_nv12, MFVideoFormat_NV12 },
	{ tmedia_chroma_uyvy422, MFVideoFormat_UYVY },
	{ tmedia_chroma_yuyv422, MFVideoFormat_YUY2 },
	/* TODO: Add more YUV formats */
	{ tmedia_chroma_rgb565le, MFVideoFormat_RGB565 },
	{ tmedia_chroma_bgr24, MFVideoFormat_RGB24 },
	{ tmedia_chroma_rgb32, MFVideoFormat_RGB32 },
};

HRESULT GmSinkUtils::ConvertVideoFormatToNative(const GUID& in, enum tmedia_chroma_e &out)
{
	for (UINT32 i = 0; i < sizeof(VideoFormatsMap) / sizeof(VideoFormatsMap[0]); ++i)
	{
		if (VideoFormatsMap[i].fourcc == in)
		{
			out = VideoFormatsMap[i].chroma;
			return S_OK;
		}
	}
	return E_NOT_SET;
}

HRESULT GmSinkUtils::ConvertVideoFormatFromNative(const enum tmedia_chroma_e &in, GUID& out)
{
	for (UINT32 i = 0; i < sizeof(VideoFormatsMap) / sizeof(VideoFormatsMap[0]); ++i)
	{
		if (VideoFormatsMap[i].chroma == in)
		{
			out = VideoFormatsMap[i].fourcc;
			return S_OK;
		}
	}
	return E_NOT_SET;
}

HRESULT GmSinkUtils::CreateVideoType(
	const GUID* subType, // video subType
	IMFMediaType **ppType,     // Receives a pointer to the media type.
	UINT32 unWidth /*= 0*/, // Video width (0 to ignore)
	UINT32 unHeight /*= 0*/ // Video height (0 to ignore)
	)
{
	HRESULT hr = S_OK;

	IMFMediaType *pType = NULL;

	GM_CHECK_HR(hr = MFCreateMediaType(&pType));

	GM_CHECK_HR(hr = pType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));

	GM_CHECK_HR(hr = pType->SetGUID(MF_MT_SUBTYPE, *subType));

	GM_CHECK_HR(hr = pType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE)); // UnCompressed

	GM_CHECK_HR(hr = pType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive));

	if (unWidth > 0 && unHeight > 0)
	{
		GM_CHECK_HR(hr = MFSetAttributeSize(pType, MF_MT_FRAME_SIZE, unWidth, unHeight));
	}

	*ppType = pType;
	(*ppType)->AddRef();

bail:
	GmSafeRelease(&pType);
	return hr;
}

//-------------------------------------------------------------------
// Name: ValidateVideoFormat
// Description: Validates a media type for this sink.
//-------------------------------------------------------------------
HRESULT GmSinkUtils::ValidateVideoFormat(IMFMediaType *pmt)
{
	GUID major_type = GUID_NULL;
	GUID subtype = GUID_NULL;
	MFVideoInterlaceMode interlace = MFVideoInterlace_Unknown;
	UINT32 val = 0;
	BOOL bFoundMatchingSubtype = FALSE;

	HRESULT hr = S_OK;

	// Major type must be video.
	GM_CHECK_HR(hr = pmt->GetGUID(MF_MT_MAJOR_TYPE, &major_type));

	if (major_type != MFMediaType_Video)
	{
		GM_CHECK_HR(hr = MF_E_INVALIDMEDIATYPE);
	}

	// Subtype must be one of the subtypes in our global list.

	// Get the subtype GUID.
	GM_CHECK_HR(hr = pmt->GetGUID(MF_MT_SUBTYPE, &subtype));

	// Look for the subtype in our list of accepted types.
	for (DWORD i = 0; i < g_GmNumVideoSubtypes; i++)
	{
		if (subtype == *g_GmVideoSubtypes[i])
		{
			bFoundMatchingSubtype = TRUE;
			break;
		}
	}

	if (!bFoundMatchingSubtype)
	{
		GM_CHECK_HR(hr = MF_E_INVALIDMEDIATYPE);
	}

	// Video must be progressive frames.
	GM_CHECK_HR(hr = pmt->GetUINT32(MF_MT_INTERLACE_MODE, (UINT32*)&interlace));
	if (interlace != MFVideoInterlace_Progressive)
	{
		GM_CHECK_HR(hr = MF_E_INVALIDMEDIATYPE);
	}

bail:
	return hr;
}

HRESULT GmSinkUtils::ConvertStringWString(const std::string & in, std::wstring &out)
{
	USES_CONVERSION_EX;
	const wchar_t* _out = A2W_EX(in.c_str(), in.length());
	out = std::wstring(_out);
	return S_OK;
}

HRESULT GmSinkUtils::ConvertWStringString(const std::wstring & in, std::string &out)
{
	USES_CONVERSION_EX;
	const char* _out = W2A_EX(in.c_str(), in.length());
	out = std::string(_out);
	return S_OK;
}