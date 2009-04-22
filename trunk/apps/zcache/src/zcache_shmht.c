/* Copyright 2001-2005 The Apache Software Foundation or its licensors, as
 * applicable.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdlib.h>
#include <time.h>
#include "zcache.h"

/*
 *  Wrapper functions for table library which resemble malloc(3) & Co 
 *  but use the variants from the MM shared memory library.
 */

static void *zcache_shmht_malloc(void *opt_param, size_t size)
{
    MCConfigRecord *mc = (MCConfigRecord *)opt_param;
    apr_rmm_off_t off = apr_rmm_calloc(mc->pStorageDataRMM, size);
    return apr_rmm_addr_get(mc->pStorageDataRMM, off);
}

static void *zcache_shmht_calloc(void *opt_param,
                                     size_t number, size_t size)
{
    MCConfigRecord *mc = (MCConfigRecord *)opt_param;
    apr_rmm_off_t off = apr_rmm_calloc(mc->pStorageDataRMM, (number*size));

    return apr_rmm_addr_get(mc->pStorageDataRMM, off);
}

static void *zcache_shmht_realloc(void *opt_param, void *ptr, size_t size)
{
    MCConfigRecord *mc = (MCConfigRecord *)opt_param;
    apr_rmm_off_t off = apr_rmm_realloc(mc->pStorageDataRMM, ptr, size);
    return apr_rmm_addr_get(mc->pStorageDataRMM, off);
}

static void zcache_shmht_free(void *opt_param, void *ptr)
{
    MCConfigRecord *mc = (MCConfigRecord *)opt_param;
    apr_rmm_off_t off = apr_rmm_offset_get(mc->pStorageDataRMM, ptr);
    apr_rmm_free(mc->pStorageDataRMM, off);
    return;
}

/*
 * Now the actual storage cache implementation
 * based on a hash table inside a shared memory segment.
 */

void zcache_shmht_init(MCConfigRecord *mc,apr_pool_t *p)
{
    table_t *ta;
    int ta_errno;
    apr_size_t avail;
    int n;
    apr_status_t rv;

    /*
     * Create shared memory segment
     */
    if (mc->szStorageDataFile == NULL) {
        /*ap_log_error(APLOG_MARK, APLOG_ERR, 0, s,
                     "LUASessionCache required");*/
        zcache_die();
    }
/*
    rv = apr_shm_create(&(mc->pStorageDataMM), 
                   mc->nStorageDataSize, 
                   NULL, mc->pPool);
*/
 //   if (APR_STATUS_IS_ENOTIMPL(rv)) {
        /* For a name-based segment, remove it first in case of a
         * previous unclean shutdown. */
        apr_shm_remove(mc->szStorageDataFile, mc->pPool);
        rv = apr_shm_create(&(mc->pStorageDataMM), 
                            mc->nStorageDataSize, 
                            mc->szStorageDataFile,
                            mc->pPool);
  //  }

    if (rv != APR_SUCCESS) {
        /*ap_log_error(APLOG_MARK, APLOG_ERR, rv, s,
                     "Cannot allocate shared memory");*/
        zcache_die();
    }

    if ((rv = apr_rmm_init(&(mc->pStorageDataRMM), NULL,
                   apr_shm_baseaddr_get(mc->pStorageDataMM),
                   mc->nStorageDataSize, mc->pPool)) != APR_SUCCESS) {
        /*ap_log_error(APLOG_MARK, APLOG_ERR, rv, s,
                     "Cannot initialize rmm");*/
        zcache_die();
    }
    /*ap_log_error(APLOG_MARK, APLOG_ERR, 0, s,
                 "initialize MM %pp RMM %pp",
                 mc->pStorageDataMM, mc->pStorageDataRMM);*/

    /*
     * Create hash table in shared memory segment
     */
    avail = mc->nStorageDataSize;
    n = (avail/2) / 1024;
    n = n < 10 ? 10 : n;

    /*
     * Passing server_rec as opt_param to table_alloc so that we can do
     * logging if required zcache_util_table. Otherwise, mc is sufficient.
     */ 
    if ((ta = table_alloc(n, &ta_errno, 
                          zcache_shmht_malloc,  
                          zcache_shmht_calloc, 
                          zcache_shmht_realloc, 
                          zcache_shmht_free, mc)) == NULL) {
        /*ap_log_error(APLOG_MARK, APLOG_ERR, 0, s,
                     "Cannot allocate hash table in shared memory: %s",
                     table_strerror(ta_errno));*/
        zcache_die();
    }

    table_attr(ta, TABLE_FLAG_AUTO_ADJUST|TABLE_FLAG_ADJUST_DOWN);
    table_set_data_alignment(ta, sizeof(char *));
    table_clear(ta);
    mc->tStorageDataTable = ta;

    /*
     * Log the done work
     */
    /*ap_log_error(APLOG_MARK, APLOG_INFO, 0, s, 
                 "Init: Created hash-table (%d buckets) "
                 "in shared memory (%" APR_SIZE_T_FMT 
                 " bytes) for STORAGE storage cache",
                 n, avail);*/
    return;
}

void zcache_shmht_kill(MCConfigRecord *mc)
{

    if (mc->pStorageDataRMM != NULL) {
        apr_rmm_destroy(mc->pStorageDataRMM);
        mc->pStorageDataRMM = NULL;
    }

    if (mc->pStorageDataMM != NULL) {
        apr_shm_destroy(mc->pStorageDataMM);
        mc->pStorageDataMM = NULL;
    }
    return;
}

