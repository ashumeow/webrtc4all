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
#include "tsk_debug.h"

#import <Cocoa/Cocoa.h>
#import <QTKit/QTkit.h>

@interface qtProducerVideo : NSObject  {
    const struct w4a_producer_video_qt_s* m_pcWrappedProducer;
    tsk_mutex_handle_t *m_pSenderMutex;
    dispatch_queue_t m_pSenderQueue;
    
    QTCaptureSession *m_pCaptureSession;
    QTCaptureDevice *m_pCaptureDevice;
    QTCaptureLayer *m_pCaptureLayer;
    
    bool m_bStarted;
    bool m_bFirstFrame;
}

- (qtProducerVideo*) initWithProducer:(const struct w4a_producer_video_qt_s *)pc_WrappedProducer;
- (bool)setPreviewRootLayer:(CALayer*)pRootLayer;
- (bool)startVideoCapture;
- (bool)stopVideoCapture;
- (bool)startPreview;
- (bool)stopPreview;
- (bool)start;
- (bool)pause;
- (bool)stop;
@end

typedef struct w4a_producer_video_qt_s
{
    TMEDIA_DECLARE_PRODUCER;
    
    bool bStarted;
    
    qtProducerVideo *pQtProducer;
}
w4a_producer_video_qt_t;

/* ============ Media Producer Interface ================= */
static int w4a_producer_video_qt_set(tmedia_producer_t *self, const tmedia_param_t* param)
{
	int ret = 0;
	w4a_producer_video_qt_t* pSelf = (w4a_producer_video_qt_t*)self;
    
	if (!pSelf || !param) {
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}
    
	return ret;
}

static int w4a_producer_video_qt_prepare(tmedia_producer_t* self, const tmedia_codec_t* codec)
{
	w4a_producer_video_qt_t* pSelf = (w4a_producer_video_qt_t*)self;
    
	if (!pSelf || !codec || !codec->plugin) {
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}
	
    
    [pSelf->pQtProducer release], pSelf->pQtProducer = NULL; // should be null
    if (!(pSelf->pQtProducer = [[qtProducerVideo alloc] initWithProducer:pSelf])) {
        TSK_DEBUG_ERROR("Failed to create Qt video producer");
        return -2;
    }
    
	TMEDIA_PRODUCER(pSelf)->video.fps = TMEDIA_CODEC_VIDEO(codec)->out.fps;
	TMEDIA_PRODUCER(pSelf)->video.width = TMEDIA_CODEC_VIDEO(codec)->out.width;
	TMEDIA_PRODUCER(pSelf)->video.height = TMEDIA_CODEC_VIDEO(codec)->out.height;
    
    // FIXME
    extern CALayer *__layerDisplayProducer;
    if (__layerDisplayProducer) {
        if ([NSThread currentThread] != [NSThread mainThread]) { // from Doubango worker thread?
            [pSelf->pQtProducer performSelectorOnMainThread:@selector(setPreviewRootLayer:) withObject:__layerDisplayProducer waitUntilDone:YES];
        }
        else {
            [pSelf->pQtProducer setPreviewRootLayer:__layerDisplayProducer];
        }
    }
    
    return 0;
    
}
                 
static int w4a_producer_video_qt_start(tmedia_producer_t* self)
{
    w4a_producer_video_qt_t* pSelf = (w4a_producer_video_qt_t*)self;
    
    if (!pSelf) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    
    if (pSelf->bStarted) {
        return 0;
    }
    
    if (!pSelf->pQtProducer) {
        TSK_DEBUG_ERROR("QT video producer not prepared");
        return -2;
    }
    if (![pSelf->pQtProducer start]) {
        return -3;
    }
    
    pSelf->bStarted = true;
    
    return 0;
}

static int w4a_producer_video_qt_pause(tmedia_producer_t* self)
{
    w4a_producer_video_qt_t* pSelf = (w4a_producer_video_qt_t*)self;
    
    if (!pSelf) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    
    if (pSelf->pQtProducer) {
        [pSelf->pQtProducer pause];
    }
    
    return 0;
}

static int w4a_producer_video_qt_stop(tmedia_producer_t* self)
{
    w4a_producer_video_qt_t* pSelf = (w4a_producer_video_qt_t*)self;
    
    if (!pSelf) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    
    // for the thread
    pSelf->bStarted = false;
    
    if (pSelf->pQtProducer) {
        [pSelf->pQtProducer stop];
    }
    
    return 0;
}

@implementation qtProducerVideo

