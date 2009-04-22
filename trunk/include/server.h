#ifndef AP_SERVERD_H
#define AP_SERVERD_H

#include "apr.h"
#include "apr_poll.h"
#include "apr_buckets.h"

#ifdef __cplusplus
extern "C"{
#endif

/** The default string length */
#define MAX_STRING_LEN HUGE_STRING_LEN

/** The length of a Huge string */
#define HUGE_STRING_LEN 8192

/** The size of the server's internal read-write buffers */
#define AP_IOBUFSIZE 8192

/** The max number of regex captures that can be expanded by ap_pregsub */
#define AP_MAX_REG_MATCH 10

/**
 *  * APR_HAS_LARGE_FILES introduces the problem of spliting sendfile into 
 *   * mutiple buckets, no greater than MAX(apr_size_t), and more granular 
 *    * than that in case the brigade code/filters attempt to read it directly.
 *     * ### 16mb is an invention, no idea if it is reasonable.
 *      */
#define AP_MAX_SENDFILE 16777216  /* 2^24 */

/**
 *  * Special Apache error codes. These are basically used
 *   *  in http_main.c so we can keep track of various errors.
 *    *        
 *     */
/** a normal exit */
#define APEXIT_OK 0x0
/** A fatal error arising during the server's init sequence */
#define APEXIT_INIT 0x2
/**  The child died during its init sequence */
#define APEXIT_CHILDINIT 0x3
/**  
 *  *   The child exited due to a resource shortage.
 *   *   The parent should limit the rate of forking until
 *    *   the situation is resolved.
 *     */
#define APEXIT_CHILDSICK        0x7
/** 
 *  *     A fatal error, resulting in the whole server aborting.
 *   *     If a child exits with this error, the parent process
 *    *     considers this a server-wide fatal error and aborts.
 *     */
#define APEXIT_CHILDFATAL 0xf

#ifndef AP_DECLARE
/**
 *  * Stuff marked #AP_DECLARE is part of the API, and intended for use
 *   * by modules. Its purpose is to allow us to add attributes that
 *    * particular platforms or compilers require to every exported function.
 *     */
# define AP_DECLARE(type)    type
#endif

#ifndef AP_DECLARE_NONSTD
/**
 *  * Stuff marked #AP_DECLARE_NONSTD is part of the API, and intended for
 *   * use by modules.  The difference between #AP_DECLARE and
 *    * #AP_DECLARE_NONSTD is that the latter is required for any functions
 *     * which use varargs or are used via indirect function call.  This
 *      * is to accomodate the two calling conventions in windows dlls.
 *       */
# define AP_DECLARE_NONSTD(type)    type
#endif
#ifndef AP_DECLARE_DATA
# define AP_DECLARE_DATA
#endif

#ifndef AP_MODULE_DECLARE
# define AP_MODULE_DECLARE(type)    type
#endif
#ifndef AP_MODULE_DECLARE_NONSTD
# define AP_MODULE_DECLARE_NONSTD(type)  type
#endif
#ifndef AP_MODULE_DECLARE_DATA
# define AP_MODULE_DECLARE_DATA
#endif

/**
 *  * @internal
 *   * modules should not use functions marked AP_CORE_DECLARE
 *    */
#ifndef AP_CORE_DECLARE
# define AP_CORE_DECLAREAP_DECLARE
#endif

/**
 *  * @internal
 *   * modules should not use functions marked AP_CORE_DECLARE_NONSTD
 *    */

#ifndef AP_CORE_DECLARE_NONSTD
# define AP_CORE_DECLARE_NONSTDAP_DECLARE_NONSTD
#endif
/** A structure that represents the status of the current connection */
typedef struct conn_state_t conn_state_t;

/** 
 *  * @brief A structure to contain connection state information 
 *   */
struct conn_state_t {
	/** memory pool to allocate from */
	apr_pool_t *p;
	/** poll file decriptor information */
	apr_pollfd_t *pfd;

	apr_pollset_t *pollset;

	apr_bucket_alloc_t *bain;
	apr_bucket_brigade *bbin;

	apr_bucket_alloc_t *baout;
	apr_bucket_brigade *bbout;
};

AP_DECLARE(void) ap_lingering_close(apr_socket_t *csd);

#define DECLINED -1/**< Module declines to handle */
#define DONE -2/**< Module has served the response completely */
#define OK 0

#define AP_NORESTART APR_OS_START_USEERR + 1

#ifdef __cplusplus
}
#endif

#endif