BOOL zcache_shmht_store(MCConfigRecord *mc,UCHAR *id, int idlen, time_t expiry, void *pdata, int ndata)
{
    void *vp;

    zcache_mutex_on(mc);
    if (table_insert_kd(mc->tStorageDataTable, 
                        id, idlen, NULL, sizeof(time_t)+ndata,
                        NULL, &vp, 1) != TABLE_ERROR_NONE) {
        zcache_mutex_off(mc);
        return FALSE;
    }
    memcpy(vp, &expiry, sizeof(time_t));
    memcpy((char *)vp+sizeof(time_t), pdata, ndata);
    zcache_mutex_off(mc);

    /* allow the regular expiring to occur */
    zcache_shmht_expire(mc);

    return TRUE;
}

void *zcache_shmht_retrieve(MCConfigRecord *mc,UCHAR *id, int idlen, int* ndata)
{
	void *vp;
	void *pdata;

	time_t expiry;
	time_t now;
	int n;

	/* allow the regular expiring to occur */
	zcache_shmht_expire(mc);

	/* lookup key in table */
	zcache_mutex_on(mc);
	if (table_retrieve(mc->tStorageDataTable,
				id, idlen, &vp, &n) != TABLE_ERROR_NONE) {
		zcache_mutex_off(mc);
		return NULL;
	}

	/* copy over the information to the SCI */
	*ndata = n-sizeof(time_t);
	pdata = (UCHAR *)malloc(*ndata);
	if (pdata == NULL) {
		zcache_mutex_off(mc);
		return NULL;
	}
	memcpy(&expiry, vp, sizeof(time_t));
	memcpy(pdata, (char *)vp+sizeof(time_t), *ndata);
	zcache_mutex_off(mc);

	/* make sure the stuff is still not expired */
	now = time(NULL);
	if (expiry <= now) {
		zcache_shmht_remove(mc, id, idlen);
		return NULL;
	}

	return pdata;
}

void zcache_shmht_remove(MCConfigRecord *mc,UCHAR *id, int idlen)
{
    /* remove value under key in table */
    zcache_mutex_on(mc);
    table_delete(mc->tStorageDataTable, id, idlen, NULL, NULL);
    zcache_mutex_off(mc);
    return;
}

void zcache_shmht_expire(MCConfigRecord *mc)
{
    static time_t tLast = 0;
    table_linear_t iterator;
    time_t tExpiresAt;
    void *vpKey;
    void *vpKeyThis;
    void *vpData;
    int nKey;
    int nKeyThis;
    int nData;
    int nElements = 0;
    int nDeleted = 0;
    int bDelete;
    int rc;
    time_t tNow;

    /*
     * make sure the expiration for still not-accessed storage
     * cache entries is done only from time to time
     */
    tNow = time(NULL);
    tLast = tNow;

    zcache_mutex_on(mc);
    if (table_first_r(mc->tStorageDataTable, &iterator,
                      &vpKey, &nKey, &vpData, &nData) == TABLE_ERROR_NONE) {
        do {
            bDelete = FALSE;
            nElements++;
            if (nData < sizeof(time_t) || vpData == NULL)
                bDelete = TRUE;
            else {
                memcpy(&tExpiresAt, vpData, sizeof(time_t));
                /* 
                 * XXX : Force the record to be cleaned up. TBD (Madhu)
                 * tExpiresAt = tNow;
                 */
                if (tExpiresAt <= tNow)
                   bDelete = TRUE;
            }
            vpKeyThis = vpKey;
            nKeyThis  = nKey;
            rc = table_next_r(mc->tStorageDataTable, &iterator,
                              &vpKey, &nKey, &vpData, &nData);
            if (bDelete) {
                table_delete(mc->tStorageDataTable,
                             vpKeyThis, nKeyThis, NULL, NULL);
                nDeleted++;
            }
        } while (rc == TABLE_ERROR_NONE); 
        /* (vpKeyThis != vpKey) && (nKeyThis != nKey) */
    }
    zcache_mutex_off(mc);
    /*ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, s,
                 "Inter-Process Session Cache (SHMHT) Expiry: "
                 "old: %d, new: %d, removed: %d",
                 nElements, nElements-nDeleted, nDeleted);*/
    return;
}

void zcache_shmht_status(MCConfigRecord *mc,apr_pool_t *p, void (*func)(char *, void *), void *arg)
{
    void *vpKey;
    void *vpData;
    int nKey;
    int nData;
    int nElem;
    int nSize;
    int nAverage;

    nElem = 0;
    nSize = 0;
    zcache_mutex_on(mc);
    if (table_first(mc->tStorageDataTable,
                    &vpKey, &nKey, &vpData, &nData) == TABLE_ERROR_NONE) {
        do {
            if (vpKey == NULL || vpData == NULL)
                continue;
            nElem += 1;
            nSize += nData;
        } while (table_next(mc->tStorageDataTable,
                        &vpKey, &nKey, &vpData, &nData) == TABLE_ERROR_NONE);
    }
    zcache_mutex_off(mc);
    if (nSize > 0 && nElem > 0)
        nAverage = nSize / nElem;
    else
        nAverage = 0;
    func(apr_psprintf(p, "cache type: <b>SHMHT</b>, maximum size: <b>%d</b> bytes<br>", mc->nStorageDataSize), arg);
    func(apr_psprintf(p, "current storages: <b>%d</b>, current size: <b>%d</b> bytes<br>", nElem, nSize), arg);
    func(apr_psprintf(p, "average storage size: <b>%d</b> bytes<br>", nAverage), arg);
    return;
}
