#ifndef AP_LOG_H
#define AP_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "apr_thread_proc.h"
#include "zevent_config.h"

#define APLOG_MARK	__FILE__,__LINE__

AP_DECLARE(apr_status_t) ap_open_log(apr_pool_t *p,const char *filename);

AP_DECLARE(apr_status_t) ap_open_stderr_log(apr_pool_t *p);

AP_DECLARE(apr_status_t) ap_replace_stderr_log(apr_pool_t *p,
                                               const char *filename);
AP_DECLARE(void) ap_log_error(const char *file, 
		             int line,
                             apr_pool_t *p, 
                             const char *fmt, ...)
			    __attribute__((format(printf,4,5)));


AP_DECLARE(void) ap_log_close();
#ifdef __cplusplus
}
#endif

#endif
