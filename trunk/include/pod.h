/**
 * @file  event/pod.h
 * @brief pod definitions
 *
 * @addtogroup APACHE_MPM_EVENT
 * @{
 */

#include "apr.h"
#include "apr_strings.h"
#define APR_WANT_STRFUNC
#include "apr_want.h"

#include "mpm.h"
#include "mpm_common.h"
#include "ap_mpm.h"
#include "ap_listen.h"
#include "mpm_default.h"

#define RESTART_CHAR '$'
#define GRACEFUL_CHAR '!'

#define AP_RESTART  0
#define AP_GRACEFUL 1

typedef struct ap_pod_t ap_pod_t;

struct ap_pod_t
{
    apr_file_t *pod_in;
    apr_file_t *pod_out;
    apr_pool_t *p;
};

AP_DECLARE(apr_status_t) ap_mpm_pod_open(apr_pool_t * p, ap_pod_t ** pod);
AP_DECLARE(int) ap_mpm_pod_check(ap_pod_t * pod);
AP_DECLARE(apr_status_t) ap_mpm_pod_close(ap_pod_t * pod);
AP_DECLARE(apr_status_t) ap_mpm_pod_signal(ap_pod_t * pod, int graceful);
AP_DECLARE(void) ap_mpm_pod_killpg(ap_pod_t * pod, int num, int graceful);
/** @} */
