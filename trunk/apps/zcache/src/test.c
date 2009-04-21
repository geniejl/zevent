#include "apr_errno.h"
#include "apr_general.h"
#include "apr_user.h"
#include "apr_file_io.h"
#include "apr_thread_proc.h"
#include "apr_tables.h"
#include "apr_strings.h"
#include "apr_network_io.h"
#include "apr_shm.h"
#include "apr_rmm.h"
#include <stdio.h>
#include <assert.h>

static apr_array_header_t *a1 = NULL;
static apr_table_t *table1 = NULL;

static apr_file_t *readp = NULL;
static apr_file_t *writep = NULL;

static apr_thread_once_t *control = NULL;
static apr_thread_mutex_t *thread_lock;
static int x = 0;
static int value = 0;

static apr_thread_t *t1;
static apr_thread_t *t2;
static apr_thread_t *t3;
static apr_thread_t *t4;

static apr_status_t exit_ret_val = 123;
apr_pool_t *p;

static void init_func(void)
{
	value++;
}

static void * APR_THREAD_FUNC thread_func1(apr_thread_t *thd,void *data)
{
	int i;
	apr_thread_once(control,init_func);
	for (i = 0; i < 10000; i++){
		apr_thread_mutex_lock(thread_lock);
		x++;
		apr_thread_mutex_unlock(thread_lock);
	}
	apr_thread_exit(thd,exit_ret_val);
	return NULL;
}

static void thread_init(void *data)
{
	apr_status_t rv;
	rv = apr_thread_once_init(&control,p);
	rv = apr_thread_mutex_create(&thread_lock,APR_THREAD_MUTEX_DEFAULT,p);
}

static void create_threads(void *data)
{
	apr_status_t rv;

	rv = apr_thread_create(&t1,NULL,thread_func1,NULL,p);
	rv = apr_thread_create(&t2,NULL,thread_func1,NULL,p);
	rv = apr_thread_create(&t3,NULL,thread_func1,NULL,p);
	rv = apr_thread_create(&t4,NULL,thread_func1,NULL,p);
}

static void join_threads(void *data)
{
	apr_status_t s;
	apr_thread_join(&s,t1);
	apr_thread_join(&s,t2);
	apr_thread_join(&s,t3);
	apr_thread_join(&s,t4);
	printf("s:%d\n",s);
}

static void check_locks(void *data)
{

	printf("x:%d\n",x);
	assert(40000 == x);
}

static void check_thread_once(void *data)
{
	printf("val:%d\n",value);
	assert(1 == value);
}

#define FRAG_SIZE 80
#define FRAG_COUNT 10
#define SHARED_SIZE (apr_size_t)(FRAG_SIZE * FRAG_COUNT * sizeof(char*))

