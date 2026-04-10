/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "koala.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
@__init__:
[start_pc: 0, insns: 7, nlocals: 0, max_call_args: 2]

@sum:
[start_pc: 7, insns: 7, nlocals: 3, max_call_args: 2]

@__init__[0,6]:
0000:  04000000   loadk r0, #0
0001:  02010000   load_int_imm r1, #0
0002:  5D000002   call flg=0, r0, #2
0003:  00000001   data (rel32=1)
0004:  5D1FFF01   call flg=1, #1
0005:  00000000   data (import_index=0)
0006:  63000000   ret_void
@sum[7,13]:
0007:  48000001   jmp_int_ne_imm r0, #0, 1
0008:  5F000001   ret r1
0009:  08020001   int.sub_imm r2, r0, #1
0010:  05010100   int.add r1, r1, r0
0011:  01000002   move r0, r2
0012:  5E0FFF02   tail_call flg=0, #2
0013:  00000001   data (rel32=1)


*/

int main(int argc, char *argv[])
{
    koala_initialize();

    Object *m = kl_new_module("sum");

    /* time = 3.8, python3.13.9 7.1, lua5.4 4.2 */
    // char *rom = mm_alloc(sizeof(void *) * 3 + sizeof(uint32_t) * 24);

    uint32_t _insns[] = {
        0x04000000, 0x02010000, 0x5D000002, 0x00000001, 0x5D1FFF01,
        0x00000000, 0x63000000, 0x48000001, 0x5F000001, 0x08020001,
        0x05010100, 0x01000002, 0x5E0FFF02, 0x00000001,
    };

    kl_mo_set_code(m, _insns, COUNT_OF(_insns));

    Object *obj = kl_new_code("__init__", m);
    CodeObject *code = (CodeObject *)obj;
    code->cs.start_pc = 0;
    code->cs.num_insns = 7;
    code->cs.nlocals = 0;
    code->cs.max_call_args = 2;
    kl_mo_add_func(m, obj);

    obj = kl_new_code("sum", m);
    code = (CodeObject *)obj;
    code->cs.start_pc = 7;
    code->cs.num_insns = 7;
    code->cs.nlocals = 3;
    code->cs.max_call_args = 2;
    kl_mo_add_func(m, obj);

    kl_mo_add_int(m, 100000000);
    kl_mo_add_import(m, IMPORT_KIND_FUNC, "std/builtin", "print");

    kl_init_module(m);

    kl_resolve_import(m);

    // kl_dump_module(m);

    kl_run_module(m);

    koala_finalize();
    return 0;
}

#ifdef __cplusplus
}
#endif
