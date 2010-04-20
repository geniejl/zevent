/* 
**  mod_test.c -- Apache sample test module
**  [Autogenerated via ``apxs -n test -g'']
**
**  To play with this sample module first compile it into a
**  DSO file and install it into Apache's modules directory 
**  by running:
**
**    $ apxs -c -i mod_rtinfo.c
** apxs -c -i mod_rtinfo.c RT_Svr.c Rio.c \

**  Then activate it in Apache's httpd.conf file for instance
**  for the URL /test in as follows:
**
**    #   httpd.conf
**    LoadModule rtinfo_module modules/mod_rtinfo.so
**    <Location /realtime>
**    SetHandler rtinfo
**    </Location>
**
**  Then after restarting Apache via
**
**    $ apachectl restart
**
**  you immediately can request the URL /test and watch for the
**  output of this module. This can be achieved for instance via:
**
**    $ lynx -mime_header http://localhost/count 
**
**  The output should be similar to the following one:
**
**    HTTP/1.1 200 OK
**    Date: Tue, 31 Mar 1998 14:42:22 GMT
**    Server: Apache/1.3.4 (Unix)
**    Connection: close
**    Content-Type: text/html
**  
**    The sample page from mod_count.c
*/ 

#include "httpd.h"
#include "http_config.h"
#include "http_protocol.h"
#include "http_log.h"
#include "http_core.h"
#include "ap_config.h"
#include "ap_compat.h"

#include "Protocol.h"
#include "RT_Svr.h"
#include "mod_rtinfo.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

module AP_MODULE_DECLARE_DATA rtinfo_module;

static int strisdigit(const char *str)
{
	while(*str)   {   
		if(!isdigit(*str++))   
			return   0;   
	}   
	return   1; 
}
static int strtolower(char *str)
{
	char *s=str;
	while(*s)
	{
		*s=tolower(*s);
		++s;
	}
	return 1;

}
static int rt_parse_cmdline(request_rec *r, const char * ps, anUser* pu)
{
	int ret, i;
	const char * qs = NULL;
	char *value, *param;
	char * pc;

	if(ps == NULL) return -1;
	qs = ps;
	/* take care of application/x-www-form-urlencoded pesky + situation. */
	ret=strlen(qs); pc = (char *) qs; i=0;
	while( (pc[i]!='\0') && (i<ret)) {
		if (pc[i]=='+') pc[i]=' ';
		++i;
	}
	i=0; pc=NULL;

	while ( qs && *qs && ((value=ap_getword(r->pool, &qs, '&'))!=NULL) ) {
		param=ap_getword(r->pool, (const char**)&value, '=');
		if (!param || (*param=='\0')) continue;

		ap_unescape_url((char*)value);

		if (strcasecmp(param,"b")==0){
			if(value != NULL && strlen(value)>0 && strlen(value) < KEY_LEN -2){
				strcpy(&pu->dbname[0],value);
			}else{
				pu->err = err_dbname;
			}
		}
		else if (strcasecmp(param, "k")==0) {
			if(value != NULL && strlen(value)>0 && strlen(value) < KEY_LEN - 2){
				strcpy(&pu->key[0], value);
			}else{
				pu->err = err_key;
			}
		} 
		else if (strcasecmp(param, "v")==0) {
			if(value != NULL && strlen(value)>0 && strlen(value) < VALUE_LEN - 2){
				strcpy(&pu->value[0], value);
			}else{
				pu->err = err_value;
			}
		} 
		else if (strcasecmp(param, "c")==0){
			if(value != NULL && strlen(value)>0 && strlen(value) < 2  && strisdigit(value)){
				pu->command = atoi(value);
			} else {
				if(pu->err != err_key)
					pu->err = err_command;
			}
		}     
		
	} 
	if(pu->err!=err_noerr)
		return -1;
	else
		return 0;

}

