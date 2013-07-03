#include <Windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <Mfreadwrite.h>
#include <mferror.h>
#include <Wmcodecdsp.h>
#include <Codecapi.h>

#include <stdio.h>
#include <initguid.h>

// Intel QuickSync
// {08B2F572-51BF-4e93-8B15-33864546DC9A}
DEFINE_GUID(CLSID_MF_H264EncFilter,
0x08b2f572, 0x51bf, 0x4e93, 0x8b, 0x15, 0x33, 0x86, 0x45, 0x46, 0xdc, 0x9a);

#pragma comment(lib, "mfreadwrite")
#pragma comment(lib, "mfplat")
#pragma comment(lib, "mfuuid")

const UINT32 VIDEO_WIDTH = 640;
const UINT32 VIDEO_HEIGHT = 480;
const UINT32 VIDEO_FPS = 25;
const UINT32 VIDEO_BIT_RATE = 800000;
const UINT32 VIDEO_BUFFER_SIZE = (VIDEO_WIDTH * VIDEO_HEIGHT * 3) >> 1;

#undef SafeRelease
#define SafeRelease(ppT) \
{ \
    if (*ppT) \
    { \
        (*ppT)->Release(); \
        *ppT = NULL; \
    } \
} 

#undef CHECK_HR
#define CHECK_HR(x) if (FAILED(x)) { fprintf(stdout, "Operation Failed"); goto bail; }

enum EncodeMode
{
    EncodeMode_CBR,
    EncodeMode_VBR_Quality,
    EncodeMode_VBR_Peak,
    EncodeMode_VBR_Unconstrained,
};

struct LeakyBucket
{
    DWORD dwBitrate;
    DWORD msBufferSize;
    DWORD msInitialBufferFullness;
};

class CWmaEncoder 
{
public:

    CWmaEncoder() 
        : m_pMFT(NULL), m_dwInputID(0), m_dwOutputID(0), m_pOutputType(NULL)
    {
    }

    ~CWmaEncoder()
    {
        SafeRelease(&m_pMFT);
        SafeRelease(&m_pOutputType);
    }

    HRESULT Initialize();
    HRESULT SetEncodingType(EncodeMode mode);
    HRESULT SetInputType(IMFMediaType* pMediaType);
    HRESULT GetOutputType(IMFMediaType** ppMediaType);
    HRESULT GetLeakyBucket1(LeakyBucket *pBucket);
    HRESULT ProcessInput(IMFSample* pSample);
    HRESULT ProcessOutput(IMFSample **ppSample);
    HRESULT Drain();
       
protected:
    DWORD           m_dwInputID;     // Input stream ID.
    DWORD           m_dwOutputID;    // Output stream ID.

    IMFTransform    *m_pMFT;         // Pointer to the encoder MFT.
    IMFMediaType    *m_pOutputType;  // Output media type of the encoder.

};


HRESULT CWmaEncoder::Initialize() 
{
    CLSID *pCLSIDs = NULL;   // Pointer to an array of CLISDs. 
    UINT32 count = 0;      // Size of the array.

    IMFMediaType* pOutMediaType = NULL;
    
    // Look for a encoder.
    MFT_REGISTER_TYPE_INFO toutinfo;
    toutinfo.guidMajorType = MFMediaType_Video;
    toutinfo.guidSubtype = MFVideoFormat_H264;

    HRESULT hr = S_OK;

	UINT32 unFlags = MFT_ENUM_FLAG_HARDWARE |
                 MFT_ENUM_FLAG_SYNCMFT  |  
                 MFT_ENUM_FLAG_LOCALMFT | 
                 MFT_ENUM_FLAG_SORTANDFILTER;

    hr = MFTEnum(
        MFT_CATEGORY_VIDEO_ENCODER,
        unFlags,                  // Reserved
        NULL,               // Input type to match. 
        &toutinfo,          // Output type to match.
        NULL,               // Attributes to match. (None.)
        &pCLSIDs,           // Receives a pointer to an array of CLSIDs.
        &count              // Receives the size of the array.
        );
    
    if (SUCCEEDED(hr))
    {
        if (count == 0)
        {
            hr = MF_E_TOPO_CODEC_NOT_FOUND;
        }
    }

	//hr = CoCreateInstance(CLSID_MF_H264EncFilter, NULL, 
    //        CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_pMFT));
	

    //Create the MFT decoder
    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(pCLSIDs[0], NULL, 
            CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_pMFT));
    }

    return hr;
}

