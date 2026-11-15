/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_GCMODULE_H_
#define _KOALA_GCMODULE_H_

#ifdef __cplusplus
extern "C" {
#endif

/* The koala `std/gc` module, implemented in modules/gc.c. */

/* gc debug flags, independent of the koala log level */
#define GC_DEBUG_STATS 0x01 /* summary of each collection */
#define GC_DEBUG_FREE  0x02 /* each freed block */

/* reads KOALA_GC_DEBUG=stats,free; called after gc_init */
void gc_debug_init(void);

void gc_set_debug(int flags);
int gc_get_debug(void);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_GCMODULE_H_ */
