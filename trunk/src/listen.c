#include "apr_network_io.h"
#include "apr_strings.h"
#include "apr_lib.h"
#include "stdlib.h"

#define APR_WANT_STRFUNC
#include "apr_want.h"

#include "zevent_config.h"
#include "server.h"
#include "ap_listen.h"
#include "mpm_common.h"

AP_DECLARE_DATA ap_listen_rec *ap_listeners = NULL;

static ap_listen_rec *old_listeners;
static int ap_listenbacklog;
static int send_buffer_size;
static int receive_buffer_size;

/* TODO: make_sock is just begging and screaming for APR abstraction */
static apr_status_t make_sock(apr_pool_t *p, ap_listen_rec *server)
{
    apr_socket_t *s = server->sd;
    int one = 1;
#if APR_HAVE_IPV6
#ifdef AP_ENABLE_V4_MAPPED
    int v6only_setting = 0;
#else
    int v6only_setting = 1;
#endif
#endif
    apr_status_t stat;

#ifndef WIN32
    stat = apr_socket_opt_set(s, APR_SO_REUSEADDR, one);
    if (stat != APR_SUCCESS && stat != APR_ENOTIMPL) {
        apr_socket_close(s);
        return stat;
    }
#endif

    stat = apr_socket_opt_set(s, APR_SO_KEEPALIVE, one);
    if (stat != APR_SUCCESS && stat != APR_ENOTIMPL) {
        apr_socket_close(s);
        return stat;
    }

#if APR_HAVE_IPV6
    if (server->bind_addr->family == APR_INET6) {
        stat = apr_socket_opt_set(s, APR_IPV6_V6ONLY, v6only_setting);
        if (stat != APR_SUCCESS && stat != APR_ENOTIMPL) {
            apr_socket_close(s);
            return stat;
        }
    }
#endif

    /*
     * To send data over high bandwidth-delay connections at full
     * speed we must force the TCP window to open wide enough to keep the
     * pipe full.  The default window size on many systems
     * is only 4kB.  Cross-country WAN connections of 100ms
     * at 1Mb/s are not impossible for well connected sites.
     * If we assume 100ms cross-country latency,
     * a 4kB buffer limits throughput to 40kB/s.
     *
     * To avoid this problem I've added the SendBufferSize directive
     * to allow the web master to configure send buffer size.
     *
     * The trade-off of larger buffers is that more kernel memory
     * is consumed.  YMMV, know your customers and your network!
     *
     * -John Heidemann <johnh@isi.edu> 25-Oct-96
     *
     * If no size is specified, use the kernel default.
     */
    if (send_buffer_size) {
        stat = apr_socket_opt_set(s, APR_SO_SNDBUF,  send_buffer_size);
        if (stat != APR_SUCCESS && stat != APR_ENOTIMPL) {
		;
            /* not a fatal error */
        }
    }
    if (receive_buffer_size) {
        stat = apr_socket_opt_set(s, APR_SO_RCVBUF, receive_buffer_size);
        if (stat != APR_SUCCESS && stat != APR_ENOTIMPL) {
		;
            /* not a fatal error */
        }
    }

#if APR_TCP_NODELAY_INHERITED
    ap_sock_disable_nagle(s);
#endif

    if ((stat = apr_socket_bind(s, server->bind_addr)) != APR_SUCCESS) {
        apr_socket_close(s);
        return stat;
    }

    if ((stat = apr_socket_listen(s, ap_listenbacklog)) != APR_SUCCESS) {
        apr_socket_close(s);
        return stat;
    }

#ifdef WIN32
    /* I seriously doubt that this would work on Unix; I have doubts that
     * it entirely solves the problem on Win32.  However, since setting
     * reuseaddr on the listener -prior- to binding the socket has allowed
     * us to attach to the same port as an already running instance of
     * Apache, or even another web server, we cannot identify that this
     * port was exclusively granted to this instance of Apache.
     *
     * So set reuseaddr, but do not attempt to do so until we have the
     * parent listeners successfully bound.
     */
    stat = apr_socket_opt_set(s, APR_SO_REUSEADDR, one);
    if (stat != APR_SUCCESS && stat != APR_ENOTIMPL) {
        ap_log_perror(APLOG_MARK, APLOG_CRIT, stat, p,
                    "make_sock: for address %pI, apr_socket_opt_set: (SO_REUSEADDR)",
                     server->bind_addr);
        apr_socket_close(s);
        return stat;
    }
#endif

    server->sd = s;
    server->active = 1;

#ifdef MPM_ACCEPT_FUNC
     server->accept_func = MPM_ACCEPT_FUNC;
#else
    server->accept_func = NULL;
#endif

    return APR_SUCCESS;
}


