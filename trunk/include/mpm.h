/**
 * @file event/mpm.h
 * @brief Unix Exent driven MPM
 *
 * @defgroup APACHE_MPM_EVENT Unix Event MPM
 * @ingroup  APACHE_OS_UNIX APACHE_MPM
 * @{
 */

#include "scoreboard.h"
#include "server.h"
#include "unixd.h"

#ifndef APACHE_MPM_EVENT_H
#define APACHE_MPM_EVENT_H

#define EVENT_MPM

#define MPM_NAME "Event"

#define AP_MPM_WANT_RECLAIM_CHILD_PROCESSES
#define AP_MPM_WANT_WAIT_OR_TIMEOUT
#define AP_MPM_WANT_PROCESS_CHILD_STATUS
#define AP_MPM_WANT_SET_PIDFILE
#define AP_MPM_WANT_SET_SCOREBOARD
#define AP_MPM_WANT_SET_LOCKFILE
#define AP_MPM_WANT_SET_MAX_REQUESTS
#define AP_MPM_WANT_SET_COREDUMPDIR
#define AP_MPM_WANT_SET_ACCEPT_LOCK_MECH
#define AP_MPM_WANT_SIGNAL_SERVER
#define AP_MPM_WANT_SET_MAX_MEM_FREE
#define AP_MPM_WANT_SET_STACKSIZE
#define AP_MPM_WANT_SET_GRACEFUL_SHUTDOWN
#define AP_MPM_WANT_FATAL_SIGNAL_HANDLER
#define AP_MPM_DISABLE_NAGLE_ACCEPTED_SOCK

#define MPM_CHILD_PID(i) (ap_scoreboard_image->parent[i].pid)
#define MPM_NOTE_CHILD_KILLED(i) (MPM_CHILD_PID(i) = 0)
#define MPM_ACCEPT_FUNC unixd_accept


extern int ap_threads_per_child;
extern int ap_max_daemons_limit;
extern char ap_coredump_dir[MAX_STRING_LEN];

#endif /* APACHE_MPM_EVENT_H */
/** @} */
