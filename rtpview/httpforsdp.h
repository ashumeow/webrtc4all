#pragma once

// this code is primarily derived/adapted from the tinyHTTP functional test code in ...\doubango\tinyHTTP\test\test_transport.h
#include "tinyhttp.h"

#define HTTP_FOR_SDP_MIN_STREAM_CHUNK_SIZE    0x32

/************************************************
* thttp_for_sdp_peer_t
************************************************/
typedef struct thttp_for_sdp_peer_s
{
    TSK_DECLARE_OBJECT;
    tnet_fd_t fd;
    tsk_buffer_t* buff;
}
thttp_for_sdp_peer_t;
typedef tsk_list_t thttp_for_sdp_peers_L_t;

static tsk_object_t* thttp_for_sdp_peer_ctor(tsk_object_t * self, va_list * app) { return self; }
static tsk_object_t* thttp_for_sdp_peer_dtor(tsk_object_t * self)
{ 
    thttp_for_sdp_peer_t *peer = (thttp_for_sdp_peer_t *) self;
    if (peer) {
        TSK_OBJECT_SAFE_FREE(peer->buff);
    }

    return self;
}
static const tsk_object_def_t thttp_for_sdp_peer_def_s = 
{
    sizeof(thttp_for_sdp_peer_t),
    thttp_for_sdp_peer_ctor, 
    thttp_for_sdp_peer_dtor,
    tsk_null, 
};

static thttp_for_sdp_peer_t* thttp_for_sdp_peer_create(tnet_fd_t fd)
{
    thttp_for_sdp_peer_t* peer = (thttp_for_sdp_peer_t *) tsk_object_new(&thttp_for_sdp_peer_def_s);
    peer->fd = fd;
    peer->buff = tsk_buffer_create_null();
    return peer;
}

static int thttp_for_sdp_peer_pred_fd_cmp(const tsk_list_item_t* item, const void* data)
{
    return ((thttp_for_sdp_peer_t*)item->data)->fd - *((const tnet_fd_t*)data);
}


/************************************************
* thttp_for_sdp_transport_t
************************************************/
typedef struct thttp_for_sdp_transport_s
{
    TSK_DECLARE_OBJECT;

    thttp_for_sdp_peers_L_t* peers;
    tnet_transport_handle_t* handle;
}
thttp_for_sdp_transport_t;

static tsk_object_t* thttp_for_sdp_transport_ctor(tsk_object_t * self, va_list * app)
{ 
    thttp_for_sdp_transport_t *transport = (thttp_for_sdp_transport_t *) self;
    if (transport) {
    }
    return self; 
}
static tsk_object_t* thttp_for_sdp_transport_dtor(tsk_object_t * self)
{ 
    thttp_for_sdp_transport_t *transport = (thttp_for_sdp_transport_t *) self;
    if(transport){
        TSK_OBJECT_SAFE_FREE(transport->handle);
        TSK_OBJECT_SAFE_FREE(transport->peers);
    }

    return self;
}
static const tsk_object_def_t thttp_for_sdp_transport_def_s = 
{
    sizeof(thttp_for_sdp_transport_t),
    thttp_for_sdp_transport_ctor, 
    thttp_for_sdp_transport_dtor,
    tsk_null, 
};

static int thttp_for_sdp_transport_callback(const tnet_transport_event_t* e); // forward declaration

static thttp_for_sdp_transport_t* thttp_for_sdp_transport_create(const char* local_ip, tnet_port_t local_port, enum tnet_socket_type_e socket_type)
{
    thttp_for_sdp_transport_t *transport = (thttp_for_sdp_transport_t *) tsk_object_new(&thttp_for_sdp_transport_def_s);
    if (transport) {
        transport->peers = tsk_list_create();
        transport->handle = tnet_transport_create(local_ip, local_port, socket_type, "HTTP Transport");
        if (!transport->handle) {
            TSK_OBJECT_SAFE_FREE(transport);
            return tsk_null;
        }
        tnet_transport_set_callback(transport->handle, thttp_for_sdp_transport_callback, transport);
    }
    return transport;
}

