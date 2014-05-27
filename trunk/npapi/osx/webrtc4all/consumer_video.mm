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

#include "tinymedia/tmedia_consumer.h"

#include "tsk_mutex.h"
#include "tsk_string.h"
#include "tsk_debug.h"

#import <Cocoa/Cocoa.h>
#import <QuartzCore/CoreVideo.h>
#import <QuartzCore/CIContext.h>

typedef struct w4a_consumer_video_osx_s
{
	TMEDIA_DECLARE_CONSUMER;
    
    BOOL bStarted;
    CALayer *pRootLayer;
    CGContextRef pContext;
    tsk_size_t uNegWidth;
    tsk_size_t uNegHeight;
    void* pBuff;
    tsk_mutex_handle_t *pMutex;
}
w4a_consumer_video_osx_t;

/* ============ Media Producer Interface ================= */
int w4a_consumer_video_osx_set(tmedia_consumer_t *self, const tmedia_param_t* param)
{
	int ret = 0;
    
	if (!self || !param) {
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}
    
	return ret;
}


int w4a_consumer_video_osx_prepare(tmedia_consumer_t* self, const tmedia_codec_t* codec)
{
	w4a_consumer_video_osx_t* consumer = (w4a_consumer_video_osx_t*)self;
    
	if (!consumer || !codec || !codec->plugin) {
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}
    
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    
	TMEDIA_CONSUMER(consumer)->video.fps = TMEDIA_CODEC_VIDEO(codec)->in.fps;
	TMEDIA_CONSUMER(consumer)->video.in.width = TMEDIA_CODEC_VIDEO(codec)->in.width;
	TMEDIA_CONSUMER(consumer)->video.in.height = TMEDIA_CODEC_VIDEO(codec)->in.height;
    
	if (!TMEDIA_CONSUMER(consumer)->video.display.width) {
		TMEDIA_CONSUMER(consumer)->video.display.width = TMEDIA_CONSUMER(consumer)->video.in.width;
	}
	if (!TMEDIA_CONSUMER(consumer)->video.display.height) {
		TMEDIA_CONSUMER(consumer)->video.display.height = TMEDIA_CONSUMER(consumer)->video.in.height;
	}
    
    // FIXME
    extern CALayer *__layerDisplayConsumer;
    if (__layerDisplayConsumer) {
        [consumer->pRootLayer release];
        consumer->pRootLayer = [__layerDisplayConsumer retain];
    }

    [pool release];
    
	return 0;
}

int w4a_consumer_video_osx_start(tmedia_consumer_t* self)
{
	w4a_consumer_video_osx_t* consumer = (w4a_consumer_video_osx_t*)self;
    
	if (!consumer) {
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}
    
    tsk_mutex_lock(consumer->pMutex);
    
    consumer->bStarted = YES;
    
    tsk_mutex_unlock(consumer->pMutex);
    
	return 0;
}

int w4a_consumer_video_osx_consume(tmedia_consumer_t* _self, const void* buffer, tsk_size_t size, const tsk_object_t* proto_hdr)
{
	w4a_consumer_video_osx_t* consumer = (w4a_consumer_video_osx_t*)_self;
    int ret = 0;
    tsk_size_t uBuffSize;
    
    if (!consumer || !buffer || !size) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    
    tsk_mutex_lock(consumer->pMutex);
    
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    
    if (!consumer->pRootLayer || !consumer->bStarted) {
        ret = 0;
        goto bail;
    }
    
    uBuffSize = (_self->video.in.width * _self->video.in.height) << 2; // RGB32
    if (uBuffSize != size) {
        TSK_DEBUG_ERROR("Size missmatch: %u<>%u", uBuffSize, size);
        ret = -2;
        goto bail;
    }
	
    if (!consumer->pContext || _self->video.in.width != consumer->uNegWidth || _self->video.in.height != consumer->uNegHeight) {
        TSK_DEBUG_INFO("Video consumer size changed: (%ux%u)->(%ux%u)", consumer->uNegWidth, consumer->uNegHeight, _self->video.in.width, _self->video.in.height);
        if (consumer->pContext) {
            CGContextRelease(consumer->pContext);
            consumer->pContext = NULL;
        }
        if (!(consumer->pBuff = tsk_realloc(consumer->pBuff, uBuffSize))) {
            TSK_DEBUG_ERROR("Failed to allocate buffer with size = %u", uBuffSize);
            ret = -3;
            goto bail;
        }
        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
		consumer->pContext = CGBitmapContextCreate(consumer->pBuff, _self->video.in.width, _self->video.in.height, 8, _self->video.in.width * 4, colorSpace,
											   kCGBitmapByteOrder32Little | kCGImageAlphaPremultipliedFirst);
		CGColorSpaceRelease(colorSpace);
        if (!consumer->pContext) {
            TSK_DEBUG_ERROR("failed to create RGB32 context");
            ret = -4;
            goto bail;
        }
        CGContextSetInterpolationQuality(consumer->pContext, kCGInterpolationMedium);
        CGContextSetShouldAntialias(consumer->pContext, true);
        consumer->pRootLayer.contentsGravity = kCAGravityResizeAspect;
        consumer->uNegWidth = _self->video.in.width;
        consumer->uNegHeight = _self->video.in.height;
    }
    
    memcpy(consumer->pBuff, buffer, uBuffSize);
    
    tsk_object_ref(TSK_OBJECT(consumer));
    dispatch_async(dispatch_get_main_queue(), ^{ // *must* be async
        tsk_mutex_lock(consumer->pMutex);
        if (consumer->bStarted) {
            CGImageRef image = CGBitmapContextCreateImage(consumer->pContext);
            consumer->pRootLayer.contents = (id)image;
            CGImageRelease(image);
        }
        tsk_mutex_unlock(consumer->pMutex);
        tsk_object_unref(TSK_OBJECT(consumer));
    });
    
bail:
    [pool release];
    tsk_mutex_unlock(consumer->pMutex);
	return ret;
}

