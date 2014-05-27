/* Copyright (C) 2014 Mamadou DIOP.
 *
 * This file is part of Open Source Doubango Framework.
 *
 * DOUBANGO is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * DOUBANGO is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with DOUBANGO.
 */
#include "../../../common/_Config.h"

#if W4A_UNDER_MAC

#include "tinymedia/tmedia_producer.h"

#include "tsk_timer.h"
#include "tsk_debug.h"

#import <Cocoa/Cocoa.h>
#import <QTKit/QTkit.h>

#if !defined(kMaxFrameRate)
#       define kMaxFrameRate 4 // FIXME
#endif /* kMaxFrameRate */

@interface osxProducerScreenCast : NSObject  {
    const struct w4a_producer_screencast_osx_s* m_pcWrappedProducer;
    
    bool m_bStarted;
    tsk_timer_manager_handle_t *m_pTimerMgr;
    tsk_timer_id_t m_idTimerGrab;
    uint64_t m_uTimoutGrab;
    void* m_pBuffPtr;
    tsk_size_t m_uBuffSize;
    CGWindowID m_idWindow;
}

- (osxProducerScreenCast*) initWithProducer:(const struct w4a_producer_screencast_osx_s *)pc_WrappedProducer;
- (bool)setPreviewRootLayer:(CALayer*)pRootLayer;
- (bool)start;
- (bool)pause;
- (bool)stop;
- (bool)grab;
@end

typedef struct w4a_producer_screencast_osx_s
{
    TMEDIA_DECLARE_PRODUCER;
    
    bool bStarted;
    
    osxProducerScreenCast *pOSXProducer;
    TSK_DECLARE_SAFEOBJ;
}
w4a_producer_screencast_osx_t;

/* ============ Media Producer Interface ================= */
static int w4a_producer_screencast_osx_set(tmedia_producer_t *self, const tmedia_param_t* param)
{
	int ret = 0;
	w4a_producer_screencast_osx_t* pSelf = (w4a_producer_screencast_osx_t*)self;
    
	if (!pSelf || !param) {
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}
    
	return ret;
}

static int w4a_producer_screencast_osx_prepare(tmedia_producer_t* self, const tmedia_codec_t* codec)
{
	w4a_producer_screencast_osx_t* pSelf = (w4a_producer_screencast_osx_t*)self;
    
	if (!pSelf || !codec || !codec->plugin) {
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}
    
	TMEDIA_PRODUCER(pSelf)->video.fps = TSK_MIN(TMEDIA_CODEC_VIDEO(codec)->out.fps, kMaxFrameRate);
	TMEDIA_PRODUCER(pSelf)->video.width = TMEDIA_CODEC_VIDEO(codec)->out.width;
	TMEDIA_PRODUCER(pSelf)->video.height = TMEDIA_CODEC_VIDEO(codec)->out.height;
    
    [pSelf->pOSXProducer release], pSelf->pOSXProducer = NULL;
    pSelf->pOSXProducer = [[osxProducerScreenCast alloc] initWithProducer:pSelf];
    if (!pSelf->pOSXProducer) {
        TSK_DEBUG_ERROR("Failed to create producer");
        return -2;
    }
    
    return 0;
    
}
                 
static int w4a_producer_screencast_osx_start(tmedia_producer_t* self)
{
    w4a_producer_screencast_osx_t* pSelf = (w4a_producer_screencast_osx_t*)self;
    
    if (!pSelf) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    
    if (pSelf->bStarted) {
        return 0;
    }
    
    if (!pSelf->pOSXProducer) {
        TSK_DEBUG_ERROR("QT video producer not prepared");
        return -2;
    }
    if (![pSelf->pOSXProducer start]) {
        return -3;
    }
    
    pSelf->bStarted = true;
    
    return 0;
}

static int w4a_producer_screencast_osx_pause(tmedia_producer_t* self)
{
    w4a_producer_screencast_osx_t* pSelf = (w4a_producer_screencast_osx_t*)self;
    
    if (!pSelf) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    
    if (pSelf->pOSXProducer) {
        [pSelf->pOSXProducer pause];
    }
    
    return 0;
}

static int w4a_producer_screencast_osx_stop(tmedia_producer_t* self)
{
    w4a_producer_screencast_osx_t* pSelf = (w4a_producer_screencast_osx_t*)self;
    
    if (!pSelf) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    
    // for the thread
    pSelf->bStarted = false;
    
    if (pSelf->pOSXProducer) {
        [pSelf->pOSXProducer stop];
    }
    
    return 0;
}

static int _osx_producer_screencast_timer_cb(const void* arg, tsk_timer_id_t timer_id)
{
    osxProducerScreenCast* pOSXProducer = [((osxProducerScreenCast*)arg) retain];
    [pOSXProducer grab];
    [pOSXProducer release];
    
    return 0;
}

