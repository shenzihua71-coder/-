#include <reg52.h>
#include "config.h"
#include "jdq.h"

#if LOCK_ACTIVE_LOW
#define LOCK_ON 0
#define LOCK_OFF 1
#else
#define LOCK_ON 1
#define LOCK_OFF 0
#endif

void lock_init(void)
{
    lock_close();
}

void lock_open(void)
{
    LOCK = LOCK_ON;
}

void lock_close(void)
{
    LOCK = LOCK_OFF;
}