int w4a_consumer_video_osx_pause(tmedia_consumer_t* self)
{
	w4a_consumer_video_osx_t* consumer = (w4a_consumer_video_osx_t*)self;
    
	if (!consumer) {
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}
    
	return 0;
}

int w4a_consumer_video_osx_stop(tmedia_consumer_t* _self)
{
	w4a_consumer_video_osx_t* consumer = (w4a_consumer_video_osx_t*)_self;
    
	TSK_DEBUG_INFO("w4a_consumer_video_osx_stop");
    
	if (!_self) {
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}
    
    tsk_mutex_lock(consumer->pMutex);
    
    consumer->bStarted = NO;
    
    tsk_mutex_unlock(consumer->pMutex);
    
	return 0;
}


//
//	Windows Media Foundation video consumer object definition
//
/* constructor */
static tsk_object_t* w4a_consumer_video_osx_ctor(tsk_object_t * self, va_list * app)
{
	w4a_consumer_video_osx_t *consumer = (w4a_consumer_video_osx_t *)self;
	if (consumer) {
		/* init base */
		tmedia_consumer_init(TMEDIA_CONSUMER(consumer));
		TMEDIA_CONSUMER(consumer)->video.display.chroma = tmedia_chroma_rgb32;
        
		/* init self */
		TMEDIA_CONSUMER(consumer)->video.fps = 15;
		TMEDIA_CONSUMER(consumer)->video.display.width = 352;
		TMEDIA_CONSUMER(consumer)->video.display.height = 288;
		TMEDIA_CONSUMER(consumer)->video.display.auto_resize = tsk_true;
        if (!(consumer->pMutex = tsk_mutex_create())) {
            TSK_DEBUG_ERROR("Failed to create mutex");
            return NULL;
        }
	}
	return self;
}
/* destructor */
static tsk_object_t* w4a_consumer_video_osx_dtor(tsk_object_t * _self)
{
	w4a_consumer_video_osx_t *consumer = (w4a_consumer_video_osx_t *)_self;
	if (consumer) {
        
		/* stop */
		//if(consumer->started){
        w4a_consumer_video_osx_stop((tmedia_consumer_t*)_self);
		//}
        
		/* deinit base */
		tmedia_consumer_deinit(TMEDIA_CONSUMER(consumer));
		/* deinit self */
        if (consumer->pRootLayer) {
            consumer->pRootLayer.contents = nil;
            [consumer->pRootLayer release], consumer->pRootLayer = nil;
        }
        if (consumer->pContext) {
            CGContextRelease(consumer->pContext);
            consumer->pContext = NULL;
        }
        TSK_FREE(consumer->pBuff);
        if (consumer->pMutex) {
            tsk_mutex_destroy(&consumer->pMutex);
        }
        
        TSK_DEBUG_INFO("*** w4a_consumer_video_osx destroyed ***");
	}
    
	return _self;
}
/* object definition */
static const tsk_object_def_t w4a_consumer_video_osx_def_s =
{
	sizeof(w4a_consumer_video_osx_t),
	w4a_consumer_video_osx_ctor,
	w4a_consumer_video_osx_dtor,
	tsk_null,
};
/* plugin definition*/
static const tmedia_consumer_plugin_def_t w4a_consumer_video_osx_plugin_def_s =
{
	&w4a_consumer_video_osx_def_s,
	
	tmedia_video,
	"OSX video consumer",
	
	w4a_consumer_video_osx_set,
	w4a_consumer_video_osx_prepare,
	w4a_consumer_video_osx_start,
	w4a_consumer_video_osx_consume,
	w4a_consumer_video_osx_pause,
	w4a_consumer_video_osx_stop
};
const tmedia_consumer_plugin_def_t *w4a_consumer_video_osx_plugin_def_t = &w4a_consumer_video_osx_plugin_def_s;

#endif /*W4A_UNDER_MAC  */