#ifndef AP_HOOKS_H
#define AP_HOOKS_H

#include "ap_config.h"
#include "server.h"

#ifdef __cplusplus
extern "C" {
#endif
	AP_DECLARE_HOOK(void,zevent_init,(apr_pool_t *pzevent))
	AP_DECLARE_HOOK(void,zevent_fini,(apr_pool_t *pzevent))
	AP_DECLARE_HOOK(void,child_init,(apr_pool_t *pchild))
	AP_DECLARE_HOOK(void,child_fini,(apr_pool_t *pchild))
	AP_DECLARE_HOOK(int,process_connection,(conn_state_t *cs))
#ifdef __cplusplus
}
#endif
#endif
