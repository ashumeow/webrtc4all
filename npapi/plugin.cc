/*
* Copyright (C) 2012 Doubango Telecom <http://www.doubango.org>
*	
* This file is part of Open Source webrtc4all project <http://code.google.com/p/webrtc4all/>
*
* webrtc4all is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as publishd by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*	
* webrtc4all is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*	
* You should have received a copy of the GNU General Public License
* along with webrtc4ie.
*/
#include "plugin.h"
#include "Display.h"// FIXME
#include "WebRtc4npapi.h" // FIXME

#include "tsk_string.h"

#include <stdlib.h>
#include <string>
#include <stdio.h>

#define kPluginName					"WebRTC extension for Safari, Opera, Firefox and IE"
#define kPluginDescription			kPluginName " (Doubango Telecom)"
const char* kPluginVersion = THIS_VERSION;

#define kMimeTypeWebRtc4all			"application/w4a"
#define kMimeTypePeerConnection		"application/w4a-peerconnection"
#define kMimeTypeSdp				"application/w4a-sdp"
#define kMimeTypeNetwork			"application/w4a-network"
#define kMimeTypeDisplay			"application/w4a-display"


NPNetscapeFuncs* BrowserFuncs = NULL;

extern NPClass WebRtc4npapiClass;
extern NPClass PeerConnectionClass;
extern NPClass NetTransportClass;
extern NPClass SessionDescriptionClass;
extern NPClass DisplayClass;

typedef enum PluginType_e
{
	PluginType_WebRtc4npapi,
	PluginType_PeerConnection,
	PluginType_Sdp,
	PluginType_NetTransport,
	PluginType_Display
}
PluginType_t;

typedef struct InstanceData_s
{
	PluginType_t type;
	NPP npp;
	NPWindow window;
	NPObject* object;
#if W4A_UNDER_APPLE
    CALayer *rootLayer;
#endif
} 
InstanceData_t;

NPError OSCALL
NP_Initialize(NPNetscapeFuncs* browserFuncs)
{
  /* Save the browser function table. */
  BrowserFuncs = browserFuncs;

  return NPERR_NO_ERROR;
}

NPError OSCALL
NP_GetEntryPoints(NPPluginFuncs* pluginFuncs)
{
  /* Check the size of the provided structure based on the offset of the
     last member we need. */
  if (pluginFuncs->size < (offsetof(NPPluginFuncs, setvalue) + sizeof(void*)))
    return NPERR_INVALID_FUNCTABLE_ERROR;

  pluginFuncs->newp = NPP_New;
  pluginFuncs->destroy = NPP_Destroy;
  pluginFuncs->setwindow = NPP_SetWindow;
  pluginFuncs->newstream = NPP_NewStream;
  pluginFuncs->destroystream = NPP_DestroyStream;
  pluginFuncs->asfile = NPP_StreamAsFile;
  pluginFuncs->writeready = NPP_WriteReady;
  pluginFuncs->write = (NPP_WriteProcPtr)NPP_Write;
  pluginFuncs->print = NPP_Print;
  pluginFuncs->event = NPP_HandleEvent;
  pluginFuncs->urlnotify = NPP_URLNotify;
  pluginFuncs->getvalue = NPP_GetValue;
  pluginFuncs->setvalue = NPP_SetValue;

  return NPERR_NO_ERROR;
}

char*
NP_GetPluginVersion()
{
  return (char*)kPluginVersion;
}

const char*
NP_GetMIMEDescription()
{
  return "application/x-w4a WebRtc4all";
}

NP_EXPORT(NPError)
NP_GetValue(void* future, NPPVariable aVariable, void* aValue) {
  switch (aVariable) {
    case NPPVpluginNameString:
      *((char**)aValue) = kPluginName;
      break;
    case NPPVpluginDescriptionString:
      *((char**)aValue) = kPluginDescription;
      break;
    default:
      return NPERR_INVALID_PARAM;
      break;
  }
  return NPERR_NO_ERROR;
}

NP_EXPORT(NPError) OSCALL
NP_Shutdown()
{
  return NPERR_NO_ERROR;
}