#if APR_HAS_THREADS
static apr_status_t LOCK(apr_thread_mutex_t *mutex)
{
	apr_status_t rv =APR_SUCCESS;
	if(!mutex)
	{
		return APR_EGENERAL;
	}
	rv = apr_thread_mutex_lock(mutex);
	return rv;
}
static apr_status_t UNLOCK(apr_thread_mutex_t *mutex)
{
	apr_status_t rv =APR_SUCCESS;
	if(!mutex)
	{
		return APR_EGENERAL;
	}
	rv = apr_thread_mutex_unlock(mutex);
	return rv;
}
#endif
static int process_request(STORE *store,int cmd)
{
	int ret = 0;
#if APR_HAS_THREADS
	LOCK(rtinfo_con.mutex);
#endif
	if(rtinfo_con.fd ==-1)
	{
		if((rtinfo_con.fd = connect_server(server_ip,server_port)) == -1) {

			ret = EXEC_CON_FAILED;
		}
	}
	if(rtinfo_con.fd != -1)
	{
		ret = exec_c(rtinfo_con.fd,store,cmd);	
		if(ret == -2)
		{
			connect_close(rtinfo_con.fd);
			rtinfo_con.fd = -1;
			if((rtinfo_con.fd = connect_server(server_ip,server_port)) == -1) {
				rtinfo_con.fd = -1;
				ret = EXEC_CON_FAILED;
			}
			else
			{
				ret = exec_c(rtinfo_con.fd,store,cmd);
			}
		}
	}

#if APR_HAS_THREADS
	UNLOCK(rtinfo_con.mutex);
#endif
	return ret;
}
static int verify_params(anUser *usr,int *cmd)
{
	int rv =0;
	if(usr->key[0]=='\0')
	{
		rv = -1;
	}
	else
	{
		switch (usr->command)
		{
			case set:
				if(usr->value[0]=='\0')
				{
					rv=-1;
				}
				*cmd = CMD_DATA_SET;
				break;
			case get:
				*cmd = CMD_DATA_GET;
				break;
			default:
				rv=-2;
		}
	}
	return rv;
}
static int process_post_data(request_rec *r,char *pPostData)
{
	apr_bucket_brigade *bb;
	apr_status_t rv;
	int seen_eos, child_stopped_reading;
	char* pRealContent;
	unsigned int nContentLength;
	unsigned int nCurPostion;
	apr_pool_t *p;

	if (strcmp(r->handler, "rtinfo"))
		return DECLINED;
	nCurPostion = 0;
	nContentLength = (unsigned int)apr_atoi64(apr_table_get(r->headers_in, "Content-Length"));
	if (0 == nContentLength || (MAX_VALUE_LEN-2) < nContentLength )
	{
		return HTTP_BAD_REQUEST;
	}
	apr_pool_create(&p, NULL);
	pRealContent = apr_palloc(p, nContentLength);
	bb = apr_brigade_create(r->pool, r->connection->bucket_alloc);
	seen_eos = 0;
	child_stopped_reading = 0;
	do
	{
		apr_bucket *bucket;

		rv = ap_get_brigade(r->input_filters, bb, AP_MODE_READBYTES,
				APR_BLOCK_READ, HUGE_STRING_LEN);
		if (rv != APR_SUCCESS)
		{
			apr_brigade_destroy(bb);
			apr_pool_destroy(p);
			return rv;
		}

		for (bucket = APR_BRIGADE_FIRST(bb);
				bucket != APR_BRIGADE_SENTINEL(bb);
				bucket = APR_BUCKET_NEXT(bucket))
		{
			const char *data;
			apr_size_t len;

			if (APR_BUCKET_IS_EOS(bucket)) {
				seen_eos = 1;
				break;
			}

			/* We can't do much with this. */
			if (APR_BUCKET_IS_FLUSH(bucket)) {
				continue;
			}

			/* read */
			apr_bucket_read(bucket, &data, &len, APR_BLOCK_READ);
			if (len + nCurPostion > nContentLength)
			{
				ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, r, "mem alloct error, no more memory!!nContentLength=%d,len=%d,nCurPostion=%d.",nContentLength,len,nCurPostion);
				apr_brigade_destroy(bb);
				apr_pool_destroy(p);
				return HTTP_INTERNAL_SERVER_ERROR;   
			}

			memcpy(pRealContent + nCurPostion, data, len);
			nCurPostion += len;

		}
		apr_brigade_cleanup(bb);
	}
	while (!seen_eos);

	apr_brigade_destroy(bb);
	apr_pool_destroy(p);

	if (pRealContent[nCurPostion] != '\0')
	{
		pRealContent[nCurPostion] = '\0';
	}
