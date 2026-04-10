/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "koala.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
func fib(v int) int {
    if v < 2 { return v }
    return fib(v - 1) + fib(v - 2)
}

@__init__[0,1]:
0000:  00000000   nop
0001:  62000000   ret_void
@fib[2,15]:
0002:  00000001   nop
0003:  1E010002   int.cmplt_imm r1, r0, #2
0004:  44010001   jmp_false r1, 1
0005:  5E000000   ret r0
0006:  08010001   int.sub_imm r1, r0, #1
0007:  59000001   push r1
0008:  5D000101   call flg=0, r1, #1
0009:  FFFFFFF8   data (rel32=-8)
0010:  08000002   int.sub_imm r0, r0, #2
0011:  59000000   push r0
0012:  5D000001   call flg=0, r0, #1
0013:  FFFFFFF4   data (rel32=-12)
0014:  05000100   int.add r0, r1, r0
0015:  5E000000   ret r0
@main[16,23]:
0016:  00000002   nop
0017:  5A00000A   push_int_imm #10
0018:  5D000001   call flg=0, r0, #1
0019:  FFFFFFEE   data (rel32=-18)
0020:  59000000   push r0
0021:  5D1FFF01   call flg=1, #1
0022:  00000000   data (import_index=0)
0023:  62000000   ret_void
*/

int main(int argc, char *argv[])
{
    koala_initialize();

    Object *m = kl_new_module("fib");

    /* time = 3.8, python3.13.9 7.1, lua5.4 4.2 */
    // char *rom = mm_alloc(sizeof(void *) * 3 + sizeof(uint32_t) * 24);

    uint32_t _insns[] = {
        0x00000000, 0x62000000, 0x00000001, 0x1E010002, 0x44010001, 0x5E000000,
        0x08010001, 0x59000001, 0x5D000101, 0xFFFFFFF8, 0x08000002, 0x59000000,
        0x5D000001, 0xFFFFFFF4, 0x05000100, 0x5E000000, 0x00000002, 0x5A000028,
        0x5D000001, 0xFFFFFFEE, 0x59000000, 0x5D1FFF01, 0x00000000, 0x62000000,
    };

    kl_mo_set_code(m, _insns, COUNT_OF(_insns));

    Object *obj = kl_new_code("__init__", m);
    CodeObject *code = (CodeObject *)obj;
    code->cs.start_pc = 0;
    code->cs.num_insns = 2;
    code->cs.nlocals = 0;
    kl_mo_add_func(m, obj);

    obj = kl_new_code("fib", m);
    code = (CodeObject *)obj;
    code->cs.start_pc = 2;
    code->cs.num_insns = 14;
    code->cs.nlocals = 2;
    kl_mo_add_func(m, obj);

    obj = kl_new_code("main", m);
    code = (CodeObject *)obj;
    code->cs.start_pc = 16;
    code->cs.num_insns = 8;
    code->cs.nlocals = 1;
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
