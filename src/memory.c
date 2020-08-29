#include "db.h"

/* Release memory */
void mbj_release(void *ptr) {
    mjb.memory_free(ptr);
}
