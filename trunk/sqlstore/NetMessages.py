# -*- coding: utf-8 -*- 
class Packets:
        DEF_MSGTYPE_CONFIRM = 0x0000 
	DEF_MSGTYPE_REJECT = 0x0001

	MSGID_REQUEST_LOGIN = 0x00000001
	MSGID_RESPONSE_LOGIN = 0x10000001

	MSGID_REQUEST_ENTERGAME = 0x00000002
	MSGID_RESPONSE_ENTERGAME = 0x10000002
	
	MSGID_REQUEST_LEAVEGAME = 0x00000004
        MSGID_RESPONSE_LEAVEGAME = 0x10000004

	MSGID_REQUEST_NEWACCOUNT = 0x00000008
        MSGID_RESPONSE_NEWACCOUNT = 0x10000008

	MSGID_REQUEST_NEWCHARACTER = 0x00000010
        MSGID_RESPONSE_NEWCHARACTER = 0x10000010

	MSGID_REQUEST_GETCHARLIST = 0x00000011
        MSGID_RESPONSE_GETCHARLIST = 0x10000011

	MSGID_REQUEST_BINDGS = 0x00000111
	MSGID_RESPONSE_BINDGS = 0x10000111

	#for gs
	MSGID_REQUEST_REGGS = 0x01000001
	MSGID_RESPONSE_REGGS = 0x11000001
	#转发消息
	MSGID_REQUEST_DATA2GS = 0x0000000F
	MSGID_RESPONSE_DATA2GS = 0x1000000F
	MSGID_REQUEST_DATA2CLIENTS = 0x000000FF

	#sqlstore
	MSGID_REQUEST_EXECSQL = 0x0000002F
	MSGID_RESPONSE_EXECSQL = 0x1000002F
	MSGID_REQUEST_EXECPROC = 0x0000003F
	MSGID_RESPONSE_EXECPROC = 0x1000003F
	MSGID_REQUEST_QUERY = 0x0000004F
	MSGID_RESPONSE_QUERY = 0x1000004F
	#disconnect
	MSGID_NOTIFY_DISCONNECT = 0x11111111

	DEF_LOGRESMSGTYPE_PASSWORDMISMATCH = 0x0001
	DEF_LOGRESMSGTYPE_NOTEXISTINGACCOUNT = 0x0002
	DEF_LOGRESMSGTYPE_NOTEXISTINGCHARACTER = 0x003
 
	DEF_ENTERGAMEMSGTYPE_NEW = 0x0F1C

	DEF_ENTERGAMERESTYPE_PLAYING = 0x0F20
	DEF_ENTERGAMERESTYPE_REJECT = 0x0F21
	DEF_ENTERGAMERESTYPE_CONFIRM = 0x0F22
	DEF_ENTERGAMERESTYPE_FORCEDISCONN = 0x0F23

