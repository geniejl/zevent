#ifndef MOD_LUA_H
#define MOD_LUA_H

/* ARP headers */
#include "apr.h"
#include "apr_strings.h"
#define APR_WANT_STRFUNC
#include "apr_want.h"
#include "apr_tables.h"
#include "apr_lib.h"
#include "apr_fnmatch.h"
#include "apr_strings.h"
#include "apr_dbm.h"
#include "apr_rmm.h"
#include "apr_shm.h"
#include "apr_global_mutex.h"
#include "apr_optional.h"
#include "apr_queue.h"
#include "apr_strings.h"
#include "apr_env.h"
#include "apr_thread_mutex.h"
#include "apr_thread_cond.h"


/* The #ifdef macros are only defined AFTER including the above
* therefore we cannot include these system files at the top  :-(
*/
#if APR_HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if APR_HAVE_UNISTD_H
#include <unistd.h> /* needed for STDIN_FILENO et.al., at least on FreeBSD */
#endif

#include "storage_util_table.h"
/*
* Provide reasonable default for some defines
*/
#ifndef FALSE
#define FALSE (0)
#endif
#ifndef TRUE
#define TRUE (!FALSE)
#endif
#ifndef PFALSE
#define PFALSE ((void *)FALSE)
#endif
#ifndef PTRUE
#define PTRUE ((void *)TRUE)
#endif
#ifndef UNSET
#define UNSET (-1)
#endif
#ifndef NUL
#define NUL '\0'
#endif
#ifndef RAND_MAX
#include <limits.h>
#define RAND_MAX INT_MAX
#endif

#define APR_SHM_MAXSIZE (64 * 1024 * 1024)

typedef enum {
	STORAGE_SCMODE_UNSET = UNSET,
	STORAGE_SCMODE_NONE  = 0,
//	STORAGE_SCMODE_DBM   = 1,
	STORAGE_SCMODE_SHMHT = 2,
	STORAGE_SCMODE_SHMCB = 3,
} storage_scmode_t;

/*
* Define the STORAGE mutex modes
*/
typedef enum {
	STORAGE_MUTEXMODE_UNSET  = UNSET,
	STORAGE_MUTEXMODE_NONE   = 0,
	STORAGE_MUTEXMODE_USED   = 1
} storage_mutexmode_t;

typedef struct {
	pid_t           pid;
	apr_pool_t     *pPool;
	int            bFixed;
	int             nStorageMode;
	char           *szStorageDataFile;
	int             nStorageDataSize;
	apr_shm_t      *pStorageDataMM;
	apr_rmm_t      *pStorageDataRMM;
	apr_table_t    *tStorageDataTable;
	storage_mutexmode_t  nMutexMode;
	apr_lockmech_e  nMutexMech;
	const char     *szMutexFile;
	apr_global_mutex_t   *pMutex;
}MCConfigRecord;


#ifndef BOOL
#define BOOL unsigned int
#endif
#ifndef UCHAR
#define UCHAR unsigned char
#endif

#endif
