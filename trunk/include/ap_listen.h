/**
 * @file  ap_listen.h
 * @brief Apache Listeners Library
 *
 * @defgroup APACHE_CORE_LISTEN Apache Listeners Library
 * @ingroup  APACHE_CORE
 * @{
 */

#ifndef AP_LISTEN_H
#define AP_LISTEN_H

#include "apr_network_io.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ap_listen_rec ap_listen_rec;
typedef apr_status_t (*accept_function)(void **csd, ap_listen_rec *lr, apr_pool_t *ptrans);

/**
 * @brief Apache's listeners record.  
 *
 * These are used in the Multi-Processing Modules
 * to setup all of the sockets for the MPM to listen to and accept on.
 */
struct ap_listen_rec {
    /**
     * The next listener in the list
     */
    ap_listen_rec *next;
    /**
     * The actual socket 
     */
    apr_socket_t *sd;
    /**
     * The sockaddr the socket should bind to
     */
    apr_sockaddr_t *bind_addr;
    /**
     * The accept function for this socket
     */
    accept_function accept_func;
    /**
     * Is this socket currently active 
     */
    int active;
    /**
     * The default protocol for this listening socket.
     */
    const char* protocol;
    int prototype;
};

/**
 * The global list of ap_listen_rec structures
 */
AP_DECLARE_DATA extern ap_listen_rec *ap_listeners;

#ifdef __cplusplus
}
#endif

#endif
/** @} */