static const char *alloc_listener(apr_pool_t *p, char *addr,
                                  apr_port_t port, const char* proto)
{
    ap_listen_rec **walk, *last;
    apr_status_t status;
    apr_sockaddr_t *sa;
    int found_listener = 0;

    /* see if we've got an old listener for this address:port */
    for (walk = &old_listeners; *walk;) {
        sa = (*walk)->bind_addr;
        /* Some listeners are not real so they will not have a bind_addr. */
        if (sa) {
            ap_listen_rec *new;
            apr_port_t oldport;

            oldport = sa->port;
            /* If both ports are equivalent, then if their names are equivalent,
             * then we will re-use the existing record.
             */
            if (port == oldport &&
                ((!addr && !sa->hostname) ||
                 ((addr && sa->hostname) && !strcmp(sa->hostname, addr)))) {
                new = *walk;
                *walk = new->next;
                new->next = ap_listeners;
                ap_listeners = new;
                found_listener = 1;
                continue;
            }
        }

        walk = &(*walk)->next;
    }

    if (found_listener) {
        return NULL;
    }

    if ((status = apr_sockaddr_info_get(&sa, addr, APR_UNSPEC, port, 0,
                                        p))
        != APR_SUCCESS) {
	    ;
    }

    /* Initialize to our last configured ap_listener. */
    last = ap_listeners;
    while (last && last->next) {
        last = last->next;
    }

    while (sa) {

        ap_listen_rec *new;

        /* this has to survive restarts */
        new = apr_palloc(p, sizeof(ap_listen_rec));
        new->active = 0;
        new->next = 0;
        new->bind_addr = sa;
        new->protocol = apr_pstrdup(p, proto);

        /* Go to the next sockaddr. */
        sa = sa->next;

        status = apr_socket_create(&new->sd, new->bind_addr->family,
                                    SOCK_STREAM, 0, p);

#if APR_HAVE_IPV6
        /* What could happen is that we got an IPv6 address, but this system
         * doesn't actually support IPv6.  Try the next address.
         */
        if (status != APR_SUCCESS && !addr &&
            new->bind_addr->family == APR_INET6) {
            continue;
        }
#endif
        if (status != APR_SUCCESS) {
		;
        }

        /* We need to preserve the order returned by getaddrinfo() */
        if (last == NULL) {
            ap_listeners = last = new;
        } else {
            last->next = new;
            last = new;
        }
    }

    return NULL;
}

AP_DECLARE_NONSTD(void) ap_close_listeners(void)
{
    ap_listen_rec *lr;

    for (lr = ap_listeners; lr; lr = lr->next) {
        apr_socket_close(lr->sd);
        lr->active = 0;
    }
}


static apr_status_t close_listeners_on_exec(void *v)
{
    ap_close_listeners();
    return APR_SUCCESS;
}

/* Evaluates to true if the (apr_sockaddr_t *) addr argument is the
 * IPv4 match-any-address, 0.0.0.0. */
#define IS_INADDR_ANY(addr) ((addr)->family == APR_INET \
                             && (addr)->sa.sin.sin_addr.s_addr == INADDR_ANY)

/* Evaluates to true if the (apr_sockaddr_t *) addr argument is the
 * IPv6 match-any-address, [::]. */
