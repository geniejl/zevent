#include "btpd.h"

/*
 * Called when a peer announces it's got a new piece.
 *
 * If the piece is missing or unfull we increase the peer's
 * wanted level and if possible call dl_on_download.
 */
void
dl_on_piece_ann(struct peer *p, uint32_t index)
{
    struct net *n = p->n;
    struct piece *pc;
    n->piece_count[index]++;
    if (cm_has_piece(n->tp, index))
        return;
    pc = dl_find_piece(n, index);
    if (n->endgame) {
        assert(pc != NULL);
        peer_want(p, index);
        if (peer_leech_ok(p))
            dl_assign_requests_eg(p);
    } else if (pc == NULL) {
        peer_want(p, index);
        if (peer_leech_ok(p)) {
            pc = dl_new_piece(n, index);
            dl_piece_assign_requests(pc, p);
        }
    } else if (!piece_full(pc)) {
        peer_want(p, index);
        if (peer_leech_ok(p))
            dl_piece_assign_requests(pc, p);
    }
}

void
dl_on_download(struct peer *p)
{
    struct net *n = p->n;
    if (n->endgame)
    {
	//�˴��д�����
	//if(p->ptype == BT_PEER)
	    dl_assign_requests_eg(p);
    }
    else
        dl_assign_requests(p);
}

void
dl_on_unchoke(struct peer *p)
{
    if (peer_leech_ok(p))
        dl_on_download(p);
}

void
dl_on_undownload(struct peer *p)
{
    if (!p->n->endgame || p->ptype == HTTP_PEER)
        dl_unassign_requests(p);
    else
    {
	if(p->ptype == BT_PEER)
	    dl_unassign_requests_eg(p);
    }
}

void
dl_on_choke(struct peer *p)
{
    if (p->nreqs_out > 0)
        dl_on_undownload(p);
}

/**
 * Called when a piece has been tested positively.
 */
void
dl_on_ok_piece(struct net *n, uint32_t piece)
{
    struct peer *p;
    struct piece *pc = dl_find_piece(n, piece);
    struct net_buf *have;

    btpd_log(BTPD_L_POL, "Got piece: %u.\r\n", pc->index);

    have = nb_create_have(pc->index);
    nb_hold(have);
    BTPDQ_FOREACH(p, &n->peers, p_entry)
        if (!peer_has(p, pc->index) && p->ptype == BT_PEER)
            peer_send(p, have);
    nb_drop(have);

    //对结束下载策略做了调�?    
	if (n->endgame)
	BTPDQ_FOREACH(p, &n->peers, p_entry)
	{
		if(p == BT_PEER)
	    peer_unwant(p, pc->index);
	}

    piece_log_good(pc);

    assert(pc->nreqs == 0);
    piece_free(pc);
}

/*
 * Called when a piece has been tested negatively.
 */
void
dl_on_bad_piece(struct net *n, uint32_t piece)
{
    struct piece *pc = dl_find_piece(n, piece);
    uint32_t i = 0;

    btpd_log(BTPD_L_ERROR, "Bad hash for piece %u of '%s'.\r\n",
        pc->index, torrent_name(n->tp));

    for (i = 0; i < pc->nblocks; i++)
        clear_bit(pc->down_field, i);

    pc->ngot = 0;
    pc->nbusy = 0;

    piece_log_bad(pc);

    if (n->endgame) {
        struct peer *p;
        BTPDQ_FOREACH(p, &n->peers, p_entry) {
            if (p->ptype == BT_PEER && peer_leech_ok(p) && peer_requestable(p, pc->index))
                dl_assign_requests_eg(p);
        }
    } else
        dl_on_piece_unfull(pc);
}

void
dl_on_new_peer(struct peer *p)
{
}

void dl_on_lost_peer_p2s(struct peer *p)
{
    assert(p->ptype == HTTP_PEER);
    while(p->nreqs_out > 0) {
	struct block_request *req = BTPDQ_FIRST(&p->my_reqs);
	struct piece *pc = dl_find_piece(p->n,req->p->in.pc_index);
	int was_full = piece_full(pc);

	while(req != NULL) {
	    struct block_request *next = BTPDQ_NEXT(req,p_entry);

	    if(!p->n->endgame)
	    {
		uint32_t blki = req->p->in.pc_begin / PIECE_BLOCKLEN;
		assert(has_bit(pc->down_field,blki));
		clear_bit(pc->down_field,blki);
		pc->nbusy--;
	    }

	    BTPDQ_REMOVE(&p->my_reqs,req,p_entry);
	    p->nreqs_out--;
	    BTPDQ_REMOVE(&pc->reqs,req,blk_entry);
	    nb_drop(req->msg);
	    free(req);
	    pc->nreqs--;

	    while(next != NULL && next->p->in.pc_index != pc->index)
		next = BTPDQ_NEXT(next,p_entry);
	    req = next;
	}

	if(was_full && !piece_full(pc))
	    dl_on_piece_unfull(pc);
    }
    assert(BTPDQ_EMPTY(&p->my_reqs));
}

