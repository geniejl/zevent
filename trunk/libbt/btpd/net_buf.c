#include "btpd.h"
#include "http_client.h"
#include "stream.h"

static struct net_buf *m_choke;
static struct net_buf *m_unchoke;
static struct net_buf *m_interest;
static struct net_buf *m_uninterest;
static struct net_buf *m_keepalive;

static void
kill_buf_no(char *buf, size_t len)
{
}

static void
kill_buf_free(char *buf, size_t len)
{
    free(buf);
}

static void
kill_buf_abort(char *buf, size_t len)
{
    abort();
}

static struct net_buf *
nb_create_alloc(short type, size_t len)
{
    struct net_buf *nb = btpd_calloc(1, sizeof(*nb) + len);
    nb->type = type;
    nb->buf = (char *)(nb + 1);
    nb->len = len;
    nb->kill_buf = kill_buf_no;
    return nb;
}

static struct net_buf *
nb_create_set(short type, char *buf, size_t len,
    void (*kill_buf)(char *, size_t))
{
    struct net_buf *nb = btpd_calloc(1, sizeof(*nb));
    nb->type = type;
    nb->buf = buf;
    nb->len = len;
    nb->kill_buf = kill_buf;
    return nb;
}

static struct net_buf *
nb_create_onesized(char mtype, int btype)
{
    struct net_buf *out = nb_create_alloc(btype, 5);
    enc_be32(out->buf, 1);
    out->buf[4] = mtype;
    return out;
}

static struct net_buf *
nb_singleton(struct net_buf *nb)
{
    nb_hold(nb);
    nb->kill_buf = kill_buf_abort;
    return nb;
}

struct net_buf *
nb_create_keepalive(void)
{
    if (m_keepalive == NULL) {
        m_keepalive = nb_create_alloc(NB_KEEPALIVE, 4);
        enc_be32(m_keepalive->buf, 0);
        nb_singleton(m_keepalive);
    }
    return m_keepalive;
}

struct net_buf *
nb_create_piece(uint32_t index, uint32_t begin, size_t blen)
{
    struct net_buf *out;
    out = nb_create_alloc(NB_PIECE, 13);
    enc_be32(out->buf, 9 + blen);
    out->buf[4] = MSG_PIECE;
    enc_be32(out->buf + 5, index);
    enc_be32(out->buf + 9, begin);
    return out;
}

struct net_buf *
nb_create_torrentdata(void)
{
    struct net_buf *out;
    out = nb_create_set(NB_TORRENTDATA, NULL, 0, kill_buf_no);
    return out;
}

int
nb_torrentdata_fill(struct net_buf *nb, struct torrent *tp, uint32_t index,
    uint32_t begin, uint32_t length)
{
    int err;
    uint8_t *content;
    assert(nb->type == NB_TORRENTDATA && nb->buf == NULL);
    if ((err = cm_get_bytes(tp, index, begin, length, &content)) != 0)
        return err;
    nb->buf = content;
    nb->len = length;
    nb->kill_buf = kill_buf_free;
    return 0;
}

struct net_buf *
nb_create_request(uint32_t index, uint32_t begin, uint32_t length)
{
    struct net_buf *out = nb_create_alloc(NB_REQUEST, 17);
    enc_be32(out->buf, 13);
    out->buf[4] = MSG_REQUEST;
    enc_be32(out->buf + 5, index);
    enc_be32(out->buf + 9, begin);
    enc_be32(out->buf + 13, length);
    return out;
}

int f_seek(struct mi_file *files,unsigned nfiles,fpos_t *off)
{
    int i;

    for(i = 0; i< nfiles && (*off) >= files[i].length; i++)
	(*off) -= files[i].length;

    return i < nfiles ? i : -1;
}

struct net_buf *nb_create_request_p2sp(struct peer *peer_p2sp,uint32_t index,
	uint32_t begin,uint32_t length,short type)
{
    struct piece *pc;
    uint32_t block;
    const char *fname = NULL;
    struct net_buf *out = NULL;
    char uri[1024],req[1024],enc_uri[2048];
    int i = 0,fidx = 0;
    fpos_t range_start = 0;
    int ret,len;

    wchar_t ucs2_path[MAX_PATH];
    size_t inwords;

    memset(uri,0,sizeof(uri));
    memset(req,0,sizeof(req));
    memset(enc_uri,0,sizeof(enc_uri));

    range_start = index * peer_p2sp->n->tp->piece_length + begin;
    fidx = f_seek(peer_p2sp->n->tp->files,peer_p2sp->n->tp->nfiles,&range_start);
    if(fidx < 0)
	return NULL;
    
    sprintf(uri,"%s%s",peer_p2sp->url.uri,peer_p2sp->n->tp->files[fidx].path);
    while(uri[i])
    {
	if(uri[i] == '\\')
	    uri[i] = '/';
	i++;
    }

    inwords = MultiByteToWideChar(CP_ACP,0,uri,-1,ucs2_path,
	    sizeof(ucs2_path)/sizeof(ucs2_path[0]));
    http_uri_encode(ucs2_path,inwords,enc_uri);

    sprintf(req,"GET %s HTTP/1.1\r\n"
	    "Host: %s:%hu\r\n"
	    "Accept:*/*\r\n"
	    "Pragma: no-cache\r\n"
	    "Connection: keep-alive\r\n"
	    //"Connection: close\r\n"
	    "Content-Type:application/octet-stream\r\n"
	    "User-Agent: btpd\r\n"
	    "Range: bytes=%lld-%lld\r\n"
	    "\r\n",enc_uri,peer_p2sp->url.ip,peer_p2sp->url.port,
	    range_start,range_start+(long)length-1);

    len = strlen(req);
    out = nb_create_alloc(type,len);
    memcpy(out->buf,req,len);

    pc = dl_find_piece(peer_p2sp->n,index);
    block = peer_p2sp->in.pc_curblock;
    peer_p2sp->in.st_bytes = torrent_block_size(peer_p2sp->n->tp,index,
	    torrent_piece_blocks(peer_p2sp->n->tp,index),block);
    peer_p2sp->in.pc_index = index;
    peer_p2sp->in.pc_begin = block * PIECE_BLOCKLEN;
    
    return out;
}

