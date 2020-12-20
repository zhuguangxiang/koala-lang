/*===-- state.h - Koala Global & Thread State ---------------------*- C -*-===*\
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) 2020 James, https://github.com/zhuguangxiang                 *|
|*                                                                            *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* This header declares global & thread state and their interfaces.           *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_STATE_H_
#define _KOALA_STATE_H_

#include "koala.h"

#ifdef __cplusplus
extern "C" {
#endif

/* minimum statkc size */
#define STACK_MINIMUM_SIZE 512

/* forward declaration */
typedef struct CallInfo CallInfo;

/* `global state`, shared by all threads */
typedef struct KoalaGlobal {
} KoalaGlobal;

/* func call info */
struct CallInfo {
    /* back call info */
    CallInfo *back;
    /* top for this func */
    kl_value_t *top;
    /* func object */
    kl_value_t *func;
    /* code index */
    int saved_index;
};

/* thread state structure */
struct KoalaState {
    /* first free slot */
    kl_value_t *top;

    /* call info */
    CallInfo *ci;
    /* call info num */
    int nci;

    /* stack size */
    int stacksize;
    /* stack base */
    kl_value_t *stack;
    /* last free slot */
    kl_value_t *stack_last;

    /* base call info */
    CallInfo base_ci;
};

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_STATE_H_ */
