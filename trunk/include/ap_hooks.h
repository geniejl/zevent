#ifndef AP_HOOKS_H
#define AP_HOOKS_H

#include "server.h"
#include "apr_hooks.h"
#include "apr_network_io.h"

#ifdef __cplusplus
extern "C" {
#endif
	AP_DECLARE_HOOK(int,process_connection,(conn_state_t *cs))
#ifdef __cplusplus
}
#endif
#endif
