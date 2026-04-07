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

@ack:
[start_pc: 7, insns: 16, nlocals: 3, max_call_args: 2]

@__init__[0,6]:
0000:  02000003   load_int_imm r0, #3
0001:  0201000B   load_int_imm r1, #11
0002:  5D000002   call flg=0, r0, #2
0003:  00000001   data (rel32=1)
0004:  5D1FFF01   call flg=1, #1
0005:  00000000   data (import_index=0)
0006:  63000000   ret_void
@ack[7,22]:
0007:  48000002   jmp_int_ne_imm r0, #0, 2
0008:  06020101   int.add_imm r2, r1, #1
0009:  5F000002   ret r2
0010:  48010004   jmp_int_ne_imm r1, #0, 4
0011:  08000001   int.sub_imm r0, r0, #1
0012:  02010001   load_int_imm r1, #1
0013:  5E3FFF02   tail_call flg=3, #2
0014:  00000001   data
0015:  08020001   int.sub_imm r2, r0, #1
0016:  08040101   int.sub_imm r4, r1, #1
0017:  01003000   move r3, r0
0018:  5D000102   call flg=0, r1, #2
0019:  00000001   data (rel32=1)
0020:  01000002   move r0, r2
0021:  5E3FFF02   tail_call flg=3, #2
0022:  00000001   data
*/

int main(int argc, char *argv[])
{
    koala_initialize();

    Object *m = kl_new_module("ack");

    /* time = 3.8, python3.13.9 7.1, lua5.4 4.2 */
    // char *rom = mm_alloc(sizeof(void *) * 3 + sizeof(uint32_t) * 24);

    uint32_t _insns[] = {
        0x02000003, 0x0201000B, 0x5D000002, 0x00000001, 0x5D1FFF01, 0x00000000,
        0x63000000, 0x48000002, 0x06020101, 0x5F000002, 0x48010004, 0x08000001,
        0x02010001, 0x5E3FFF02, 0x00000001, 0x08020001, 0x08040101, 0x01003000,
        0x5D000102, 0x00000001, 0x01000002, 0x5E3FFF02, 0x00000001,
    };

    kl_mo_set_code(m, _insns, COUNT_OF(_insns));

    Object *obj = kl_new_code("__init__", m);
    CodeObject *code = (CodeObject *)obj;
    code->cs.start_pc = 0;
    code->cs.code_size = 7;
    code->cs.nlocals = 0;
    code->cs.max_call_args = 2;
    kl_mo_add_func(m, obj);

    obj = kl_new_code("ack", m);
    code = (CodeObject *)obj;
    code->cs.start_pc = 7;
    code->cs.code_size = 16;
    code->cs.nlocals = 3;
    code->cs.max_call_args = 2;
    kl_mo_add_func(m, obj);

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
