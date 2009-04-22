#include "stdio.h"
#include "storage.h"

#define SHARED_SIZE (16*1024*1024)
#define SHARED_FILENAME "testshm.shm"
#define MUTEX_FILENAME "testshm.mutex"
#define TEST_NUM (100000)

apr_pool_t *p;
int main(int argc,const char *argv[])
{
	apr_status_t rv;
	apr_initialize();
	atexit(apr_terminate);
	rv = apr_pool_create(&p, NULL);
	if(rv != APR_SUCCESS)
		printf("error\n");


	MCConfigRecord mc;
	mc.pStorageDataMM = NULL;
	mc.pStorageDataRMM = NULL;
	mc.pPool = p;
	mc.nMutexMode = STORAGE_MUTEXMODE_USED;
	mc.szStorageDataFile = SHARED_FILENAME;
	mc.nStorageDataSize = SHARED_SIZE;
        mc.nStorageMode = STORAGE_SCMODE_SHMCB;
	mc.nMutexMech = APR_LOCK_DEFAULT;
	mc.szMutexFile = MUTEX_FILENAME;

  //    apr_shm_t *shm = NULL; 
  //	apr_shm_remove(SHARED_FILENAME, p);
        
	storage_init(&mc,p);
	if(mc.nMutexMode == STORAGE_MUTEXMODE_USED)
		storage_mutex_init(&mc,p);

	char key[64];
	int klen ;

	char data[256];
	int i,len;
	for(i=0;i<TEST_NUM;++i)
	{
		memset(key,0,sizeof(key));
		sprintf(key,"%d",i);
		klen = strlen(key);

	        memset(data,0,sizeof(data));
                sprintf(data,"%d-test data!",i);
		len = strlen(data)+1;

		time_t expiry = time(NULL);
		expiry += 1000;
		if(!storage_store(&mc,key,klen, expiry,(void *)data,len))
			printf("error store data key:%s\n",key);
	}
	printf("store data complete!\n");

	for(i=0;i<10; ++i)
	{
		sprintf(key,"%d",i);
		klen = strlen(key);

		void *pdata = storage_retrieve(&mc,key,klen,&len);
		printf("key:%s,data:%s\n",key,(const char *)pdata);
		storage_remove(&mc,key,klen);
	}
	///////////update///////////////
	memset(key,0,sizeof(key));
	sprintf(key,"%d",0);
	klen = strlen(key);

	memset(data,0,sizeof(data));
	sprintf(data,"0-newtest data!");
	len = strlen(data)+1;

	time_t expiry = time(NULL);
	expiry += 1000;
	if(!storage_store(&mc,key,klen, expiry,(void *)data,len))
		printf("error store data key:%s\n",key);

	sprintf(key,"%d",0);
	klen = strlen(key);

	void *pdata = storage_retrieve(&mc,key,klen,&len);
	printf("key:%s,newdata:%s\n",key,(const char *)pdata);
	////////////////////////////////////////////

	storage_kill(&mc);
	return 0;
}