- (qtProducerVideo*) initWithProducer:(const struct w4a_producer_video_qt_s *)pc_WrappedProducer {
    if ((self = (qtProducerVideo*)[super init])) {
		m_pcWrappedProducer =pc_WrappedProducer;
		m_pSenderMutex = tsk_mutex_create_2(tsk_false);
	}
	return self;
}

- (bool)setPreviewRootLayer:(CALayer*)pRootLayer {
    if (!pRootLayer){
		// stop preview
		[self stopPreview];
	}
	else {
		// start preview
        if (!m_pCaptureLayer) {
            m_pCaptureLayer = [[QTCaptureLayer alloc] init];
        }
        if (m_pCaptureLayer) {
            [m_pCaptureLayer removeFromSuperlayer];
            m_pCaptureLayer.bounds = pRootLayer.bounds;
            m_pCaptureLayer.frame = pRootLayer.frame;
            m_pCaptureLayer.autoresizingMask = kCALayerWidthSizable | kCALayerHeightSizable;
            m_pCaptureLayer.session = m_pCaptureSession;
            [pRootLayer addSublayer:m_pCaptureLayer];
        }
        [self startPreview];
	}
    return true;
}

- (bool)startVideoCapture {
    bool success;
	NSError *error = nil;
    
    if (m_pCaptureSession && [m_pCaptureSession isRunning]) {
        TSK_DEBUG_INFO("Video capture already started");
        return true;
    }
	
	TSK_DEBUG_INFO("Starting Video stream...");
	
    if (!m_pCaptureDevice) {
        m_pCaptureDevice = [[QTCaptureDevice defaultInputDeviceWithMediaType:QTMediaTypeVideo] retain];
        if (!m_pCaptureDevice) {
            TSK_DEBUG_ERROR("Failed to get valid capture device[QTMediaTypeVideo]");
            return false;
        }
    }
	
    if (![m_pCaptureDevice isOpen]) {
        success = [m_pCaptureDevice open:&error];
        if (!success) {
            NSLog(@"Failed to open video device(QTMediaTypeVideo): %@", error);
            [m_pCaptureDevice release], m_pCaptureDevice = NULL;
            
            m_pCaptureDevice = [[QTCaptureDevice defaultInputDeviceWithMediaType:QTMediaTypeMuxed] retain];
            if (!m_pCaptureDevice) {
                TSK_DEBUG_ERROR("Failed to get valid capture device[QTMediaTypeMuxed]");
                return false;
            }
            success = [m_pCaptureDevice open:&error];
            if (!success) {
                NSLog(@"Failed to open video device(QTMediaTypeVideo): %@", error);
                [m_pCaptureDevice release], m_pCaptureDevice = NULL;
                return false;
            }
        }
    }
	
    QTCaptureDeviceInput *videoInput = [[QTCaptureDeviceInput alloc] initWithDevice:m_pCaptureDevice];
    if (!videoInput){
        NSLog(@"Failed to create video input: %@", error);
		[m_pCaptureDevice release], m_pCaptureDevice = NULL;
        return false;
    }
    
    if (m_pCaptureSession) {
        for (QTCaptureDeviceInput *input in [m_pCaptureSession inputs]) {
            [m_pCaptureSession removeInput:input];
        }
        for (QTCaptureOutput *output in [m_pCaptureSession outputs]) {
            [m_pCaptureSession removeOutput:output];
        }
    }
    else {
        m_pCaptureSession = [[QTCaptureSession alloc] init];
        if (!m_pCaptureSession) {
            TSK_DEBUG_ERROR("Failed to create QT capture session");
            return false;
        }
    }
    
	success = [m_pCaptureSession addInput:videoInput error:&error];
    if (!success) {
		NSLog(@"Failed to add video input: %@", error);
		return false;
    }
	
	QTCaptureDecompressedVideoOutput *decompressedVideoOutput = [[QTCaptureDecompressedVideoOutput alloc] init];
	[decompressedVideoOutput setAutomaticallyDropsLateVideoFrames:YES];
	[decompressedVideoOutput setMinimumVideoFrameInterval:(1.0f/(double)TMEDIA_PRODUCER(m_pcWrappedProducer)->video.fps)];
	
	NSArray *formats = [NSArray arrayWithObjects:
                        [NSNumber numberWithUnsignedInt:kCVPixelFormatType_420YpCbCr8Planar],
                        [NSNumber numberWithUnsignedInt:kCVPixelFormatType_422YpCbCr8],
                        [NSNumber numberWithUnsignedInt:kCVPixelFormatType_32ARGB],
                        [NSNumber numberWithUnsignedInt:kCVPixelFormatType_24RGB],
                        nil];
	NSDictionary *attributes = [NSDictionary dictionaryWithObjectsAndKeys:
                                formats, kCVPixelBufferPixelFormatTypeKey,
                                [NSNumber numberWithInt:TMEDIA_PRODUCER(m_pcWrappedProducer)->video.width], (id)kCVPixelBufferWidthKey,
                                [NSNumber numberWithInt:TMEDIA_PRODUCER(m_pcWrappedProducer)->video.height], (id)kCVPixelBufferHeightKey,
                                
                                nil];
	[decompressedVideoOutput setPixelBufferAttributes:attributes];
	[decompressedVideoOutput setDelegate:self];
	success = [m_pCaptureSession addOutput:decompressedVideoOutput error:&error];
	if (!success) {
        NSLog(@"Failed to add video output: %@", error);
        return false;
    }
	[decompressedVideoOutput release];
	
	if ([NSThread currentThread] != [NSThread mainThread]) { // from Doubango worker thread?
		[self performSelectorOnMainThread:@selector(startPreview) withObject:nil waitUntilDone:YES];
	}
	else {
		[self startPreview];
	}
	
	TSK_DEBUG_INFO("Video capture started");
    
    return true;
}