static int thttp_for_sdp_transport_start(thttp_for_sdp_transport_t* self)
{
    if (self) {
        return tnet_transport_start(self->handle);
    }
    return -1;
}

static thttp_for_sdp_peer_t* thttp_for_sdp_transport_connect_to(thttp_for_sdp_transport_t* self, const char* dst_host, tnet_port_t dst_port)
{
    tnet_fd_t fd = tnet_transport_connectto_2(self->handle, dst_host, dst_port);
    if (fd > 0) {
        return thttp_for_sdp_peer_create(fd);  // Up to the caller to free the returned object using TSK_OBJECT_SAFREE(peer)
    }
    return tsk_null;
}

static tsk_size_t thttp_for_sdp_transport_send_data(thttp_for_sdp_transport_t* self, tnet_fd_t fd, const void* data_ptr, tsk_size_t data_size)
{
    if (self && self->handle && data_ptr && data_size) {
        return tnet_transport_send(self->handle, fd, data_ptr, data_size);
    }
    return 0;
}

static void thttp_for_sdp_transport_add_peer(thttp_for_sdp_transport_t* self, thttp_for_sdp_peer_t* peer)
{
    tsk_list_lock(self->peers);
    tsk_list_push_back_data(self->peers, (tsk_object_t**) &peer);
    tsk_list_unlock(self->peers);
}

static void thttp_for_sdp_transport_remove_peer_by_fd(thttp_for_sdp_transport_t* self, tnet_fd_t fd)
{
    tsk_list_lock(self->peers);
    tsk_list_remove_item_by_pred(self->peers, thttp_for_sdp_peer_pred_fd_cmp, &fd);
    tsk_list_unlock(self->peers);
}

static const thttp_for_sdp_peer_t* thttp_for_sdp_transport_find_peer_by_fd(thttp_for_sdp_transport_t* self, tnet_fd_t fd)
{
    const tsk_list_item_t* item;
    tsk_list_lock(self->peers);
    item = tsk_list_find_item_by_pred(self->peers, thttp_for_sdp_peer_pred_fd_cmp, &fd);
    tsk_list_unlock(self->peers);
    if (item) {
        return (const thttp_for_sdp_peer_t*)item->data;
    }
    return tsk_null;
}

static void tHttpForSdpTransportProcessMsgFromCallback(thttp_for_sdp_transport_t* self, const thttp_message_t* msg, tnet_fd_t fd);  // forward declaration