//	ap_rputs(pRealContent, r);
        strcpy(pPostData,pRealContent);
	ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_INFO,
						0, NULL,pRealContent);
	
	return OK;
}
/* The sample rt data handler */
static int rtinfo_handler(request_rec *r)
{
	int rv = 0;
	int failed=0;
	int cmd = -1;

	anUser usr;
	memset(usr.dbname,0,sizeof(usr.dbname));
	memset(usr.key,0,sizeof(usr.key));
	memset(usr.value,0,sizeof(usr.value));
	usr.command=-1;
	usr.err=err_noerr;

	int fd;
	STORE store;

	char str[TMP_BUF_LEN];
        bzero(str,TMP_BUF_LEN);
        char buf[TMP_BUF_LEN];
	bzero(buf,TMP_BUF_LEN);

	if (strcmp(r->handler, "rtinfo")) {
		return DECLINED;
	}
	
	apr_table_set(r->headers_out, "Pragma", "no-cache");
	apr_table_set(r->headers_out, "Cache-Control", "no-cache");

	if(!r->args)
		return OK;
	rv=rt_parse_cmdline(r, r->args, &usr);

	if(rv!=0)
	{
		sprintf(str,"%d",EXEC_PARAM_BAD);
		ap_rputs(str,r);
		ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_ERR,0, NULL,"Parse user parameter failed\n");
		return OK;
	}
	if (r->method_number == M_POST)
	{
		if(OK!=process_post_data(r,usr.value))
		{
			sprintf(str,"%d",EXEC_PARAM_BAD);
			ap_rputs(str,r);
			ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_ERR,0, NULL,"Process user POST data failed\n");
			return OK;
		}
		//strtolower(usr.value);

	}

	strcpy(store.data.value,usr.value);
	strcpy(store.dbname,usr.dbname);
	strcpy(store.key,usr.key);
	store.data.curTime = time(NULL);	
	if(verify_params(&usr,&cmd)!=0)
	{
		sprintf(str,"%02d\r\n",EXEC_PARAM_BAD);                       
		ap_rputs(str,r);                                             
		ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_ERR,              
				0, NULL,"Exec cmd failed.Invalid Parameter.\n"); 
		sprintf(buf,"User Params key:%s,c:%d,value:%s\n",usr.key,usr.command,usr.value);
		ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_ERR,0,NULL,buf);

		return OK;
	}
	else
	{
		int ret = process_request(&store,cmd);
		if(ret!= 0)
		{
			if(ret == EXEC_CON_FAILED)
			{
				sprintf(str,"%02d",EXEC_CON_FAILED);
				ap_rputs(str,r);
				ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_ERR,0, NULL,
						"Connect rtinfo server failed!\n");
			}
			else
			{
				sprintf(str,"%02d",EXEC_CMD_FAILED);
				ap_rputs(str,r);
				ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_ERR,
						0, NULL,"Exec exec_c failed.\n");

				sprintf(buf,"User Params key:%s,c:%d,value:%s\n",
						usr.key,usr.command,usr.value);
				ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_ERR,0,NULL,buf);
			}
		}
		else
		{
			sprintf(str,"%02d\r\n",EXEC_CMD_SUC);
			switch (cmd)
			{
				case CMD_DATA_GET:
					ap_rputs(store.data.value,r);
					break;
				case CMD_DATA_SET:
					sprintf(str,"%d",EXEC_CMD_SUC);
					ap_rputs(str,r);
					break;
			}

		}
	}

	return OK;
}
static const char *
get_server_ip(cmd_parms *cmd,void *dconfig,const char *value){
	if(value != NULL && value[0] != 0){
		if(server_ip != NULL)
			free(server_ip);
		server_ip = strdup(value);
	}
	return NULL;
}
static const char *
get_server_port(cmd_parms *cmd,void *dconfig,const char *value){
	long nPort = strtol(value,NULL,0);
	if(nPort < 0)
	{
		server_port = DEFAULT_SERVER_PORT;
	}
	else
	{
		server_port = nPort;
	}
	return NULL;
}

static apr_status_t child_init(apr_pool_t *pool,server_rec *s)
{
	apr_status_t rv = APR_SUCCESS;
	rtinfo_con.fd = -1;
#if APR_HAS_THREADS
	rv = apr_thread_mutex_create(&rtinfo_con.mutex,APR_THREAD_MUTEX_DEFAULT,pool);
	if(rv != APR_SUCCESS)
	{
		ap_log_error(APLOG_MARK,APLOG_CRIT,rv,s,"rtinfo:failed to create thread mutex");
		return rv;
	}
#endif 
	return rv;
}

static const command_rec access_cmds[] = 
{
	AP_INIT_TAKE1("RTSvc_Server_IP",get_server_ip,NULL,RSRC_CONF,
			"Set server ip"),
	AP_INIT_TAKE1("RTSvc_Server_Port",get_server_port,NULL,RSRC_CONF,
			"Set server port"),
	{NULL}
};

static void rtinfo_register_hooks(apr_pool_t *p)
{
	ap_hook_child_init((void *)child_init,NULL,NULL,APR_HOOK_MIDDLE);
	ap_hook_handler(rtinfo_handler, NULL, NULL, APR_HOOK_MIDDLE);
}

/* Dispatch list for API hooks */
module AP_MODULE_DECLARE_DATA rtinfo_module = {
    STANDARD20_MODULE_STUFF, 
    NULL,                  /* create per-dir    config structures */
    NULL,                  /* merge  per-dir    config structures */
    NULL,                  /* create per-server config structures */
    NULL,                  /* merge  per-server config structures */
    access_cmds,                  /* table of config file commands       */
    rtinfo_register_hooks  /* register hooks                      */
};