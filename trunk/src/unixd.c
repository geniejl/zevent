/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ap_config.h"
#define CORE_PRIVATE
#include "unixd.h"
#include "mpm_common.h"
//#include "os.h"
#include "ap_mpm.h"
#include "apr_thread_proc.h"
#include "apr_strings.h"
#include "apr_portable.h"
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
/* XXX */
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_GRP_H
#include <grp.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#ifdef HAVE_SYS_SEM_H
#include <sys/sem.h>
#endif
#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#endif

AP_DECLARE(apr_status_t) unixd_accept(void **accepted, ap_listen_rec *lr,
                                        apr_pool_t *ptrans)
{
    apr_socket_t *csd;
    apr_status_t status;
#ifdef _OSD_POSIX
    int sockdes;
#endif

    *accepted = NULL;
    status = apr_socket_accept(&csd, lr->sd, ptrans);
    if (status == APR_SUCCESS) {
        *accepted = csd;
#ifdef _OSD_POSIX
        apr_os_sock_get(&sockdes, csd);
        if (sockdes >= FD_SETSIZE) {
            apr_socket_close(csd);
            return APR_EINTR;
        }
#endif
        return APR_SUCCESS;
    }

    if (APR_STATUS_IS_EINTR(status)) {
        return status;
    }
    /* Our old behaviour here was to continue after accept()
     * errors.  But this leads us into lots of troubles
     * because most of the errors are quite fatal.  For
     * example, EMFILE can be caused by slow descriptor
     * leaks (say in a 3rd party module, or libc).  It's
     * foolish for us to continue after an EMFILE.  We also
     * seem to tickle kernel bugs on some platforms which
     * lead to never-ending loops here.  So it seems best
     * to just exit in most cases.
     */
    switch (status) {
#if defined(HPUX11) && defined(ENOBUFS)
        /* On HPUX 11.x, the 'ENOBUFS, No buffer space available'
         * error occurs because the accept() cannot complete.
         * You will not see ENOBUFS with 10.20 because the kernel
         * hides any occurrence from being returned to user space.
         * ENOBUFS with 11.x's TCP/IP stack is possible, and could
         * occur intermittently. As a work-around, we are going to
         * ignore ENOBUFS.
         */
        case ENOBUFS:
#endif

#ifdef EPROTO
        /* EPROTO on certain older kernels really means
         * ECONNABORTED, so we need to ignore it for them.
         * See discussion in new-httpd archives nh.9701
         * search for EPROTO.
         *
         * Also see nh.9603, search for EPROTO:
         * There is potentially a bug in Solaris 2.x x<6,
         * and other boxes that implement tcp sockets in
         * userland (i.e. on top of STREAMS).  On these
         * systems, EPROTO can actually result in a fatal
         * loop.  See PR#981 for example.  It's hard to
         * handle both uses of EPROTO.
         */
        case EPROTO:
#endif
#ifdef ECONNABORTED
        case ECONNABORTED:
#endif
        /* Linux generates the rest of these, other tcp
         * stacks (i.e. bsd) tend to hide them behind
         * getsockopt() interfaces.  They occur when
         * the net goes sour or the client disconnects
         * after the three-way handshake has been done
         * in the kernel but before userland has picked
         * up the socket.
         */
#ifdef ECONNRESET
        case ECONNRESET:
#endif
#ifdef ETIMEDOUT
        case ETIMEDOUT:
#endif
#ifdef EHOSTUNREACH
        case EHOSTUNREACH:
#endif
#ifdef ENETUNREACH
        case ENETUNREACH:
#endif
        /* EAGAIN/EWOULDBLOCK can be returned on BSD-derived
         * TCP stacks when the connection is aborted before
         * we call connect, but only because our listener
         * sockets are non-blocking (AP_NONBLOCK_WHEN_MULTI_LISTEN)
         */
#ifdef EAGAIN
        case EAGAIN:
#endif
#ifdef EWOULDBLOCK
#if !defined(EAGAIN) || EAGAIN != EWOULDBLOCK
        case EWOULDBLOCK:
#endif
#endif
            break;
#ifdef ENETDOWN
        case ENETDOWN:
            /*
             * When the network layer has been shut down, there
             * is not much use in simply exiting: the parent
             * would simply re-create us (and we'd fail again).
             * Use the CHILDFATAL code to tear the server down.
             * @@@ Martin's idea for possible improvement:
             * A different approach would be to define
             * a new APEXIT_NETDOWN exit code, the reception
             * of which would make the parent shutdown all
             * children, then idle-loop until it detected that
             * the network is up again, and restart the children.
             * Ben Hyde noted that temporary ENETDOWN situations
             * occur in mobile IP.
             */
            return APR_EGENERAL;
#endif /*ENETDOWN*/

#ifdef TPF
        case EINACT:
            return APR_EGENERAL;
            break;
        default:
            return APR_EGENERAL;
#else
        default:
            return APR_EGENERAL;
#endif
    }
    return status;
}


