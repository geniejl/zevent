
#ifndef APACHE_MPM_COMMON_H
#define APACHE_MPM_COMMON_H

#include "scoreboard.h"
#include "server.h"
#include "unixd.h"

#include "zevent_config.h"

#if APR_HAVE_NETINET_TCP_H
#include <netinet/tcp.h>    /* for TCP_NODELAY */
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* The maximum length of the queue of pending connections, as defined
 * by listen(2).  Under some systems, it should be increased if you
 * are experiencing a heavy TCP SYN flood attack.
 *
 * It defaults to 511 instead of 512 because some systems store it 
 * as an 8-bit datatype; 512 truncated to 8-bits is 0, while 511 is 
 * 255 when truncated.
 */
#ifndef DEFAULT_LISTENBACKLOG
#define DEFAULT_LISTENBACKLOG 511
#endif
        
/* Signal used to gracefully restart */
#define AP_SIG_GRACEFUL SIGUSR1

/* Signal used to gracefully restart (without SIG prefix) */
#define AP_SIG_GRACEFUL_SHORT USR1

/* Signal used to gracefully restart (as a quoted string) */
#define AP_SIG_GRACEFUL_STRING "SIGUSR1"

/* Signal used to gracefully stop */
#define AP_SIG_GRACEFUL_STOP SIGWINCH

/* Signal used to gracefully stop (without SIG prefix) */
#define AP_SIG_GRACEFUL_STOP_SHORT WINCH

/* Signal used to gracefully stop (as a quoted string) */
#define AP_SIG_GRACEFUL_STOP_STRING "SIGWINCH"

void ap_reclaim_child_processes(int terminate);

void ap_relieve_child_processes(void);

void ap_register_extra_mpm_process(pid_t pid);

int ap_unregister_extra_mpm_process(pid_t pid);

apr_status_t ap_mpm_safe_kill(pid_t pid, int sig);

void ap_wait_or_timeout(apr_exit_why_e *status, int *exitcode, apr_proc_t *ret, 
                        apr_pool_t *p);

int ap_process_child_status(apr_proc_t *pid, apr_exit_why_e why, int status);

#if defined(TCP_NODELAY) && !defined(MPE) && !defined(TPF)
/**
 * Turn off the nagle algorithm for the specified socket.  The nagle algorithm
 * says that we should delay sending partial packets in the hopes of getting
 * more data.  There are bad interactions between persistent connections and
 * Nagle's algorithm that have severe performance penalties.
 * @param s The socket to disable nagle for.
 */
void ap_sock_disable_nagle(apr_socket_t *s);
#else
#define ap_sock_disable_nagle(s)        /* NOOP */
#endif

AP_DECLARE(uid_t) ap_uname2id(const char *name);

AP_DECLARE(gid_t) ap_gname2id(const char *name);

#ifdef __cplusplus
}
#endif

#endif 