void
dl_on_lost_peer(struct peer *p)
{
    struct net *n = p->n;
    uint32_t i = 0;

    for (i = 0; i < n->tp->npieces; i++)
        if (peer_has(p, i))
            n->piece_count[i]--;

    if (p->nreqs_out > 0)
        dl_on_undownload(p);
}

void
dl_on_block(struct peer *p, struct block_request *req,
    uint32_t index, uint32_t begin, uint32_t length, const uint8_t *data)
{
    struct net *n = p->n;
    struct piece *pc = dl_find_piece(n, index);

    piece_log_block(pc, p, begin);
    cm_put_bytes(p->n->tp, index, begin, data, length);
    pc->ngot++;

    if (n->endgame) {
        struct block_request *req, *next;
	uint32_t mbegin;
        struct net_buf *cancel = nb_create_cancel(index, begin, length);
        nb_hold(cancel);
        BTPDQ_FOREACH(req, &pc->reqs, blk_entry) {
	    if(req->p->ptype == HTTP_PEER)
		mbegin = req->p->in.pc_begin;
	    else
		mbegin = nb_get_begin(req->msg);
	    if(mbegin == begin) {
		if(req->p != p && req->p->ptype == BT_PEER)
		    peer_cancel(req->p,req,cancel);
		pc->nreqs--;
	    }
        }
        nb_drop(cancel);
        dl_piece_reorder_eg(pc);
        BTPDQ_FOREACH_MUTABLE(req, &pc->reqs, blk_entry, next) {
            if (nb_get_begin(req->msg) != begin)
                continue;
            BTPDQ_REMOVE(&pc->reqs, req, blk_entry);
            nb_drop(req->msg);
            if (peer_leech_ok(req->p))
                dl_assign_requests_eg(req->p);
            free(req);
        }
        if (pc->ngot == pc->nblocks)
            cm_test_piece(pc->n->tp, pc->index);
    } else {
        BTPDQ_REMOVE(&pc->reqs, req, blk_entry);
        nb_drop(req->msg);
        free(req);
        pc->nreqs--;
        // XXX: Needs to be looked at if we introduce snubbing.
        clear_bit(pc->down_field, begin / PIECE_BLOCKLEN);
        pc->nbusy--;
        if (pc->ngot == pc->nblocks)
            cm_test_piece(pc->n->tp, pc->index);
        if (peer_leech_ok(p))
            dl_assign_requests(p);
    }
}

void 
dl_on_block_p2sp(struct peer *p,
	uint32_t index,uint32_t begin,uint32_t length,const uint8_t *data)
{
    struct net *n = p->n;
    struct piece *pc = dl_find_piece(n,index);

    piece_log_block(pc,p,begin);
    cm_put_bytes(p->n->tp,index,begin,data,length);
    pc->ngot++;

    if(n->endgame) {
	struct block_request *req,*next;
	uint32_t mbegin;

	struct net_buf *cancel = nb_create_cancel(index,begin,length);
	nb_hold(cancel);
	BTPDQ_FOREACH(req,&pc->reqs,blk_entry) {
	    if(req->p->ptype == HTTP_PEER)
		mbegin = req->p->in.pc_begin;
	    else
		mbegin = nb_get_begin(req->msg);
	    if(mbegin == begin) {
		if (req->p != p && req->p->ptype == BT_PEER)
		    peer_cancel(req->p,req,cancel);
		pc->nreqs--;
	    }
	}
	nb_drop(cancel);

	dl_piece_reorder_eg(pc);
	
	if(pc->ngot == pc->nblocks)
	    cm_test_piece(pc->n->tp,pc->index);
    }
    else{
	// XXX:Need to be looked at if we introduce snubbing.
	clear_bit(pc->down_field,begin / PIECE_BLOCKLEN);
	pc->nbusy--;
	if (pc->ngot == pc->nblocks)
	    cm_test_piece(pc->n->tp,pc->index);
	if(peer_leech_ok(p))
	{
	    p->in.msg_len = 0;
	    dl_assign_requests(p);
	}
    }
}