- (bool)stopVideoCapture {
    if (m_pCaptureSession) {
		[m_pCaptureSession stopRunning];
	}
	
	if ([NSThread currentThread] != [NSThread mainThread]) { // From Doubango worker thread?
		[self performSelectorOnMainThread:@selector(stopPreview) withObject:nil waitUntilDone:YES];
	}
	else {
		[self stopPreview];
	}
    return true;
}

- (bool)startPreview {
    if (m_pCaptureSession) {
		if (m_pCaptureLayer) {
            m_pCaptureLayer.session = m_pCaptureSession;
		}
        
		if (![m_pCaptureSession isRunning]) {
			[m_pCaptureSession startRunning];
		}
	}
    return true;
}

- (bool)stopPreview {
    if (m_pCaptureSession) {
		if ([m_pCaptureSession isRunning]) {
			[m_pCaptureSession stopRunning];
		}
	}
    if (m_pCaptureLayer) {
        [m_pCaptureLayer removeFromSuperlayer];
        m_pCaptureLayer.session = nil;
    }
    return true;
}

- (bool)start {
	// start video capture
	[self performSelectorOnMainThread:@selector(startVideoCapture) withObject:nil waitUntilDone:YES];
	
	if (!m_pSenderQueue) {
		m_pSenderQueue = dispatch_queue_create("org.doubango.webrtc4all.producer.sender", NULL);
        if (!m_pSenderQueue) {
            TSK_DEBUG_ERROR("Failed to create queue");
            return false;
        }
		dispatch_queue_t high_prio_queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0);
		dispatch_set_target_queue(m_pSenderQueue, high_prio_queue);
	}
	
    m_bFirstFrame = true;
    m_bStarted = true;
	return true;
}

- (bool)pause {
    if (m_pCaptureSession) {
        
    }
    return true;
}

- (bool)stop {
    m_bStarted = false;
    [self performSelectorOnMainThread:@selector(stopVideoCapture) withObject:nil waitUntilDone:YES];
    return true;
}

- (void)dealloc {
    [self stop];
	m_pcWrappedProducer = tsk_null; // you're not the owner
	
	if (m_pSenderQueue) {
		dispatch_release(m_pSenderQueue);
        m_pSenderQueue = NULL;
	}
	if (m_pSenderMutex) {
		tsk_mutex_destroy(&m_pSenderMutex);
	}
	
	[m_pCaptureSession release], m_pCaptureSession = NULL;
	[m_pCaptureDevice release], m_pCaptureDevice = NULL;
	[m_pCaptureLayer release], m_pCaptureLayer = NULL;
	
	[super dealloc];
    
    TSK_DEBUG_INFO("*** qtProducerVideo destroyed ***");
}

