#include "pod.h"

#if APR_HAVE_UNISTD_H
#include <unistd.h>
#endif

AP_DECLARE(apr_status_t) ap_mpm_pod_open(apr_pool_t * p, ap_pod_t ** pod)
{
    apr_status_t rv;

    *pod = apr_palloc(p, sizeof(**pod));
    rv = apr_file_pipe_create(&((*pod)->pod_in), &((*pod)->pod_out), p);
    if (rv != APR_SUCCESS) {
        return rv;
    }
/*
    apr_file_pipe_timeout_set((*pod)->pod_in, 0);
*/
    (*pod)->p = p;

    /* close these before exec. */
    apr_file_inherit_unset((*pod)->pod_in);
    apr_file_inherit_unset((*pod)->pod_out);

    return APR_SUCCESS;
}

AP_DECLARE(int) ap_mpm_pod_check(ap_pod_t * pod)
{
    char c;
    apr_os_file_t fd;
    int rc;

    /* we need to surface EINTR so we'll have to grab the
     * native file descriptor and do the OS read() ourselves
     */
    apr_os_file_get(&fd, pod->pod_in);
    rc = read(fd, &c, 1);
    if (rc == 1) {
        switch (c) {
        case RESTART_CHAR:
            return AP_RESTART;
        case GRACEFUL_CHAR:
            return AP_GRACEFUL;
        }
    }
    return AP_NORESTART;
}

AP_DECLARE(apr_status_t) ap_mpm_pod_close(ap_pod_t * pod)
{
    apr_status_t rv;

    rv = apr_file_close(pod->pod_out);
    if (rv != APR_SUCCESS) {
        return rv;
    }

    rv = apr_file_close(pod->pod_in);
    if (rv != APR_SUCCESS) {
        return rv;
    }
    return rv;
}

static apr_status_t pod_signal_internal(ap_pod_t * pod, int graceful)
{
    apr_status_t rv;
    char char_of_death = graceful ? GRACEFUL_CHAR : RESTART_CHAR;
    apr_size_t one = 1;

    rv = apr_file_write(pod->pod_out, &char_of_death, &one);
    if (rv != APR_SUCCESS) {
	    ;
    }
    return rv;
}

AP_DECLARE(apr_status_t) ap_mpm_pod_signal(ap_pod_t * pod, int graceful)
{
    return pod_signal_internal(pod, graceful);
}

AP_DECLARE(void) ap_mpm_pod_killpg(ap_pod_t * pod, int num, int graceful)
{
    int i;
    apr_status_t rv = APR_SUCCESS;

    for (i = 0; i < num && rv == APR_SUCCESS; i++) {
        rv = pod_signal_internal(pod, graceful);
    }
}