int main(int argc,char *argv[])
{
	apr_status_t rv;
	char *uname = NULL,*gname = NULL;

	apr_initialize();
	atexit(apr_terminate);
	apr_pool_create(&p, NULL);
	//test user
	/*
	   apr_uid_t uid;
	   apr_gid_t gid;
	   apr_uid_current(&uid, &gid, p);
	   printf("uid:%d,gid:%d\n",uid,gid);

	   apr_uid_name_get(&uname,uid,p);
	   printf("uname:%s\n",uname);

	   apr_gid_name_get(&gname,gid,p);
	   printf("gname:%s\n",gname);

	   apr_gid_get(&gid,gname,p);
	   printf("gid:%d\n",gid);*/
	//test pipe       
	/*	rv = apr_file_pipe_create(&readp,&writep,p);
		apr_file_close(readp);
		apr_file_close(writep);

		char buf[256];
		apr_size_t nbytes = 256;
		rv = apr_file_read(readp,buf,&nbytes);
		printf("%d\n",APR_STATUS_IS_EBADF(rv));

		rv = apr_file_pipe_create_ex(&readp,&writep,APR_WRITE_BLOCK,p);

		apr_interval_time_t timeout;
		rv = apr_file_pipe_timeout_get(writep,&timeout);

		rv = apr_file_pipe_timeout_set(readp,apr_time_from_sec(1));

		rv = apr_file_pipe_timeout_get(readp,&timeout);

		printf("timeout:%d\n",timeout);

		if(!rv){
		const char *temp = "test pipe!";
		nbytes = strlen(temp);
		apr_file_write(writep,temp,&nbytes);
		rv = apr_file_read(readp,buf,&nbytes);
		printf("rv:%d,timeout:%d,nbytes:%d\n",rv,
		APR_STATUS_IS_TIMEUP(rv),
		nbytes);
		}
		*/
	//test table
	/*       a1 = apr_array_make(p,2,sizeof(const char *));
		 APR_ARRAY_PUSH(a1,const char *) = "foo";
		 APR_ARRAY_PUSH(a1,const char *) = "bar";
		 printf("nelts:%d\n",a1->nelts);
		 apr_array_clear(a1);
		 printf("nelts:%d\n",a1->nelts);

		 table1 = apr_table_make(p,5);
		 const char *val;
		 apr_table_set(table1,"foo","bar");
		 val = apr_table_get(table1,"foo");
		 printf("val:%s\n",val);

		 apr_table_add(table1,"addkey","bar");
		 apr_table_add(table1,"addkey","foo");
		 val = apr_table_get(table1,"addkey");
		 printf("val:%s\n",val);

		 val = apr_table_get(table1,"addkey");
		 printf("val:%s\n",val);
		 */
	//test thread
	/*        
		  thread_init(NULL);
		  create_threads(NULL);
		  join_threads(NULL);
		  check_locks(NULL);
		  check_thread_once(NULL);*/

	//test temp
	/* const char *tempdir = NULL;
	   rv = apr_temp_dir_get(&tempdir,p);
	   printf("tempdir:%s\n",tempdir);
	   apr_file_t *f = NULL;

	   char * filetemplate;
	   filetemplate = apr_pstrcat(p,tempdir,"/tempfile.XXXXXX",NULL);

	   rv = apr_file_mktemp(&f,filetemplate,0,p);
	   printf("rv:%d\n",rv);*/

	//test sock
	/*        apr_sockaddr_t *sa;
		  apr_socket_t *sock;
		  rv = apr_sockaddr_info_get(&sa,"127.0.0.1",APR_INET,8000,0,p);
		  printf("rv:%d,hostname:%s\n",rv,sa->hostname);
		  rv = apr_socket_create(&sock,sa->family,SOCK_STREAM,APR_PROTO_TCP,p);
		  rv = apr_socket_opt_set(sock,APR_SO_REUSEADDR,1);
		  rv = apr_socket_bind(sock,sa);
		  rv = apr_socket_listen(sock,5);
		  rv = apr_socket_close(sock);*/

	/*	apr_allocator_t *allocator;
		rv = apr_allocator_create(&allocator);
		if(rv != APR_SUCCESS)
		printf("rv:%d\n",rv);
		apr_memnode_t *node = apr_allocator_alloc(allocator,1024);
		memcpy(node->first_avail,"aaaa",3);
		printf("node:%p\n",node->first_avail);
		apr_allocator_free(allocator,node);
		node = apr_allocator_alloc(allocator,1024);
		memcpy(node->first_avail,"aaaa",3);
		printf("node:%p\n",node->first_avail);*/

	// test apr_rmm
	apr_pool_t *pool;
	apr_shm_t *shm;
	apr_rmm_t *rmm;
	apr_size_t size, fragsize;
	apr_rmm_off_t *off, off2;
	int i;
	void *entity;

	rv = apr_pool_create(&pool, p); 

	/* We're going to want 10 blocks of data from our target rmm. */
	size = SHARED_SIZE + apr_rmm_overhead_get(FRAG_COUNT + 1);
	rv = apr_shm_create(&shm, size, "data", pool);

	if (rv != APR_SUCCESS)
		return -1;

	rv = apr_rmm_init(&rmm, NULL, apr_shm_baseaddr_get(shm), size, pool);

	if (rv != APR_SUCCESS)
		return -1;

	/* Creating each fragment of size fragsize */
	fragsize = SHARED_SIZE / FRAG_COUNT;
	off = apr_palloc(pool, FRAG_COUNT * sizeof(apr_rmm_off_t));
	for (i = 0; i < FRAG_COUNT; i++) {
		off[i] = apr_rmm_malloc(rmm, fragsize);
	}

	/* Checking for out of memory allocation */
	off2 = apr_rmm_malloc(rmm, FRAG_SIZE * FRAG_COUNT);
	//ABTS_TRUE(tc, !off2);

	/* Checking each fragment for address alignment */
	for (i = 0; i < FRAG_COUNT; i++) {
		char *c = apr_rmm_addr_get(rmm, off[i]);
		apr_size_t sc = (apr_size_t)c;

		//	ABTS_TRUE(tc, !!off[i]);
		//	ABTS_TRUE(tc, !(sc & 7));
	}

	/* Setting each fragment to a unique value */
	for (i = 0; i < FRAG_COUNT; i++) {
		int j;
		char **c = apr_rmm_addr_get(rmm, off[i]);
		for (j = 0; j < FRAG_SIZE; j++, c++) {
			*c = apr_itoa(pool, i + j);
		}
	}

	/* Checking each fragment for its unique value */
	for (i = 0; i < FRAG_COUNT; i++) {
		int j;
		char **c = apr_rmm_addr_get(rmm, off[i]);
		for (j = 0; j < FRAG_SIZE; j++, c++) {
			char *d = apr_itoa(pool, i + j);
			//ABTS_STR_EQUAL(tc, d, *c);
		}
	}

	/* Freeing each fragment */
	for (i = 0; i < FRAG_COUNT; i++) {
		rv = apr_rmm_free(rmm, off[i]);
		//ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
	}

	return 0;
}