- (void)captureOutput:(QTCaptureOutput *)captureOutput
  didOutputVideoFrame:(CVImageBufferRef)videoFrame
	 withSampleBuffer:(QTSampleBuffer *)sampleBuffer
	   fromConnection:(QTCaptureConnection *)connection
{
	CVReturn status = CVPixelBufferLockBaseAddress(videoFrame, 0);
	
    tmedia_producer_t* p_producer = (tmedia_producer_t*)tsk_object_ref(TSK_OBJECT(m_pcWrappedProducer));
	if (m_bStarted && status == 0 && p_producer) {
		UInt8 *bufferPtr = (UInt8 *)CVPixelBufferGetBaseAddress(videoFrame);
		size_t buffeSize = CVPixelBufferGetDataSize(videoFrame);
			
        if (m_bFirstFrame) {
            tsk_mutex_lock(m_pSenderMutex);
            
            p_producer->video.width = (tsk_size_t)CVPixelBufferGetWidth(videoFrame);
            p_producer->video.height = (tsk_size_t)CVPixelBufferGetHeight(videoFrame);
            
            int pixelFormat = CVPixelBufferGetPixelFormatType(videoFrame);
            switch (pixelFormat) {
                case kCVPixelFormatType_420YpCbCr8Planar:
                    p_producer->video.chroma = tmedia_chroma_yuv420p;
                    TSK_DEBUG_INFO("Capture pixel format=kCVPixelFormatType_420YpCbCr8Planar");
                    break;
                case kCVPixelFormatType_422YpCbCr8:
                    p_producer->video.chroma = tmedia_chroma_uyvy422;
                    TSK_DEBUG_INFO("Capture pixel format=kCVPixelFormatType_422YpCbCr8");
                    break;
                case kCVPixelFormatType_24RGB:
                    p_producer->video.chroma = tmedia_chroma_rgb24;
                    TSK_DEBUG_INFO("Capture pixel format=kCVPixelFormatType_24RGB");
                    break;
                case kCVPixelFormatType_32ARGB:
                    p_producer->video.chroma = tmedia_chroma_rgb32;
                    TSK_DEBUG_INFO("Capture pixel format=kCVPixelFormatType_32ARGB");
                    break;
                default:
                    p_producer->video.chroma = tmedia_chroma_rgb32;
                    TSK_DEBUG_INFO("Error --> %i not supported as pixelFormat", pixelFormat);
                    goto done;
            }
            
            tsk_mutex_unlock(m_pSenderMutex);
            
            m_bFirstFrame = false;
        }
		
		
        dispatch_sync(m_pSenderQueue, ^{
            tsk_mutex_lock(m_pSenderMutex);
            p_producer->enc_cb.callback(p_producer->enc_cb.callback_data, bufferPtr, (tsk_size_t)buffeSize);
            tsk_mutex_unlock(m_pSenderMutex);
        });
		
		
	done:
		tsk_object_unref(p_producer);
	}
	
	CVPixelBufferUnlockBaseAddress(videoFrame, 0);
}

@end

/* constructor */
static tsk_object_t* w4a_producer_video_qt_ctor(tsk_object_t * self, va_list * app)
{
    w4a_producer_video_qt_t *pSelf = (w4a_producer_video_qt_t *)self;
    if(pSelf){
        /* init base */
        tmedia_producer_init(TMEDIA_PRODUCER(pSelf));
        TMEDIA_PRODUCER(pSelf)->video.chroma = tmedia_chroma_yuv420p; // To avoid chroma conversion
        /* init self with default values*/
        
        TMEDIA_PRODUCER(pSelf)->video.fps = 15;
        TMEDIA_PRODUCER(pSelf)->video.width = 352;
        TMEDIA_PRODUCER(pSelf)->video.height = 288;
    }
    return self;
}
/* destructor */
static tsk_object_t* w4a_producer_video_qt_dtor(tsk_object_t * self)
{
    w4a_producer_video_qt_t *pSelf = (w4a_producer_video_qt_t *)self;
    if (pSelf) {
        /* stop */
        //if(pSelf->started){
        w4a_producer_video_qt_stop((tmedia_producer_t*)self);
        //}
        
        /* deinit base */
        tmedia_producer_deinit(TMEDIA_PRODUCER(pSelf));
        /* deinit self */
        
        [pSelf->pQtProducer release], pSelf->pQtProducer = NULL;
        
        TSK_DEBUG_INFO("*** w4a_producer_video_qt destroyed ***");
    }
    
    return self;
}
/* object definition */
static const tsk_object_def_t w4a_producer_video_qt_def_s =
{
    sizeof(w4a_producer_video_qt_t),
    w4a_producer_video_qt_ctor,
    w4a_producer_video_qt_dtor,
    tsk_null,
};
/* plugin definition*/
static const tmedia_producer_plugin_def_t w4a_producer_video_qt_plugin_def_s =
{
    &w4a_producer_video_qt_def_s,
    
    tmedia_video,
    "QuickTime video producer",
    
    w4a_producer_video_qt_set,
    w4a_producer_video_qt_prepare,
    w4a_producer_video_qt_start,
    w4a_producer_video_qt_pause,
    w4a_producer_video_qt_stop
};
const tmedia_producer_plugin_def_t *w4a_producer_video_qt_plugin_def_t = &w4a_producer_video_qt_plugin_def_s;

#endif /*W4A_UNDER_MAC  */