#ifndef BT_IF_H
#define BT_IF_H

#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>

#if !defined(WIN32)
#define BT_DECLARE(type)	type
#define BT_DECLARE_NONSTD(type)	type
#define BT_DECLARE_DATA
#else
#define BT_DECLARE(type)	__declspec(dllexport) type __stdcall
#define BT_DECLARE_NONSTD(type)	__declspec(dllexport) type
#define BT_DECLARE_DATA		__declspec(dllexport)
#endif

#ifdef __cplusplus
extern "C"{
#endif

typedef struct{
    char ip[256];
    int net_port;//bt �����˿�
    int ipc_port;//UIͨ�Ŷ˿�
    int pipe_port;//�ڲ��ܵ�ͨ�Ŷ˿�
	int use_upnp;//�Ƿ�����upnp��1�����ã�0��������

    int empty_start;
    HANDLE th_bt; //bt daemon�߳̾��
    struct ipc *cmdpipe;
}bt_arg_t;

struct btstat{
    long long num;
    enum ipc_tstate state;
    long long peers,tr_good;
    long long content_got,content_size,downloaded,uploaded,rate_up,
	 rate_down,tot_up;
    long long pieces_seen,torrent_pieces;
};

BT_DECLARE(int) bt_start_daemon(bt_arg_t *bt_arg);
BT_DECLARE(int) bt_stop_daemon(bt_arg_t *bt_arg);

BT_DECLARE(int) bt_add(const char *dir,const char *torrent,bt_arg_t *bt_arg);
BT_DECLARE(int) bt_add_url(const char *dir,const char *savename,const char *url,
	bt_arg_t *bt_arg);
BT_DECLARE(int) bt_del(int argc,char **argv,bt_arg_t *bt_arg);
BT_DECLARE(int) bt_stop(int argc,char **argv,bt_arg_t *bt_arg);
BT_DECLARE(int) bt_stopall(bt_arg_t *bt_arg);
BT_DECLARE(int) bt_start(int argc,char **argv,bt_arg_t *bt_arg);
BT_DECLARE(int) bt_stat(char *torrent,bt_arg_t *bt_arg,struct btstat *stat);

/**
 * up �ϴ����ʷ�ֵ(bytes �ֽڣ�
 * down �������ʷ�ֵ(bytes �ֽ�)
 */
BT_DECLARE(int) bt_rate(unsigned up, unsigned down, bt_arg_t *bt_arg);

/**
 * p2sp �ӿ�
 */
BT_DECLARE(int) bt_add_p2sp(char *torrent,const char *url,bt_arg_t *bt_arg); 

#ifdef __cplusplus
}
#endif
#endif