NPError
NPP_New(NPMIMEType pluginType, NPP instance, uint16_t mode, int16_t argc, char* argn[], char* argv[], NPSavedData* saved) {
#if 0
  // Make sure we can render this plugin
  NPBool browserSupportsWindowless = false;
  BrowserFuncs->getvalue(instance, NPNVSupportsWindowless, &browserSupportsWindowless);
  if (!browserSupportsWindowless) {
    TSK_DEBUG_ERROR("Windowless mode not supported by the browser");
    return NPERR_GENERIC_ERROR;
  }
#endif
    
#if W4A_UNDER_APPLE
    // Ask the browser if it supports the Core Animation drawing model
    NPBool supportsCoreAnimation;
    if (BrowserFuncs->getvalue(instance, NPNVsupportsCoreAnimationBool, &supportsCoreAnimation) != NPERR_NO_ERROR) {
        supportsCoreAnimation = FALSE;
    }
    
    if (!supportsCoreAnimation) {
        TSK_DEBUG_ERROR("CoreAnimation not supported");
        return NPERR_INCOMPATIBLE_VERSION_ERROR;
    }
    
    // If the browser supports the Core Animation drawing model, enable it.
    BrowserFuncs->setvalue(instance, NPPVpluginDrawingModel, (void *)NPDrawingModelCoreAnimation);
    
    // If the browser supports the Cocoa event model, enable it.
    NPBool supportsCocoa;
    if (BrowserFuncs->getvalue(instance, NPNVsupportsCocoaBool, &supportsCocoa) != NPERR_NO_ERROR) {
        supportsCocoa = FALSE;
    }
    
    if (!supportsCocoa) {
        TSK_DEBUG_ERROR("Cocoa not supported...but not required");
        // return NPERR_INCOMPATIBLE_VERSION_ERROR;
    }
    else {
        BrowserFuncs->setvalue(instance, NPPVpluginEventModel, (void *)NPEventModelCocoa);
    }
    
#endif

  // set up our our instance data
  InstanceData_t* instanceData = (InstanceData_t*)malloc(sizeof(InstanceData_t));
  if (!instanceData){
    return NPERR_OUT_OF_MEMORY_ERROR;
  }

  memset(instanceData, 0, sizeof(InstanceData_t));
  instanceData->npp = instance;
  instance->pdata = instanceData;

  if(!tsk_stricmp(pluginType, kMimeTypeWebRtc4all)){
	  instanceData->type = PluginType_WebRtc4npapi;
  }
  else if(!tsk_stricmp(pluginType, kMimeTypePeerConnection)){
	  instanceData->type = PluginType_PeerConnection;
  }
  else if(!tsk_stricmp(pluginType, kMimeTypeNetwork)){
	  instanceData->type = PluginType_NetTransport;
  }
  else if(!tsk_stricmp(pluginType, kMimeTypeDisplay)){
	  instanceData->type = PluginType_Display;
  }
  else if(!tsk_stricmp(pluginType, kMimeTypeSdp)){
	  instanceData->type = PluginType_Sdp;
  }
  else{
	  return NPERR_GENERIC_ERROR;
  }

  bool bWindowed = (instanceData->type == PluginType_WebRtc4npapi || instanceData->type == PluginType_Display);
  BrowserFuncs->setvalue(instance, NPPVpluginWindowBool, (void*)bWindowed);

  return NPERR_NO_ERROR;
}

NPError
NPP_Destroy(NPP instance, NPSavedData** save) {
    InstanceData_t* instanceData = (InstanceData_t*)(instance->pdata);
    if (instanceData->object) {
        NP_OBJECT_RELEASE(instanceData->object);
    }
#if W4A_UNDER_APPLE
    [instanceData->rootLayer release], instanceData->rootLayer = NULL;
#endif
    free(instanceData);
    return NPERR_NO_ERROR;
}

NPError
NPP_SetWindow(NPP instance, NPWindow* window) {
  InstanceData_t* instanceData = (InstanceData_t*)(instance->pdata);
  instanceData->window = *window;
    if (instanceData->window.window) {
        TSK_DEBUG_INFO("Got window");
    }
  return NPERR_NO_ERROR;
}

NPError
NPP_NewStream(NPP instance, NPMIMEType type, NPStream* stream, NPBool seekable, uint16_t* stype) {
  return NPERR_GENERIC_ERROR;
}

NPError
NPP_DestroyStream(NPP instance, NPStream* stream, NPReason reason) {
  return NPERR_GENERIC_ERROR;
}

