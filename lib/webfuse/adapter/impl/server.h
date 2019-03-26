#ifndef WF_ADAPTER_IMPL_SERVER_H
#define WF_ADAPTER_IMPL_SERVER_H

#ifdef __cplusplus
extern "C"
{
#endif

struct wf_server;
struct wf_server_config;

extern struct wf_server * wf_impl_server_create(
    struct wf_server_config * config);

extern void wf_impl_server_dispose(
    struct wf_server * server);

extern void wf_impl_server_run(
    struct wf_server * server);

extern void wf_impl_server_shutdown(
    struct wf_server * server);

#ifdef __cplusplus
}
#endif


#endif
