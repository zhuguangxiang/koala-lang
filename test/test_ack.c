/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "koala.h"

#ifdef __cplusplus
extern "C" {
#endif

/*

@__init__[0,7]:
0000:  5A000003   push_int_imm #3
0001:  5A00000A   push_int_imm #10
0002:  5D000002   call flg=0, r0, #2
0003:  00000001   data (rel32=1)
0004:  59000000   push r0
0005:  5D1FFF01   call flg=1, #1
0006:  00000000   data (import_index=0)
0007:  62000000   ret_void
@ack[8,28]:
0008:  48000002   jmp_int_ne_imm r0, #0, 2
0009:  06020101   int.add_imm r2, r1, #1
0010:  5E000002   ret r2
0011:  48010006   jmp_int_ne_imm r1, #0, 6
0012:  08020001   int.sub_imm r2, r0, #1
0013:  59000002   push r2
0014:  5A000001   push_int_imm #1
0015:  5D000202   call flg=0, r2, #2
0016:  00000001   data (rel32=1)
0017:  5E000002   ret r2
0018:  08020001   int.sub_imm r2, r0, #1
0019:  08010101   int.sub_imm r1, r1, #1
0020:  59000000   push r0
0021:  59000001   push r1
0022:  5D000002   call flg=0, r0, #2
0023:  00000001   data (rel32=1)
0024:  59000002   push r2
0025:  59000000   push r0
0026:  5D000002   call flg=0, r0, #2
0027:  00000001   data (rel32=1)
0028:  5E000000   ret r0
*/

int main(int argc, char *argv[])
{
    koala_initialize();

    Object *m = kl_new_module("ack");

    /* time = 3.8, python3.13.9 7.1, lua5.4 4.2 */
    // char *rom = mm_alloc(sizeof(void *) * 3 + sizeof(uint32_t) * 24);

    uint32_t _insns[] = {
        0x5A000003, 0x5A00000A, 0x5D000002, 0x00000001, 0x59000000, 0x5D1FFF01,
        0x00000000, 0x62000000, 0x48000002, 0x06020101, 0x5E000002, 0x48010006,
        0x08020001, 0x59000002, 0x5A000001, 0x5D000202, 0x00000001, 0x5E000002,
        0x08020001, 0x08010101, 0x59000000, 0x59000001, 0x5D000002, 0x00000001,
        0x59000002, 0x59000000, 0x5D000002, 0x00000001, 0x5E000000,
    };

    kl_mo_set_code(m, _insns, COUNT_OF(_insns));

    Object *obj = kl_new_code("__init__", m);
    CodeObject *code = (CodeObject *)obj;
    code->cs.start_pc = 0;
    code->cs.num_insns = 8;
    code->cs.nlocals = 1;
    kl_mo_add_func(m, obj);

    obj = kl_new_code("ack", m);
    code = (CodeObject *)obj;
    code->cs.start_pc = 8;
    code->cs.num_insns = 21;
    code->cs.nlocals = 3;
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
