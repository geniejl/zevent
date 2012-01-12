#ifndef BT_IF_H
#define BT_IF_H

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

typedef struct bt_t bt_t;

typedef struct{
    int net_port;//bt�˿� Ϊ0 ����������˿�
	int use_upnp;//�Ƿ�����upnp��1�����ã�0��������
    int empty_start;//�Ƿ�����������������Ƿ�������
}bt_arg_t;

typedef struct{
	short ipc_port;
}btcli_arg_t;

enum ipc_tstate {
	IPC_TSTATE_INACTIVE,//δ��ʼ
	IPC_TSTATE_START,   //����ʼ
	IPC_TSTATE_STOP,    //����
	IPC_TSTATE_LEECH,   //��������
	IPC_TSTATE_SEED     //������
};

struct btstat{
    long long num;
    enum ipc_tstate state;
    long long peers,tr_good;
    long long content_got,content_size,downloaded,uploaded,rate_up,
	 rate_down,tot_up;
    long long pieces_seen,torrent_pieces;
};

/*
����bt��̨���񣬻�ú����������
@bt_arg ��������
@bt ������Ҫ�õ��ľ��
@����ֵ rv��
 0 �ɹ�
 -1 ʧ��
*/
BT_DECLARE(int) bt_start_daemon(bt_arg_t *bt_arg,bt_t **bt);

/*
����bt����
@bt �������
@����ֵ rv��
   0 �ɹ�
   -1 ʧ��
*/
BT_DECLARE(int) bt_stop_daemon(bt_t *bt);

/*
����bt�ͻ��ˣ���ú����������
@btcli_arg ��������
@bt ������Ҫ�õ��ľ��
@����ֵ rv��
0 �ɹ�
-1 ʧ��
*/
BT_DECLARE(int) bt_start_client(btcli_arg_t *btcli_arg,bt_t **bt);

/*
�Ͽ�bt�ͻ���
@bt �������
@����ֵ rv��
0 �ɹ�
-1 ʧ��
*/
BT_DECLARE(int) bt_stop_client(bt_t *bt);

/*
��ӱ�������
@dir ���ش��Ŀ¼
@torrent ������
@bt bt�������
@����ֵ rv��
  0 �ɹ�
  1 �Ѵ���
  -1 ʧ��
*/
BT_DECLARE(int) bt_add(const char *dir,const char *torrent,bt_t *bt);

/*
web seed�ӿ�
@dir ���Ŀ¼
@savename Զ���������ص����غ�ı������֣���·����
@url ���ӵ�url��ַ
@bt bt�������
@����ֵ rv��
0 �ɹ�
1 �Ѵ���
-1 ʧ��
*/
BT_DECLARE(int) bt_add_url(const char *dir,const char *savename,const char *url,
	bt_t *bt);

/*
ɾ������
@argc ɾ����������
@argv[] ɾ��������������
���ӿڻ�ɾ���������ڱ��ص�������Ϣ��
eg:
   char *torrents[2]={"1.torrent","2.torrent"};
   bt_del(2,torrents,bt);
@����ֵ rv��
   0 �ɹ�
   -1 ʧ��
*/
BT_DECLARE(int) bt_del(int argc,char **argv,bt_t *bt);
/*
ֹͣ����
@����ֵ rv��
0 �ɹ�
-1 ʧ��
*/
BT_DECLARE(int) bt_stop(int argc,char **argv,bt_t *bt);
/*
ֹͣ��������
@����ֵ rv��
0 �ɹ�
-1 ʧ��
*/
BT_DECLARE(int) bt_stopall(bt_t *bt);
/*
��ʼ����
@����ֵ rv��
0 �ɹ�
-1 ʧ��
*/
BT_DECLARE(int) bt_start(int argc,char **argv,bt_t *bt);

/*
�鿴���ӵ�����״̬��Ϣ
@torrent ����������·��)
@bt �������
@stat ״̬��Ϣ
@����ֵ rv��
0 �ɹ�
-1 ʧ��
*/
BT_DECLARE(int) bt_stat(char *torrent,bt_t *bt,struct btstat *stat);

/**
 * up �ϴ����ʷ�ֵ(bytes �ֽڣ�
 * down �������ʷ�ֵ(bytes �ֽ�)
 @����ֵ rv��
 0 �ɹ�
 -1 ʧ��
 */
BT_DECLARE(int) bt_rate(unsigned up, unsigned down, bt_t *bt);

/**
 * p2sp �ӿ�
 * ��Ч���ӽӿ�
 * @torrent ����������·��)
 * @url ��Դweb��ַ
 * @bt �������
 * @���ӿ�Ϊÿ��url����һ��http���ӣ������Ҫ����������أ����ԼӴ��ٶȣ�����5������
 * �ɶ�ε��ñ��ӿڣ���ʹ��ͬһurl)
 @����ֵ rv��
 0 �ɹ�
 -1 ʧ��
 */
BT_DECLARE(int) bt_add_p2sp(char *torrent,const char *url,bt_t *bt); 

#ifdef __cplusplus
}
#endif
#endif

