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

#include "apr.h"
#include "apr_strings.h"

#define CORE_PRIVATE
#include "ap_config.h"
#include "server.h"
#include "ap_mpm.h"
#include "mpm_default.h"
#include "scoreboard.h"

/*
 * More machine-dependent networking gooo... on some systems,
 * you've got to be *really* sure that all the packets are acknowledged
 * before closing the connection, since the client will not be able
 * to see the last response if their TCP buffer is flushed by a RST
 * packet from us, which is what the server's TCP stack will send
 * if it receives any request data after closing the connection.
 *
 * In an ideal world, this function would be accomplished by simply
 * setting the socket option SO_LINGER and handling it within the
 * server's TCP stack while the process continues on to the next request.
 * Unfortunately, it seems that most (if not all) operating systems
 * block the server process on close() when SO_LINGER is used.
 * For those that don't, see USE_SO_LINGER below.  For the rest,
 * we have created a home-brew lingering_close.
 *
 * Many operating systems tend to block, puke, or otherwise mishandle
 * calls to shutdown only half of the connection.  You should define
 * NO_LINGCLOSE in ap_config.h if such is the case for your system.
 */
#ifndef MAX_SECS_TO_LINGER
#define MAX_SECS_TO_LINGER 30
#endif

/* we now proceed to read from the client until we get EOF, or until
 * MAX_SECS_TO_LINGER has passed.  the reasons for doing this are
 * documented in a draft:
 *
 * http://www.ics.uci.edu/pub/ietf/http/draft-ietf-http-connection-00.txt
 *
 * in a nutshell -- if we don't make this effort we risk causing
 * TCP RST packets to be sent which can tear down a connection before
 * all the response data has been sent to the client.
 */
#define SECONDS_TO_LINGER  2
AP_DECLARE(void) ap_lingering_close(apr_socket_t *csd)
{
    char dummybuf[512];
    apr_size_t nbytes;
    apr_time_t timeup = 0;

    if (!csd) {
        return;
    }

    /* Close the connection, being careful to send out whatever is still
     * in our buffers.  If possible, try to avoid a hard close until the
     * client has ACKed our FIN and/or has stopped sending us data.
     */

    /* Send any leftover data to the client, but never try to again */

    /* Shut down the socket for write, which will send a FIN
     * to the peer.
     */
    if (apr_socket_shutdown(csd, APR_SHUTDOWN_WRITE) != APR_SUCCESS
        ) {
        apr_socket_close(csd);
        return;
    }

    /* Read available data from the client whilst it continues sending
     * it, for a maximum time of MAX_SECS_TO_LINGER.  If the client
     * does not send any data within 2 seconds (a value pulled from
     * Apache 1.3 which seems to work well), give up.
     */
    apr_socket_timeout_set(csd, apr_time_from_sec(SECONDS_TO_LINGER));
    apr_socket_opt_set(csd, APR_INCOMPLETE_READ, 1);

    /* The common path here is that the initial apr_socket_recv() call
     * will return 0 bytes read; so that case must avoid the expensive
     * apr_time_now() call and time arithmetic. */

    do {
        nbytes = sizeof(dummybuf);
        if (apr_socket_recv(csd, dummybuf, &nbytes) || nbytes == 0)
            break;

        if (timeup == 0) {
            /* First time through; calculate now + 30 seconds. */
            timeup = apr_time_now() + apr_time_from_sec(MAX_SECS_TO_LINGER);
            continue;
        }
    } while (apr_time_now() < timeup);

    apr_socket_close(csd);
    return;
}