struct net_buf *
nb_create_cancel(uint32_t index, uint32_t begin, uint32_t length)
{
    struct net_buf *out = nb_create_alloc(NB_CANCEL, 17);
    enc_be32(out->buf, 13);
    out->buf[4] = MSG_CANCEL;
    enc_be32(out->buf + 5, index);
    enc_be32(out->buf + 9, begin);
    enc_be32(out->buf + 13, length);
    return out;
}

struct net_buf *
nb_create_have(uint32_t index)
{
    struct net_buf *out = nb_create_alloc(NB_HAVE, 9);
    enc_be32(out->buf, 5);
    out->buf[4] = MSG_HAVE;
    enc_be32(out->buf + 5, index);
    return out;
}

struct net_buf *
nb_create_multihave(struct torrent *tp)
{
    uint32_t i = 0,count = 0;
    uint32_t have_npieces = cm_pieces(tp);
    struct net_buf *out = nb_create_alloc(NB_MULTIHAVE, 9 * have_npieces);
    for (i = 0, count = 0; count < have_npieces; i++) {
        if (cm_has_piece(tp, i)) {
            enc_be32(out->buf + count * 9, 5);
            out->buf[count * 9 + 4] = MSG_HAVE;
            enc_be32(out->buf + count * 9 + 5, i);
            count++;
        }
    }
    return out;
}

struct net_buf *
nb_create_unchoke(void)
{
    if (m_unchoke == NULL)
        m_unchoke = nb_singleton(nb_create_onesized(MSG_UNCHOKE, NB_UNCHOKE));
    return m_unchoke;
}

struct net_buf *
nb_create_choke(void)
{
    if (m_choke == NULL)
        m_choke = nb_singleton(nb_create_onesized(MSG_CHOKE, NB_CHOKE));
    return m_choke;
}

struct net_buf *
nb_create_uninterest(void)
{
    if (m_uninterest == NULL)
        m_uninterest =
            nb_singleton(nb_create_onesized(MSG_UNINTEREST, NB_UNINTEREST));
    return m_uninterest;
}

struct net_buf *
nb_create_interest(void)
{
    if (m_interest == NULL)
        m_interest =
            nb_singleton(nb_create_onesized(MSG_INTEREST, NB_INTEREST));
    return m_interest;
}

struct net_buf *
nb_create_bitfield(struct torrent *tp)
{
    uint32_t plen = ceil(tp->npieces / 8.0);

    struct net_buf *out = nb_create_alloc(NB_BITFIELD, 5);
    enc_be32(out->buf, plen + 1);
    out->buf[4] = MSG_BITFIELD;
    return out;
}

struct net_buf *
nb_create_bitdata(struct torrent *tp)
{
    uint32_t plen = ceil(tp->npieces / 8.0);
    struct net_buf *out =
        nb_create_set(NB_BITDATA, cm_get_piece_field(tp), plen, kill_buf_no);
    return out;
}

struct net_buf *
nb_create_shake(struct torrent *tp)
{
    struct net_buf *out = nb_create_alloc(NB_SHAKE, 68);
    memcpy(out->buf,"\x13""BitTorrent protocol\0\0\0\0\0\0\0\0", 28);
    memcpy(out->buf + 28,tp->tl->hash, 20);
    memcpy(out->buf + 48,btpd_get_peer_id(), 20);
    return out;
}

uint32_t
nb_get_index(struct net_buf *nb)
{
    switch (nb->type) {
    case NB_CANCEL:
    case NB_HAVE:
    case NB_PIECE:
    case NB_REQUEST:
        return dec_be32(nb->buf + 5);
    default:
        abort();
    }
}

uint32_t
nb_get_begin(struct net_buf *nb)
{
    switch (nb->type) {
    case NB_CANCEL:
    case NB_PIECE:
    case NB_REQUEST:
        return dec_be32(nb->buf + 9);
    default:
        abort();
    }
}

uint32_t
nb_get_length(struct net_buf *nb)
{
    switch (nb->type) {
    case NB_CANCEL:
    case NB_REQUEST:
        return dec_be32(nb->buf + 13);
    case NB_PIECE:
        return dec_be32(nb->buf) - 9;
    default:
        abort();
    }
}

int
nb_drop(struct net_buf *nb)
{
    assert(nb->refs > 0);
    nb->refs--;
    if (nb->refs == 0) {
        nb->kill_buf(nb->buf, nb->len);
        free(nb);
        return 1;
    } else
        return 0;
}

void
nb_hold(struct net_buf *nb)
{
    nb->refs++;
}
