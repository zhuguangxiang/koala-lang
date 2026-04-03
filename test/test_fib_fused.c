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
@fib[2,14]:
0002:  00000001   nop
0003:  50000201   jmp_int_ge_imm r0, #2, 1
0004:  5E000000   ret r0
0005:  08010001   int.sub_imm r1, r0, #1
0006:  59000001   push r1
0007:  5D000101   call flg=0, r1, #1
0008:  FFFFFFF9   data (rel32=-7)
0009:  08000002   int.sub_imm r0, r0, #2
0010:  59000000   push r0
0011:  5D000001   call flg=0, r0, #1
0012:  FFFFFFF5   data (rel32=-11)
0013:  05000100   int.add r0, r1, r0
0014:  5E000000   ret r0
@main[15,22]:
0015:  00000002   nop
0016:  5A000028   push_int_imm #40
0017:  5D000001   call flg=0, r0, #1
0018:  FFFFFFEF   data (rel32=-17)
0019:  59000000   push r0
0020:  5D1FFF01   call flg=1, #1
0021:  00000000   data (import_index=0)
0022:  62000000   ret_void
*/

int main(int argc, char *argv[])
{
    koala_initialize();

    Object *m = kl_new_module("fib");

    /* time = 3.8, python3.13.9 7.1, lua5.4 4.2 */
    // char *rom = mm_alloc(sizeof(void *) * 3 + sizeof(uint32_t) * 24);

    uint32_t _insns[] = {
        0x00000000, 0x62000000, 0x00000001, 0x50000201, 0x5E000000, 0x08010001,
        0x59000001, 0x5D000101, 0xFFFFFFF9, 0x08000002, 0x59000000, 0x5D000001,
        0xFFFFFFF5, 0x05000100, 0x5E000000, 0x00000002, 0x5A000028, 0x5D000001,
        0xFFFFFFEF, 0x59000000, 0x5D1FFF01, 0x00000000, 0x62000000,
    };

    kl_mo_set_code(m, _insns, COUNT_OF(_insns));

    Object *obj = kl_new_code("__init__", m);
    CodeObject *code = (CodeObject *)obj;
    code->cs.start_pc = 0;
    code->cs.code_size = 2;
    code->cs.nlocals = 0;
    kl_mo_add_func(m, obj);

    obj = kl_new_code("fib", m);
    code = (CodeObject *)obj;
    code->cs.start_pc = 2;
    code->cs.code_size = 13;
    code->cs.nlocals = 2;
    kl_mo_add_func(m, obj);

    obj = kl_new_code("main", m);
    code = (CodeObject *)obj;
    code->cs.start_pc = 15;
    code->cs.code_size = 8;
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
