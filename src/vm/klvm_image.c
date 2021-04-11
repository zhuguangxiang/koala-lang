/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangXiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

/* translate koala ir into koala byte code */

#include "image.h"
#include "klvm.h"

#ifdef __cplusplus
extern "C" {
#endif

klc_image_t *klvm_to_image(KLVMModule *m)
{
    klc_image_t *klc = klc_create();

    KLVMVar **var;
    VectorForEach(var, &m->vars)
    {
        klc_add_var(klc, (*var)->name, &kl_type_int32, 0);
    }

    klc_show(klc);
    klc_destroy(klc);
    return NULL;
}

#ifdef __cplusplus
}
#endif