static int thttp_for_sdp_transport_callback(const tnet_transport_event_t* e)
{
    thttp_for_sdp_transport_t* transport = (thttp_for_sdp_transport_t*)e->callback_data;
    const thttp_for_sdp_peer_t* _peer = tsk_null;
    thttp_message_t *message = tsk_null;
    int endOfheaders = -1;
    tsk_ragel_state_t state;
    tsk_bool_t have_all_content = tsk_false;
    int ret = 0;
    switch (e->type)
    {
        case event_closed:
            {
                thttp_for_sdp_transport_remove_peer_by_fd(transport, e->local_fd);
                return 0;
            }

        case event_connected:
        case event_accepted:
            {
                _peer = thttp_for_sdp_transport_find_peer_by_fd(transport, e->local_fd);
                if (!_peer) {
                    _peer = thttp_for_sdp_peer_create(e->local_fd);
                    thttp_for_sdp_transport_add_peer(transport, (thttp_for_sdp_peer_t*)_peer);
                }
                return 0;
            }

        case event_data:
            {
                TSK_DEBUG_INFO("\n\nRECV: %.*s\n\n", e->size, (const char*)e->data);
                break;
            }
        default:
            return 0;
    }


    _peer = thttp_for_sdp_transport_find_peer_by_fd(transport, e->local_fd);
    if(!_peer) {
        TSK_DEBUG_ERROR("Data event but no peer found!");
        return -1;
    }

    /* Append new content. */
    tsk_buffer_append(_peer->buff, e->data, e->size);

    /* Check if we have all HTTP headers. */
parse_buffer:
    if ((endOfheaders = tsk_strindexOf((const char *)TSK_BUFFER_DATA(_peer->buff), TSK_BUFFER_SIZE(_peer->buff), "\r\n\r\n"/*2CRLF*/)) < 0) {
        TSK_DEBUG_INFO("No all HTTP headers in the TCP buffer.");
        goto bail;
    }
    /* If we are here this mean that we have all HTTP headers.
    *    ==> Parse the HTTP message without the content.
    */
    tsk_ragel_state_init(&state, (const char *) TSK_BUFFER_DATA(_peer->buff), endOfheaders + 4/*2CRLF*/);
    if (!(ret = thttp_message_parse(&state, &message, tsk_false/* do not extract the content */))) {
        const thttp_header_Transfer_Encoding_t* transfer_Encoding;

        /* chunked? */
        if((transfer_Encoding = (const thttp_header_Transfer_Encoding_t*)thttp_message_get_header(message, thttp_htype_Transfer_Encoding)) && tsk_striequals(transfer_Encoding->encoding, "chunked")){
            const char* start = (const char*)(TSK_BUFFER_TO_U8(_peer->buff) + (endOfheaders + 4/*2CRLF*/));
            const char* end = (const char*)(TSK_BUFFER_TO_U8(_peer->buff) + TSK_BUFFER_SIZE(_peer->buff));
            int index;

            TSK_DEBUG_INFO("CHUNKED transfer.");
            while(start < end){
                /* RFC 2616 - 19.4.6 Introduction of Transfer-Encoding */
                // read chunk-size, chunk-extension (if any) and CRLF
                tsk_size_t chunk_size = (tsk_size_t)tsk_atox(start);
                if((index = tsk_strindexOf(start, (tsk_size_t)(end-start), "\r\n")) >=0){
                    start += index + 2/*CRLF*/;
                }
                else{
                    TSK_DEBUG_INFO("Parsing chunked data has failed.");
                    break;
                }

                if(chunk_size == 0 && ((start + 2) <= end) && *start == '\r' && *(start+ 1) == '\n'){
                    tsk_size_t parsed_len = (tsk_size_t)(start - (const char*)(TSK_BUFFER_TO_U8(_peer->buff))) + 2/*CRLF*/;
                    tsk_buffer_remove(_peer->buff, 0, parsed_len);
                    have_all_content = tsk_true;
                    break;
                }
                    
                thttp_message_append_content(message, start, chunk_size);
                start += chunk_size + 2/*CRLF*/;
            }
        }
        else{
            tsk_size_t clen = THTTP_MESSAGE_CONTENT_LENGTH(message); /* MUST have content-length header. */
            if(clen == 0){ /* No content */
                tsk_buffer_remove(_peer->buff, 0, (endOfheaders + 4/*2CRLF*/)); /* Remove HTTP headers and CRLF ==> must never happen */
                have_all_content = tsk_true;
            }
            else{ /* There is a content */
                if((endOfheaders + 4/*2CRLF*/ + clen) > TSK_BUFFER_SIZE(_peer->buff)){ /* There is content but not all the content. */
                    TSK_DEBUG_INFO("No all HTTP content in the TCP buffer.");
                    goto bail;
                }
                else{
                    /* Add the content to the message. */
                    thttp_message_add_content(message, tsk_null, TSK_BUFFER_TO_U8(_peer->buff) + endOfheaders + 4/*2CRLF*/, clen);
                    /* Remove HTTP headers, CRLF and the content. */
                    tsk_buffer_remove(_peer->buff, 0, (endOfheaders + 4/*2CRLF*/ + clen));
                    have_all_content = tsk_true;
                }
            }
        }
    }
    /* Alert the dialog (FSM) */
    if(message){
        if (have_all_content) { /* only if we have all data */
            tHttpForSdpTransportProcessMsgFromCallback(transport, message, e->local_fd);
            /* Parse next chunck */
            if (TSK_BUFFER_SIZE(_peer->buff) >= HTTP_FOR_SDP_MIN_STREAM_CHUNK_SIZE) {
                TSK_OBJECT_SAFE_FREE(message);
                goto parse_buffer;
            }
        }
    }

bail:
    TSK_OBJECT_SAFE_FREE(message);

    return 0;
}