int32_t
NPP_WriteReady(NPP instance, NPStream* stream) {
  return 0;
}

int32_t
NPP_Write(NPP instance, NPStream* stream, int32_t offset, int32_t len, void* buffer) {
  return 0;
}

void
NPP_StreamAsFile(NPP instance, NPStream* stream, const char* fname) {

}

void
NPP_Print(NPP instance, NPPrint* platformPrint) {

}

int16_t
NPP_HandleEvent(NPP instance, void* event) {
  InstanceData_t *instanceData = (InstanceData_t*)(instance->pdata);
  return 1;
}

void
NPP_URLNotify(NPP instance, const char* URL, NPReason reason, void* notifyData) {

}

//FIXME
#if W4A_UNDER_APPLE
CALayer *__layerDisplayProducer = NULL;
CALayer *__layerDisplayConsumer = NULL;
#endif

NPError
NPP_GetValue(NPP instance, NPPVariable variable, void *value) {
    if (!instance) {
        TSK_DEBUG_ERROR("Invalid argument");
        return NPERR_INVALID_INSTANCE_ERROR;
    }
    InstanceData_t* instanceData = (InstanceData_t*)(instance->pdata);
    
	switch (variable) {
	  default:
        {
            return NPERR_GENERIC_ERROR;
        }
	  case NPPVpluginNameString:
        {
			*((char **)value) = kPluginName;
            break;
        }
	  case NPPVpluginDescriptionString:
        {
			*((char **)value) = kPluginDescription;
            break;
        }
#if W4A_UNDER_APPLE
        case NPPVpluginCoreAnimationLayer:
        {
            if (!instanceData->rootLayer && instanceData->type == PluginType_Display) {
                instanceData->rootLayer = [[CALayer layer] retain];
                instanceData->rootLayer.autoresizingMask = kCALayerWidthSizable | kCALayerHeightSizable;
                *(CALayer **)value = instanceData->rootLayer;
            }
            break;
        }
#endif
	  case NPPVpluginScriptableNPObject: {
		  if(!instanceData->object){
			  NPClass *aClass = NULL;
			  switch(instanceData->type){
					case PluginType_WebRtc4npapi: aClass = &WebRtc4npapiClass; break;
					case PluginType_PeerConnection: aClass = &PeerConnectionClass; break;
					case PluginType_NetTransport: aClass = &NetTransportClass; break;
					case PluginType_Sdp: aClass = &SessionDescriptionClass; break;
					case PluginType_Display: aClass = &DisplayClass; break;
			  }
			  if(!(instanceData->object = BrowserFuncs->createobject(instanceData->npp, aClass))){
				  return NPERR_OUT_OF_MEMORY_ERROR;
			  }
			  
			  if (instanceData->type == PluginType_WebRtc4npapi) {
				  if (!((WebRtc4npapi*)instanceData->object)->SetWindow(&instanceData->window, true)) {
					  return NPERR_GENERIC_ERROR;
				  }
			  }

#if W4A_UNDER_APPLE
              if (instanceData->type == PluginType_Display) {
                  if (instanceData->rootLayer) {
                      [instanceData->rootLayer setBounds: CGRectMake(instanceData->window.x, instanceData->window.y, instanceData->window.width, instanceData->window.height)];
                  }
                  ((Display*)instanceData->object)->setRootLayer(instanceData->rootLayer);
                  // FIXME
                  if (instanceData->window.width > 0) {
                      if (instanceData->window.width > 90) {
                          __layerDisplayConsumer = instanceData->rootLayer;
                          //__layerDisplayConsumer.backgroundColor = CGColorCreateGenericRGB(255.0, 0.0, 0.0, 1.0);
                      }
                      else {
                          __layerDisplayProducer = instanceData->rootLayer;
                          //__layerDisplayProducer.backgroundColor = CGColorCreateGenericRGB(0.0, 255.0, 0.0, 1.0);
                      }
                  }
              }
#endif
		  }
		  *(NPObject **)value = BrowserFuncs->retainobject(instanceData->object);
		}
		break;
	  case NPPVpluginNeedsXEmbed:
        {
            *((char *)value) = 1;
            break;
        }
  }
  return NPERR_NO_ERROR;
}

NPError
NPP_SetValue(NPP instance, NPNVariable variable, void *value) {
  return NPERR_GENERIC_ERROR;
}

