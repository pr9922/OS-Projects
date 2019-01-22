#ifndef _SYS_H_
#define _SYS_H_

#include "stdint.h"

/****************/
/* System calls */
/****************/

typedef int ssize_t;
typedef unsigned int size_t;

/* exit */
extern void exit(int rc);

/* write */
extern ssize_t write(int fd, void* buf, size_t nbyte);

/* create semaphore */
extern int sem(uint32_t initial);

/* up */
extern int up(int id);

/* down */
extern int down(int id);

/* close */
extern int close(int id);

/* shutdown */
extern int shutdown(void);

/* wait */
extern int wait(int id, uint32_t *status);

#endif