#define IS_IN6ADDR_ANY(addr) ((addr)->family == APR_INET6 \
                              && IN6_IS_ADDR_UNSPECIFIED(&(addr)->sa.sin6.sin6_addr))

/**
 * Create, open, listen, and bind all sockets.
 * @param process The process record for the currently running server
 * @return The number of open sockets
 */
AP_DECLARE(int) ap_open_listeners(apr_pool_t *pool)
{
    ap_listen_rec *lr;
    ap_listen_rec *next;
    ap_listen_rec *previous;
    int num_open;
    const char *userdata_key = "ap_open_listeners";
    void *data;
#if AP_NONBLOCK_WHEN_MULTI_LISTEN
    int use_nonblock;
#endif

    /* Don't allocate a default listener.  If we need to listen to a
     * port, then the user needs to have a Listen directive in their
     * config file.
     */
    num_open = 0;
    previous = NULL;
    for (lr = ap_listeners; lr; previous = lr, lr = lr->next) {
        if (lr->active) {
            ++num_open;
        }
        else {
#if APR_HAVE_IPV6
            ap_listen_rec *cur;
            int v6only_setting;
            int skip = 0;

            /* If we have the unspecified IPv4 address (0.0.0.0) and
             * the unspecified IPv6 address (::) is next, we need to
             * swap the order of these in the list. We always try to
             * bind to IPv6 first, then IPv4, since an IPv6 socket
             * might be able to receive IPv4 packets if V6ONLY is not
             * enabled, but never the other way around.
             * Note: In some configurations, the unspecified IPv6 address
             * could be even later in the list.  This logic only corrects
             * the situation where it is next in the list, such as when
             * apr_sockaddr_info_get() returns an IPv4 and an IPv6 address,
             * in that order.
             */
            if (lr->next != NULL
                && IS_INADDR_ANY(lr->bind_addr)
                && lr->bind_addr->port == lr->next->bind_addr->port
                && IS_IN6ADDR_ANY(lr->next->bind_addr)) {
                /* Exchange lr and lr->next */
                next = lr->next;
                lr->next = next->next;
                next->next = lr;
                if (previous) {
                    previous->next = next;
                }
                else {
                    ap_listeners = next;
                }
                lr = next;
            }

            /* If we are trying to bind to 0.0.0.0 and a previous listener
             * was :: on the same port and in turn that socket does not have
             * the IPV6_V6ONLY flag set; we must skip the current attempt to
             * listen (which would generate an error). IPv4 will be handled
             * on the established IPv6 socket.
             */
            if (IS_INADDR_ANY(lr->bind_addr)) {
                for (cur = ap_listeners; cur != lr; cur = cur->next) {
                    if (lr->bind_addr->port == cur->bind_addr->port
                        && IS_IN6ADDR_ANY(cur->bind_addr)
                        && apr_socket_opt_get(cur->sd, APR_IPV6_V6ONLY,
                                              &v6only_setting) == APR_SUCCESS
                        && v6only_setting == 0) {

                        /* Remove the current listener from the list */
                        previous->next = lr->next;
                        lr = previous; /* maintain current value of previous after
                                        * post-loop expression is evaluated
                                        */
                        skip = 1;
                        break;
                    }
                }
                if (skip) {
                    continue;
                }
            }
#endif
            if (make_sock(pool, lr) == APR_SUCCESS) {
                ++num_open;
                lr->active = 1;
            }
            else {
#if APR_HAVE_IPV6
                /* If we tried to bind to ::, and the next listener is
                 * on 0.0.0.0 with the same port, don't give a fatal
                 * error. The user will still get a warning from make_sock
                 * though.
                 */
                if (lr->next != NULL
                    && IS_IN6ADDR_ANY(lr->bind_addr)
                    && lr->bind_addr->port == lr->next->bind_addr->port
                    && IS_INADDR_ANY(lr->next->bind_addr)) {

                    /* Remove the current listener from the list */
                    if (previous) {
                        previous->next = lr->next;
                    }
                    else {
                        ap_listeners = lr->next;
                    }

                    /* Although we've removed ourselves from the list,
                     * we need to make sure that the next iteration won't
                     * consider "previous" a working IPv6 '::' socket.
                     * Changing the family is enough to make sure the
                     * conditions before make_sock() fail.
                     */
                    lr->bind_addr->family = AF_INET;

                    continue;
                }
#endif
                /* fatal error */
                return -1;
            }
        }
    }

    /* close the old listeners */
    for (lr = old_listeners; lr; lr = next) {
        apr_socket_close(lr->sd);
        lr->active = 0;
        next = lr->next;
    }
    old_listeners = NULL;

#if AP_NONBLOCK_WHEN_MULTI_LISTEN
    /* if multiple listening sockets, make them non-blocking so that
     * if select()/poll() reports readability for a reset connection that
     * is already forgotten about by the time we call accept, we won't
     * be hung until another connection arrives on that port
     */
    use_nonblock = (ap_listeners && ap_listeners->next);
    for (lr = ap_listeners; lr; lr = lr->next) {
        apr_status_t status;

        status = apr_socket_opt_set(lr->sd, APR_SO_NONBLOCK, use_nonblock);
        if (status != APR_SUCCESS) {
            return -1;
        }
    }
#endif /* AP_NONBLOCK_WHEN_MULTI_LISTEN */

    /* we come through here on both passes of the open logs phase
     * only register the cleanup once... otherwise we try to close
     * listening sockets twice when cleaning up prior to exec
     */
    apr_pool_userdata_get(&data, userdata_key, pool);
    if (!data) {
        apr_pool_userdata_set((const void *)1, userdata_key,
                              apr_pool_cleanup_null, pool);
        apr_pool_cleanup_register(pool, NULL, apr_pool_cleanup_null,
                                  close_listeners_on_exec);
    }

    return num_open ? 0 : -1;
}