HRESULT CWmaEncoder::SetEncodingType(EncodeMode mode)
{
    if (!m_pMFT)
    {
        return MF_E_NOT_INITIALIZED;
    }

    IPropertyStore* pProp = NULL;

    PROPVARIANT var;

    //Query the encoder for its property store

    HRESULT hr = m_pMFT->QueryInterface(IID_PPV_ARGS(&pProp));
    if (FAILED(hr))
    {
        goto done;
    }

    switch (mode)
    {
    case EncodeMode_CBR:
        //Set the VBR property to FALSE, which indicates CBR encoding
        //By default, encoding mode is set to CBR
        var.vt = VT_BOOL;
        var.boolVal = FALSE;
        hr = pProp->SetValue(MFPKEY_VBRENABLED, var);
        break;


    default:
        hr = E_NOTIMPL;
    }
    
done:
    SafeRelease(&pProp);
    return hr;
}

HRESULT CWmaEncoder::SetInputType(IMFMediaType* pMediaType)
{
    if (!m_pMFT)
    {
        return MF_E_NOT_INITIALIZED;
    }

    SafeRelease(&m_pOutputType);

    IMFMediaType *pOutputType = NULL;

    HRESULT hr = m_pMFT->GetStreamIDs(1, &m_dwInputID, 1, &m_dwOutputID);

    if (hr == E_NOTIMPL)
    {
        // The stream identifiers are zero-based.
        m_dwInputID = 0;
        m_dwOutputID = 0;
        hr = S_OK;
    }
    else if (FAILED(hr))
    {
        goto done;
    }

	{ // FIXME
		SafeRelease(&m_pOutputType);
		hr = MFCreateMediaType(&m_pOutputType);
		hr = m_pOutputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
		hr = m_pOutputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
		hr = m_pOutputType->SetUINT32(MF_MT_MPEG2_PROFILE, 1 ? eAVEncH264VProfile_Base : eAVEncH264VProfile_Main); // FIXME
		hr = m_pOutputType->SetUINT32(MF_MT_AVG_BITRATE, VIDEO_BIT_RATE);       
        hr = m_pOutputType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);   
        hr = MFSetAttributeSize(m_pOutputType, MF_MT_FRAME_SIZE, VIDEO_WIDTH, VIDEO_HEIGHT);       
        hr = MFSetAttributeRatio(m_pOutputType, MF_MT_FRAME_RATE, VIDEO_FPS, 1);       
        hr = MFSetAttributeRatio(m_pOutputType, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
		hr = m_pMFT->SetOutputType(m_dwOutputID, m_pOutputType, 0);
	}

    // Set the input type to the one passed by the application
    hr = m_pMFT->SetInputType(m_dwInputID, pMediaType, 0);
    if (FAILED(hr))
    {
        goto done;
    }

    // Loop through the available output types
    /*for (DWORD iType = 0; ; iType++)
    {
        hr = m_pMFT->GetOutputAvailableType(m_dwOutputID, iType, &pOutputType);
        if (FAILED(hr))
        {
            goto done;
        }
        
        hr = m_pMFT->SetOutputType(m_dwOutputID, pOutputType, 0);

        if (SUCCEEDED(hr))
        {
            m_pOutputType = pOutputType;
            m_pOutputType->AddRef();
            break;
        }

        SafeRelease(&pOutputType);
    }*/

done:
    SafeRelease(&pOutputType);
    return hr;
}

