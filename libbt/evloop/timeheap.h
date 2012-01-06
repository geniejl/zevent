#ifndef BTPD_TIMEHEAP_H
#define BTPD_TIMEHEAP_H

struct timespec{
    time_t tv_sec;
    long tv_nsec;
};

struct th_handle {
    int i;
    void *data;
};

int timeheap_init(void);
int timeheap_destroy(void);
int timeheap_size(void);

int  timeheap_insert(struct th_handle *h, struct timespec *t);
void timeheap_remove(struct th_handle *h);
void timeheap_change(struct th_handle *h, struct timespec *t);

void *timeheap_remove_top(void);
struct timespec timeheap_top(void);

#endif
