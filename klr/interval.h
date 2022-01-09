/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KLVM_INTERVAL_H_
#define _KLVM_INTERVAL_H_

#if !defined(_KLVM_H_)
#error "Only <klvm/klvm.h> can be included directly."
#endif

/* no error here */
#include "klvm.h"

#ifdef __cplusplus
extern "C" {
#endif

/* live interval */
struct _KLVMInterval {
    /* owner Value of this interval */
    KLVMValDef *owner;
    /* ranges of this interval */
    List range_list;
    /* registers(uses) in this interval */
    List use_list;
    /* link in linear-scan */
    List link;
};

/* live range */
struct _KLVMRange {
    /* link in interval */
    List link;
    /* end is exclusive */
    uint32 start;
    uint32 end;
};

KLVMInterval *interval_alloc(KLVMValDef *owner);

#ifdef __cplusplus
}
#endif

#endif /* _KLVM_INTERVAL_H_ */