HRESULT CWmaEncoder::GetOutputType(IMFMediaType** ppMediaType)
{
    if (m_pOutputType == NULL)
    {
        return MF_E_TRANSFORM_TYPE_NOT_SET;
    }
        
    *ppMediaType = m_pOutputType;
    (*ppMediaType)->AddRef();

    return S_OK;
};


HRESULT CWmaEncoder::GetLeakyBucket1(LeakyBucket *pBucket)
{
    if (m_pMFT == NULL || m_pOutputType == NULL)
    {
        return MF_E_NOT_INITIALIZED;
    }

    ZeroMemory(pBucket, sizeof(*pBucket));

    // Get the bit rate.

    pBucket->dwBitrate = 8 * MFGetAttributeUINT32(
        m_pOutputType, MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 0);


    // Get the buffer window.

    IWMCodecLeakyBucket *pLeakyBuckets = NULL;

    HRESULT hr = m_pMFT->QueryInterface(IID_PPV_ARGS(&pLeakyBuckets));
    if (SUCCEEDED(hr))
    {
        ULONG ulBuffer = 0;

        hr = pLeakyBuckets->GetBufferSizeBits(&ulBuffer);

        if (SUCCEEDED(hr))
        {
            pBucket->msBufferSize = ulBuffer / (pBucket->dwBitrate / 1000);    
        }

        pLeakyBuckets->Release();
    }

    return S_OK;
}


HRESULT CWmaEncoder::ProcessInput(IMFSample* pSample)
{
    if (m_pMFT == NULL)
    {
        return MF_E_NOT_INITIALIZED;
    }

    return m_pMFT->ProcessInput(m_dwInputID, pSample, 0);
}

HRESULT CWmaEncoder::ProcessOutput(IMFSample **ppSample)
{
    if (m_pMFT == NULL)
    {
        return MF_E_NOT_INITIALIZED;
    }

    *ppSample = NULL;

    IMFMediaBuffer* pBufferOut = NULL;
    IMFSample* pSampleOut = NULL;

    DWORD dwStatus;
  
    MFT_OUTPUT_STREAM_INFO mftStreamInfo = { 0 };
    MFT_OUTPUT_DATA_BUFFER mftOutputData = { 0 };

    HRESULT hr = m_pMFT->GetOutputStreamInfo(m_dwOutputID, &mftStreamInfo);
    if (FAILED(hr))
    {
        goto done;
    }

    //create a buffer for the output sample
    hr = MFCreateMemoryBuffer(mftStreamInfo.cbSize, &pBufferOut);
    if (FAILED(hr))
    {
        goto done;
    }

    //Create the output sample
    hr = MFCreateSample(&pSampleOut);
    if (FAILED(hr))
    {
        goto done;
    }

    //Add the output buffer 
    hr = pSampleOut->AddBuffer(pBufferOut);
    if (FAILED(hr))
    {
        goto done;
    }
 
    //Set the output sample
    mftOutputData.pSample = pSampleOut;

    //Set the output id
    mftOutputData.dwStreamID = m_dwOutputID;

    //Generate the output sample
    hr = m_pMFT->ProcessOutput(0, 1, &mftOutputData, &dwStatus);
    if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT)
    {
        hr = S_OK;
        goto done;
    }

    // TODO: Handle MF_E_TRANSFORM_STREAM_CHANGE

    if (FAILED(hr))
    {
        goto done;
    }

    *ppSample = pSampleOut;
    (*ppSample)->AddRef();

done:
    SafeRelease(&pBufferOut);
    SafeRelease(&pSampleOut);
    return hr;
};

HRESULT CWmaEncoder::Drain()
{
    if (m_pMFT == NULL)
    {
        return MF_E_NOT_INITIALIZED;
    }

    return m_pMFT->ProcessMessage(MFT_MESSAGE_COMMAND_DRAIN, m_dwInputID);
}