AP_DECLARE(void) ap_listen_pre_config(void)
{
    old_listeners = ap_listeners;
    ap_listeners = NULL;
    ap_listenbacklog = DEFAULT_LISTENBACKLOG;
}

AP_DECLARE(void) ap_str_tolower(char *str)
{
    while (*str) {
        *str = apr_tolower(*str);
        ++str;
    }
}

AP_DECLARE_NONSTD(const char *) ap_set_listener(apr_pool_t *p,int argc,const char *argv[])
{
    char *host, *scope_id, *proto;
    apr_port_t port;
    apr_status_t rv;

    rv = apr_parse_addr_port(&host, &scope_id, &port, argv[0], p);
    if (rv != APR_SUCCESS) {
        return "Invalid address or port";
    }

    if (host && !strcmp(host, "*")) {
        host = NULL;
    }

    if (scope_id) {
        /* XXX scope id support is useful with link-local IPv6 addresses */
        return "Scope id is not supported";
    }

    if (!port) {
        return "Port must be specified";
    }

    if (argc != 2) {
        proto = "http";
    }
    else {
        proto = apr_pstrdup(p, argv[1]);
        ap_str_tolower(proto);
    }

    return alloc_listener(p, host, port, proto);
}

AP_DECLARE_NONSTD(const char *) ap_set_listenbacklog(const char *arg)
{
    int b;

    b = atoi(arg);
    if (b < 1) {
        return "ListenBacklog must be > 0";
    }

    ap_listenbacklog = b;
    return NULL;
}

AP_DECLARE_NONSTD(const char *) ap_set_send_buffer_size(const char *arg)
{
    int s = atoi(arg);


    if (s < 512 && s != 0) {
        return "SendBufferSize must be >= 512 bytes, or 0 for system default.";
    }

    send_buffer_size = s;
    return NULL;
}

AP_DECLARE_NONSTD(const char *) ap_set_receive_buffer_size(const char *arg)
{
    int s = atoi(arg);

    if (s < 512 && s != 0) {
        return "ReceiveBufferSize must be >= 512 bytes, or 0 for system default.";
    }

    receive_buffer_size = s;
    return NULL;
}
