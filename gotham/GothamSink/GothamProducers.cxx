#include "GothamProducers.h"
#include "GothamSinkUtils.h"

#include "tinymedia/tmedia_producer.h"

#include "tsk_string.h"
#include "tsk_safeobj.h"
#include "tsk_debug.h"

#include <assert.h>

//
//	ANSI-C implementation
//

typedef struct gm_producer_s
{
	TMEDIA_DECLARE_PRODUCER;

	bool bPrepared;
	bool bStarted;
	bool bPaused;

	TSK_DECLARE_SAFEOBJ;
}
gm_producer_t;
#define gm_producer(self) ((gm_producer_t*)(self))

static int gm_producer_set(tmedia_producer_t* self, const tmedia_param_t* params)
{
	return 0;
}

static int gm_producer_prepare(tmedia_producer_t* self, const tmedia_codec_t* codec)
{
	gm_producer_t* pProducer = (gm_producer_t*)self;
	int ret = -1;
	if (codec && pProducer)
	{
		tsk_safeobj_lock(pProducer);
		GM_DEBUG_INFO("[GM_PRODUCER] gm_producer_prepare(%d, %d, %d)", TMEDIA_CODEC_VIDEO(codec)->out.width, TMEDIA_CODEC_VIDEO(codec)->out.height, TMEDIA_CODEC_VIDEO(codec)->out.fps);
		pProducer->bPrepared = true;
		tsk_safeobj_unlock(pProducer);
		ret = 0;
	}

	return ret;
}

static int gm_producer_start(tmedia_producer_t* self)
{
	gm_producer_t* pProducer = (gm_producer_t*)self;
	int ret = -1;
	if (pProducer)
	{
		tsk_safeobj_lock(pProducer);
		GM_DEBUG_INFO("[GM_PRODUCER] gm_producer_start");
		pProducer->bStarted = true;
		pProducer->bPaused = false;
		tsk_safeobj_unlock(pProducer);
		ret = 0;
	}
	return ret;
}

static int gm_producer_pause(tmedia_producer_t* self)
{
	gm_producer_t* pProducer = (gm_producer_t*)self;
	int ret = -1;
	if (pProducer)
	{
		tsk_safeobj_lock(pProducer);
		GM_DEBUG_INFO("[GM_PRODUCER] gm_producer_pause");
		pProducer->bPaused = true;
		tsk_safeobj_unlock(pProducer);
		ret = 0;
	}
	return ret;
}

static int gm_producer_stop(tmedia_producer_t* self)
{
	gm_producer_t* pProducer = (gm_producer_t*)self;
	int ret = -1;
	if (pProducer)
	{
		tsk_safeobj_lock(pProducer);
		GM_DEBUG_INFO("[GM_PRODUCER] gm_producer_stop");
		pProducer->bStarted = false;
		pProducer->bPaused = false;
		tsk_safeobj_unlock(pProducer);
		ret = 0;
	}
	return ret;
}


/* constructor */
static tsk_object_t* gm_producer_ctor(tsk_object_t * self, va_list * app)
{
	gm_producer_t *pProducer = (gm_producer_t *)self;
	if (pProducer)
	{
		enum tmedia_chroma_e chroma;
		/* init base */
		tmedia_producer_init(TMEDIA_PRODUCER(pProducer));
		/* init self */
		tsk_safeobj_init(pProducer);
		if (FAILED(GmSinkUtils::ConvertVideoFormatToNative(MFVideoFormat_NV12, chroma)))
		{
			GM_DEBUG_ERROR("Cannot find valid default chroma");
			return tsk_null;
		}
		TMEDIA_PRODUCER(pProducer)->video.width = 176; // default width
		TMEDIA_PRODUCER(pProducer)->video.height = 144; // default height
		TMEDIA_PRODUCER(pProducer)->video.chroma = chroma;
	}
	return self;
}
/* destructor */
static tsk_object_t* gm_producer_dtor(tsk_object_t * self)
{
	gm_producer_t *pProducer = (gm_producer_t *)self;
	if (pProducer)
	{
		/* stop */
		if (pProducer->bStarted)
		{
			gm_producer_stop(TMEDIA_PRODUCER(pProducer));
		}

		/* deinit base */
		tmedia_producer_deinit(TMEDIA_PRODUCER(pProducer));
		/* deinit self */
		tsk_safeobj_deinit(pProducer);

		GM_DEBUG_INFO("[GM_PRODUCER] *** destroyed ***");
	}

	return self;
}
/* object definition */
static const tsk_object_def_t gm_producer_def_s =
{
	sizeof(gm_producer_t),
	gm_producer_ctor,
	gm_producer_dtor,
	tsk_null,
};
/* plugin definition*/
static const tmedia_producer_plugin_def_t gm_producer_plugin_def_s =
{
	&gm_producer_def_s,

	tmedia_video,
	"GM_PRODUCER",

	gm_producer_set,
	gm_producer_prepare,
	gm_producer_start,
	gm_producer_pause,
	gm_producer_stop
};

const tmedia_producer_plugin_def_t *gm_producer_plugin_def_t = &gm_producer_plugin_def_s;



//
//	CPP implementation
//

GmProducer::GmProducer(const struct gm_producer_s* pcWrappedProd)
	: GmObj()
	, m_pcWrappedProd(pcWrappedProd)
{
	GM_ASSERT(m_pcWrappedProd != NULL);
}

GmProducer::~GmProducer()
{
	GM_DEBUG_INFO("*** GmProducer destroyed ***");
}

HRESULT GmProducer::SetInputSize(UINT32 nWidth, UINT32 nHeight)
{
	HRESULT hr = S_OK;

	if (!nWidth || !nHeight)
	{
		GM_CHECK_HR(hr = E_INVALIDARG);
	}
	TMEDIA_PRODUCER(m_pcWrappedProd)->video.width = nWidth;
	TMEDIA_PRODUCER(m_pcWrappedProd)->video.height = nHeight;
	
bail:
	return hr;
}

HRESULT GmProducer::SetInputFormat(const GUID& subType)
{
	HRESULT hr = S_OK;
	tmedia_chroma_t chroma;
	
	GM_CHECK_HR(hr = GmSinkUtils::ConvertVideoFormatToNative(subType, chroma));
	TMEDIA_PRODUCER(m_pcWrappedProd)->video.chroma = chroma;

bail:
	return hr;
}

HRESULT GmProducer::WriteSamples(const BYTE* pBytesPtr, UINT32 nBytesCount)
{
	HRESULT hr = S_OK;

	if (TMEDIA_PRODUCER(m_pcWrappedProd)->enc_cb.callback)
	{
		if (m_pcWrappedProd->bStarted)
		{
			int ret = TMEDIA_PRODUCER(m_pcWrappedProd)->enc_cb.callback(TMEDIA_PRODUCER(m_pcWrappedProd)->enc_cb.callback_data, pBytesPtr, nBytesCount);
			if (ret != 0)
			{
				GM_DEBUG_ERROR("Failed to send samples. Error code = %d", ret);
				GM_CHECK_HR(hr = E_FAIL);
			}
		}
		else
		{
			GM_CHECK_HR(hr = E_ILLEGAL_METHOD_CALL);
		}
	}
	else
	{
		GM_CHECK_HR(hr = E_ILLEGAL_METHOD_CALL);
	}
	
bail:
	return hr;
}

