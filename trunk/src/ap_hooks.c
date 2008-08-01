#include "ap_hooks.h"

APR_HOOK_STRUCT(
		APR_HOOK_LINK(child_init)
		APR_HOOK_LINK(process_connection)
	       )

AP_IMPLEMENT_HOOK_VOID(child_init,(apr_pool_t *pchild),(pchild))
AP_IMPLEMENT_HOOK_RUN_FIRST(int,process_connection,(conn_state_t *cs),(cs),DECLINED)