@implementation osxProducerScreenCast

- (osxProducerScreenCast*) initWithProducer:(const struct w4a_producer_screencast_osx_s *)pc_WrappedProducer {
    if ((self = (osxProducerScreenCast*)[super init])) {
		m_pcWrappedProducer = pc_WrappedProducer;
        m_uTimoutGrab = (1000/TMEDIA_PRODUCER(m_pcWrappedProducer)->video.fps);
        m_pTimerMgr = tsk_timer_manager_create();
        m_idWindow = kCGNullWindowID; // Entire desktop
	}
	return self;
}

- (bool)setPreviewRootLayer:(CALayer*)pRootLayer {
    if (!pRootLayer){
		
	}
	else {
		
	}
    return true;
}

- (bool)start {
	if (!m_pTimerMgr) {
        TSK_DEBUG_ERROR("No valid timer manager");
        return false;
    }
    
    int ret = 0;
    
    tsk_safeobj_lock(m_pcWrappedProducer);
    
    if (m_bStarted) {
        TSK_DEBUG_INFO("OSX screencast producer already started");
        goto bail;
    }
    
    if ((ret = tsk_timer_manager_start(m_pTimerMgr))) {
        goto bail;
    }
    
    m_idTimerGrab = tsk_timer_manager_schedule(m_pTimerMgr, m_uTimoutGrab, _osx_producer_screencast_timer_cb, self);
    if (!TSK_TIMER_ID_IS_VALID(m_idTimerGrab)) {
        TSK_DEBUG_ERROR("Failed to schedule timer with timeout=%llu", m_uTimoutGrab);
        ret = -2;
        goto bail;
    }
    
    m_bStarted = true;
    
bail:
    tsk_safeobj_unlock(m_pcWrappedProducer);
	
	return (ret == 0);
}

- (bool)pause {
    
    return true;
}

- (bool)stop {
    int ret = 0;
    
    tsk_safeobj_lock(m_pcWrappedProducer);
    
    if (!m_bStarted) {
        goto bail;
    }
    
    m_bStarted = false;
    
    if (m_pTimerMgr) {
        tsk_timer_manager_stop(m_pTimerMgr);
    }
    
bail:
    tsk_safeobj_unlock(m_pcWrappedProducer);
    
    return (ret == 0);
}

- (bool)grab {
    int ret = 0;
    CGImageRef screenShot  = NULL;
    CGDataProviderRef provider;
    CFDataRef dataRef;
    uint8_t *screenPixels, *srcBuffPtr;
    size_t screenWidth, screenStride, screenHeight, screenBytesPerPix, screenBuffSize;
    
    tsk_safeobj_lock(m_pcWrappedProducer);
    
    if (!m_bStarted) {
        TSK_DEBUG_ERROR("OSX screencast not started");
        ret = -1;
        goto bail;
    }
    
    screenShot = CGWindowListCreateImage(CGRectInfinite, kCGWindowListOptionOnScreenOnly, m_idWindow, kCGWindowImageDefault);
	if (!screenShot) {
        TSK_DEBUG_ERROR("Failed to take screenshot (%u)", m_idWindow);
        ret = -2;
        goto bail;
    }
    
    screenBytesPerPix = (CGImageGetBitsPerPixel(screenShot) + 7) / 8;
    switch (screenBytesPerPix) {
        case 3: TMEDIA_PRODUCER(m_pcWrappedProducer)->video.chroma = tmedia_chroma_rgb24; break;
        case 4: TMEDIA_PRODUCER(m_pcWrappedProducer)->video.chroma = tmedia_chroma_rgb32; break;
        default: TSK_DEBUG_ERROR("%lu not valid bytes per pixel", screenBytesPerPix); ret = -3; goto bail;
    }
    
    screenStride = CGImageGetWidth(screenShot);
    screenWidth = CGImageGetBytesPerRow(screenShot)/screenBytesPerPix;
    screenHeight = CGImageGetHeight(screenShot);
    screenBuffSize = (screenWidth * screenHeight * screenBytesPerPix);
    if (m_uBuffSize < screenBuffSize) {
        m_pBuffPtr = tsk_realloc(m_pBuffPtr, (tsk_size_t)screenBuffSize);
        if (!m_pBuffPtr) {
            TSK_DEBUG_ERROR("Failed to allocate buffer with size = %lu", screenBuffSize);
            m_uBuffSize = 0;
            ret = -4;
            goto bail;
        }
        m_uBuffSize = (tsk_size_t)screenBuffSize;
    }
    
    provider = CGImageGetDataProvider(screenShot);
    dataRef = CGDataProviderCopyData(provider);
    screenPixels = (uint8_t*)CFDataGetBytePtr(dataRef);
    
    if (screenWidth == screenStride) {
        srcBuffPtr = screenPixels;
    }
    else {
        size_t i;
        uint8_t* _pBuffPtr = (uint8_t*)m_pBuffPtr;
        for (i = 0; i < screenHeight; ++i) {
            memcpy(_pBuffPtr, screenPixels, screenWidth*screenBytesPerPix);
            screenPixels += screenStride*screenBytesPerPix;
            _pBuffPtr += screenWidth*screenBytesPerPix;
        }
        srcBuffPtr = (uint8_t*)m_pBuffPtr;
    }
    
    // encode and send data
    TMEDIA_PRODUCER(m_pcWrappedProducer)->video.width = (tsk_size_t)screenWidth;
    TMEDIA_PRODUCER(m_pcWrappedProducer)->video.height = (tsk_size_t)screenHeight;
    TMEDIA_PRODUCER(m_pcWrappedProducer)->enc_cb.callback(TMEDIA_PRODUCER(m_pcWrappedProducer)->enc_cb.callback_data, srcBuffPtr, (tsk_size_t)screenBuffSize);
    
    m_idTimerGrab = tsk_timer_manager_schedule(m_pTimerMgr, m_uTimoutGrab, _osx_producer_screencast_timer_cb, self);
    if (!TSK_TIMER_ID_IS_VALID(m_idTimerGrab)) {
        TSK_DEBUG_ERROR("Failed to schedule timer with timeout=%llu", m_uTimoutGrab);
        ret = -5;
        goto bail;
    }
    
bail:
    tsk_safeobj_unlock(m_pcWrappedProducer);
    if (screenShot) {
        CGImageRelease(screenShot), screenShot = NULL;
    }
    
    return (ret == 0);
}

