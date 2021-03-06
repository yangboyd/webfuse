#include "webfuse/impl/session.h"
#include "webfuse/impl/authenticators.h"
#include "webfuse/impl/message_queue.h"
#include "webfuse/impl/message.h"
#include "webfuse/impl/mountpoint_factory.h"
#include "webfuse/impl/mountpoint.h"

#include "webfuse/impl/util/container_of.h"
#include "webfuse/impl/util/util.h"

#include "webfuse/impl/jsonrpc/proxy.h"
#include "webfuse/impl/jsonrpc/request.h"
#include "webfuse/impl/jsonrpc/response.h"
#include "webfuse/impl/json/doc.h"

#include <libwebsockets.h>
#include <stddef.h>
#include <stdlib.h>

#define WF_DEFAULT_TIMEOUT (10 * 1000)
#define WF_DEFAULT_MESSAGE_SIZE (8 * 1024)

static bool wf_impl_session_send(
    struct wf_message * message,
    void * user_data)
{
    struct wf_impl_session * session = user_data;
    bool result = false;

    if (NULL != session->wsi)
    {
        wf_impl_slist_append(&session->messages, &message->item);
        lws_callback_on_writable(session->wsi);

        result = true;
    }
    else
    {
        wf_impl_message_dispose(message);
    }

    return result;
}

struct wf_impl_session * wf_impl_session_create(
    struct lws * wsi,
    struct wf_impl_authenticators * authenticators,
    struct wf_timer_manager * timer_manager,
    struct wf_jsonrpc_server * server,
    struct wf_impl_mountpoint_factory * mountpoint_factory)
{

    struct wf_impl_session * session = malloc(sizeof(struct wf_impl_session));
    wf_impl_slist_init(&session->filesystems);
    
    session->wsi = wsi;
    session->is_authenticated = false;
    session->authenticators = authenticators;
    session->server = server;
    session->mountpoint_factory = mountpoint_factory;
    session->rpc = wf_impl_jsonrpc_proxy_create(timer_manager, WF_DEFAULT_TIMEOUT, &wf_impl_session_send, session);
    wf_impl_slist_init(&session->messages);
    wf_impl_buffer_init(&session->recv_buffer, WF_DEFAULT_MESSAGE_SIZE);

    return session;
}

static void wf_impl_session_dispose_filesystems(
    struct wf_slist * filesystems)
{    
    struct wf_slist_item * item = wf_impl_slist_first(filesystems);
    while (NULL != item)
    {
        struct wf_slist_item * next = item->next;
        struct wf_impl_filesystem * filesystem = wf_container_of(item, struct wf_impl_filesystem, item);
        wf_impl_filesystem_dispose(filesystem);
        
        item = next;
    }
}

void wf_impl_session_dispose(
    struct wf_impl_session * session)
{
    wf_impl_jsonrpc_proxy_dispose(session->rpc);
    wf_impl_message_queue_cleanup(&session->messages);

    wf_impl_session_dispose_filesystems(&session->filesystems);
    wf_impl_buffer_cleanup(&session->recv_buffer);
    free(session);
} 

bool wf_impl_session_authenticate(
    struct wf_impl_session * session,
    struct wf_credentials * creds)
{
    session->is_authenticated = wf_impl_authenticators_authenticate(session->authenticators, creds);

    return session->is_authenticated;
}

bool wf_impl_session_add_filesystem(
    struct wf_impl_session * session,
    char const * name)
{
    bool result;

    struct wf_mountpoint * mountpoint = wf_impl_mountpoint_factory_create_mountpoint(session->mountpoint_factory, name);
    result = (NULL != mountpoint);
 
    if (result)
    {
        struct wf_impl_filesystem * filesystem = wf_impl_filesystem_create(session->wsi, session->rpc, name, mountpoint);
        result = (NULL != filesystem);
        if (result)
        {
            wf_impl_slist_append(&session->filesystems, &filesystem->item);
        }
    }
    
    // cleanup on error
    if (!result)
    {
        if (NULL != mountpoint)
        {
            wf_impl_mountpoint_dispose(mountpoint);
        }
    }

    return result;
}


void wf_impl_session_onwritable(
    struct wf_impl_session * session)
{
    if (!wf_impl_slist_empty(&session->messages))
    {
        struct wf_slist_item * item = wf_impl_slist_remove_first(&session->messages);                
        struct wf_message * message = wf_container_of(item, struct wf_message, item);
        lws_write(session->wsi, (unsigned char*) message->data, message->length, LWS_WRITE_TEXT);
        wf_impl_message_dispose(message);

        if (!wf_impl_slist_empty(&session->messages))
        {
            lws_callback_on_writable(session->wsi);
        }
    }
}

static void wf_impl_session_process(
    struct wf_impl_session * session,
    char * data,
    size_t length)
{
    struct wf_json_doc * doc = wf_impl_json_doc_loadb(data, length);
    if (NULL != doc)
    {
        struct wf_json const * message = wf_impl_json_doc_root(doc);
        if (wf_impl_jsonrpc_is_response(message))
        {
            wf_impl_jsonrpc_proxy_onresult(session->rpc, message);
        }
        else if (wf_impl_jsonrpc_is_request(message))
        {
            wf_impl_jsonrpc_server_process(session->server, message, &wf_impl_session_send, session);
        }

        wf_impl_json_doc_dispose(doc);
    }
}

void wf_impl_session_receive(
    struct wf_impl_session * session,
    char * data,
    size_t length,
    bool is_final_fragment)
{
    if (is_final_fragment)
    {
        if (wf_impl_buffer_is_empty(&session->recv_buffer))
        {
            wf_impl_session_process(session, data, length);
        }
        else
        {
            wf_impl_buffer_append(&session->recv_buffer, data, length);
            wf_impl_session_process(session,
                wf_impl_buffer_data(&session->recv_buffer),
                wf_impl_buffer_size(&session->recv_buffer));
            wf_impl_buffer_clear(&session->recv_buffer);
        }        
    }
    else
    {
        wf_impl_buffer_append(&session->recv_buffer, data, length);
    }
}

static struct wf_impl_filesystem * wf_impl_session_get_filesystem(
    struct wf_impl_session * session,
    struct lws * wsi)
{
    struct wf_impl_filesystem * result = NULL;

    struct wf_slist_item * item = wf_impl_slist_first(&session->filesystems);
    while (NULL != item)
    {
        struct wf_slist_item * next = item->next;
        struct wf_impl_filesystem * filesystem = wf_container_of(item, struct wf_impl_filesystem, item);
        if (wsi == filesystem->wsi)
        {
            result = filesystem;
            break;
        }

        item = next;
    }

    return result;
}


bool wf_impl_session_contains_wsi(
    struct wf_impl_session * session,
    struct lws * wsi)
{
    bool const result = (NULL != wsi) && ((wsi == session->wsi) || (NULL != wf_impl_session_get_filesystem(session, wsi)));
    return result;
}


void wf_impl_session_process_filesystem_request(
    struct wf_impl_session * session, 
    struct lws * wsi)
{
    struct wf_impl_filesystem * filesystem = wf_impl_session_get_filesystem(session, wsi);
    if (NULL != filesystem)
    {
        wf_impl_filesystem_process_request(filesystem);
    }
}
