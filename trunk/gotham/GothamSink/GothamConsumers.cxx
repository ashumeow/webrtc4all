#include "GothamConsumers.h"
#include "GothamSinkUtils.h"

#include "tinymedia/tmedia_consumer.h"

#include "tsk_string.h"
#include "tsk_safeobj.h"
#include "tsk_debug.h"

typedef struct gm_display_s {
	TMEDIA_DECLARE_CONSUMER;

	tsk_bool_t b_started;
	tsk_bool_t b_prepared;

	TSK_DECLARE_SAFEOBJ;
}
gm_display_t;



/* ============ Media Consumer Interface ================= */
static int gm_display_set(tmedia_consumer_t *self, const tmedia_param_t* param)
{
	int ret = 0;
	gm_display_t* p_display = (gm_display_t*)self;

	if (!self || !param)
	{
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}

	if (param->value_type == tmedia_pvt_int64)
	{
		if (tsk_striequals(param->key, "remote-hwnd"))
		{
			GM_DEBUG_INFO("[GM_DISPLAY] Set remote handle where to display video");
		}
	}
	else if (param->value_type == tmedia_pvt_int32)
	{
		if (tsk_striequals(param->key, "fullscreen"))
		{
			GM_DEBUG_INFO("[GM_DISPLAY] Enable/disable fullscreen");
		}
	}

	return ret;
}


static int gm_display_prepare(tmedia_consumer_t* self, const tmedia_codec_t* codec)
{
	gm_display_t* p_display = (gm_display_t*)self;

	GM_DEBUG_INFO("[GM_DISPLAY] prepare");

	if (!p_display || !codec && codec->plugin)
	{
		GM_DEBUG_ERROR("[GM_DISPLAY] Invalid parameter");
		return -1;
	}

	tsk_safeobj_lock(p_display);

	TMEDIA_CONSUMER(p_display)->video.fps = TMEDIA_CODEC_VIDEO(codec)->in.fps;
	TMEDIA_CONSUMER(p_display)->video.in.width = TMEDIA_CODEC_VIDEO(codec)->in.width;
	TMEDIA_CONSUMER(p_display)->video.in.height = TMEDIA_CODEC_VIDEO(codec)->in.height;

	// Defines what we want for incoming size (up to Doubango video engine to resize decoded stream to make you happy)
	if (!TMEDIA_CONSUMER(p_display)->video.display.width)
	{
		TMEDIA_CONSUMER(p_display)->video.display.width = TMEDIA_CONSUMER(p_display)->video.in.width;
	}
	if (!TMEDIA_CONSUMER(p_display)->video.display.height)
	{
		TMEDIA_CONSUMER(p_display)->video.display.height = TMEDIA_CONSUMER(p_display)->video.in.height;
	}

	p_display->b_prepared = tsk_true;

	tsk_safeobj_unlock(p_display);

	return 0;
}

static int gm_display_start(tmedia_consumer_t* self)
{
	gm_display_t* p_display = (gm_display_t*)self;
	int ret = 0;

	GM_DEBUG_INFO("[GM_DISPLAY] start");

	if (!p_display)
	{
		GM_DEBUG_ERROR("[GM_DISPLAY] Invalid parameter");
		return -1;
	}

	tsk_safeobj_lock(p_display);

	if (!p_display->b_prepared) {
		GM_DEBUG_ERROR("[GM_DISPLAY] Not prepared");
		ret = -2;
		goto bail;
	}

	if (p_display->b_started)
	{
		GM_DEBUG_WARN("[GM_DISPLAY] Already started");
		goto bail;
	}

	p_display->b_started = tsk_true;

bail:
	tsk_safeobj_unlock(p_display);
	return ret;
}

static int gm_display_consume(tmedia_consumer_t* self, const void* buffer, tsk_size_t size, const tsk_object_t* proto_hdr)
{
	gm_display_t* p_display = (gm_display_t*)self;
	GM_DEBUG_INFO("[GM_DISPLAY] consume");
	if (p_display && buffer && size)
	{
		tsk_safeobj_lock(p_display);
		GM_DEBUG_INFO("[GM_DISPLAY] Skipping video buffer: %ux%u", TMEDIA_CONSUMER(p_display)->video.display.width, TMEDIA_CONSUMER(p_display)->video.display.height);
		tsk_safeobj_unlock(p_display);
		return 0;
	}
	return -1;
}

static int gm_display_pause(tmedia_consumer_t* self)
{
	gm_display_t* p_display = (gm_display_t*)self;

	GM_DEBUG_INFO("[GM_DISPLAY] pause");

	if (!p_display)
	{
		TSK_DEBUG_ERROR("[GM_DISPLAY] Invalid parameter");
		return -1;
	}

	tsk_safeobj_lock(p_display);

	tsk_safeobj_unlock(p_display);

	return 0;
}

static int gm_display_stop(tmedia_consumer_t* self)
{
	gm_display_t* p_display = (gm_display_t*)self;
	int ret = 0;

	GM_DEBUG_INFO("[GM_DISPLAY] stop");

	if (!p_display)
	{
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}

	tsk_safeobj_lock(p_display);

	if (!p_display->b_started) {
		goto bail;
	}

	p_display->b_started = tsk_false;

bail:
	tsk_safeobj_unlock(p_display);
	return ret;
}

/* constructor */
static tsk_object_t* gm_display_ctor(tsk_object_t * self, va_list * app)
{
	gm_display_t *p_display = (gm_display_t *)self;
	if (p_display) {
		/* init base */
		tmedia_consumer_init(TMEDIA_CONSUMER(p_display));
		TMEDIA_CONSUMER(p_display)->video.display.chroma = tmedia_chroma_yuv420p; // We want I420 frames
#if 0 // Display doesn't need resizing?
		TMEDIA_CONSUMER(p_display)->video.display.auto_resize = tsk_true;
#endif

		/* init self */
		tsk_safeobj_init(p_display);
	}
	return self;
}
/* destructor */
static tsk_object_t* gm_display_dtor(tsk_object_t * self)
{
	gm_display_t *p_display = (gm_display_t *)self;
	if (p_display) {
		/* stop */
		if (p_display->b_started) {
			gm_display_stop(TMEDIA_CONSUMER(p_display));
		}
		/* deinit base */
		tmedia_consumer_deinit(TMEDIA_CONSUMER(p_display));
		/* deinit self */
		tsk_safeobj_deinit(p_display);

		GM_DEBUG_INFO("*** [GM_DISPLAY] destroyed ***");
	}

	return self;
}
/* object definition */
static const tsk_object_def_t gm_display_def_s = {
	sizeof(gm_display_t),
	gm_display_ctor,
	gm_display_dtor,
	tsk_null,
};
/* plugin definition*/
static const tmedia_consumer_plugin_def_t gm_display_plugin_def_s = {
	&gm_display_def_s,

	tmedia_video,
	"GM_DISPLAY",

	gm_display_set,
	gm_display_prepare,
	gm_display_start,
	gm_display_consume,
	gm_display_pause,
	gm_display_stop
};
const tmedia_consumer_plugin_def_t *gm_display_plugin_def_t = &gm_display_plugin_def_s;