- (void)dealloc {
    [self stop];
	m_pcWrappedProducer = tsk_null; // you're not the owner
	
    TSK_OBJECT_SAFE_FREE(m_pTimerMgr);
    TSK_FREE(m_pBuffPtr);
    m_uBuffSize = 0;
    
	[super dealloc];
    
    TSK_DEBUG_INFO("*** osxProducerScreenCast destroyed ***");
}

@end

/* constructor */
static tsk_object_t* w4a_producer_screencast_osx_ctor(tsk_object_t * self, va_list * app)
{
    w4a_producer_screencast_osx_t *pSelf = (w4a_producer_screencast_osx_t *)self;
    if(pSelf){
        /* init base */
        tmedia_producer_init(TMEDIA_PRODUCER(pSelf));
        TMEDIA_PRODUCER(pSelf)->video.chroma = tmedia_chroma_rgb32;
        /* init self with default values*/
        
        TMEDIA_PRODUCER(pSelf)->video.fps = 15;
        TMEDIA_PRODUCER(pSelf)->video.width = 352;
        TMEDIA_PRODUCER(pSelf)->video.height = 288;
        
        tsk_safeobj_init(pSelf);
    }
    return self;
}
/* destructor */
static tsk_object_t* w4a_producer_screencast_osx_dtor(tsk_object_t * self)
{
    w4a_producer_screencast_osx_t *pSelf = (w4a_producer_screencast_osx_t *)self;
    if (pSelf) {
        /* stop */
        //if(pSelf->started){
        w4a_producer_screencast_osx_stop((tmedia_producer_t*)self);
        //}
        
        /* deinit base */
        tmedia_producer_deinit(TMEDIA_PRODUCER(pSelf));
        /* deinit self */
        
        [pSelf->pOSXProducer release], pSelf->pOSXProducer = NULL;
        tsk_safeobj_deinit(pSelf);
        
        TSK_DEBUG_INFO("*** w4a_producer_screencast_osx destroyed ***");
    }
    
    return self;
}
/* object definition */
static const tsk_object_def_t w4a_producer_screencast_osx_def_s =
{
    sizeof(w4a_producer_screencast_osx_t),
    w4a_producer_screencast_osx_ctor,
    w4a_producer_screencast_osx_dtor,
    tsk_null,
};
/* plugin definition*/
static const tmedia_producer_plugin_def_t w4a_producer_screencast_osx_plugin_def_s =
{
    &w4a_producer_screencast_osx_def_s,
    
    tmedia_bfcp_video,
    "OSX screncast producer",
    
    w4a_producer_screencast_osx_set,
    w4a_producer_screencast_osx_prepare,
    w4a_producer_screencast_osx_start,
    w4a_producer_screencast_osx_pause,
    w4a_producer_screencast_osx_stop
};
const tmedia_producer_plugin_def_t *w4a_producer_screencast_osx_plugin_def_t = &w4a_producer_screencast_osx_plugin_def_s;

#endif /*W4A_UNDER_MAC  */