static HRESULT CreateMediaSample(DWORD cbData, IMFSample **ppSample)
{
    HRESULT hr = S_OK;

    IMFSample *pSample = NULL;
    IMFMediaBuffer *pBuffer = NULL;

    hr = MFCreateSample(&pSample);

    if (SUCCEEDED(hr))
    {
        hr = MFCreateMemoryBuffer(cbData, &pBuffer);
    }

    if (SUCCEEDED(hr))
    {
        hr = pSample->AddBuffer(pBuffer);
    }

    if (SUCCEEDED(hr))
    {
        *ppSample = pSample;
        (*ppSample)->AddRef();
    }

    SafeRelease(&pSample);
    SafeRelease(&pBuffer);
    return hr;
}

void main()
{
	HRESULT hr = S_OK;
	IMFMediaType    *pMediaTypeIn = NULL;
	IMFMediaType    *pMediaTypeOut = NULL;
	IMFSample *pSampleIn = NULL;
	IMFSample *pSampleOut = NULL;
	LONGLONG rtStart = 0;
    UINT64 rtDuration;
	IMFMediaBuffer *pMediaBuffer = NULL;
	DWORD nLength;

	CHECK_HR(hr = MFStartup(MF_VERSION, 0));
	CWmaEncoder* encoder = new CWmaEncoder();
	CHECK_HR(hr = encoder->Initialize());

    CHECK_HR(hr = MFFrameRateToAverageTimePerFrame(VIDEO_FPS, 1, &rtDuration));

	CHECK_HR(hr = MFCreateMediaType(&pMediaTypeIn));
	CHECK_HR(hr = pMediaTypeIn->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
	CHECK_HR(hr = pMediaTypeIn->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12));
	CHECK_HR(hr = pMediaTypeIn->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive));
	CHECK_HR(hr = MFSetAttributeSize(pMediaTypeIn, MF_MT_FRAME_SIZE, VIDEO_WIDTH, VIDEO_HEIGHT)); 
	CHECK_HR(hr = MFSetAttributeRatio(pMediaTypeIn, MF_MT_FRAME_RATE, VIDEO_FPS, 1));
	CHECK_HR(hr = MFSetAttributeRatio(pMediaTypeIn, MF_MT_PIXEL_ASPECT_RATIO, 1, 1));
	
	CHECK_HR(hr = encoder->SetInputType(pMediaTypeIn));

	CHECK_HR(hr = CreateMediaSample(VIDEO_BUFFER_SIZE, &pSampleIn));
	CHECK_HR(hr = pSampleIn->SetSampleDuration(rtDuration));
	
	for(INT i = 0; i < 10; ++i)
	{
		CHECK_HR(hr = pSampleIn->SetSampleTime(rtStart));
		CHECK_HR(hr = encoder->ProcessInput(pSampleIn));
		rtStart += rtDuration;

		CHECK_HR(hr = encoder->ProcessOutput(&pSampleOut));
		if(pSampleOut)
		{
			SafeRelease(&pMediaBuffer);
			CHECK_HR(hr = pSampleOut->GetBufferByIndex(0, &pMediaBuffer));
			CHECK_HR(hr = pMediaBuffer->GetCurrentLength(&nLength));
			if(nLength > 0)
			{
				BYTE* pBuffer = NULL;
				CHECK_HR(hr = pMediaBuffer->Lock(&pBuffer, NULL, NULL));
			}
			SafeRelease(&pSampleOut);
		}
	}

	(MF_E_TRANSFORM_TYPE_NOT_SET);

bail:
	SafeRelease(&pMediaBuffer);
	SafeRelease(&pMediaTypeIn);
	SafeRelease(&pMediaTypeOut);
	SafeRelease(&pSampleIn);
	SafeRelease(&pSampleOut);
}