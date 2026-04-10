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

func main() {
    print(fib(40))
}

@__init__:
[start_pc: 0, insns: 1, nlocals: 0, max_call_args: 0]

@fib:
[start_pc: 1, insns: 10, nlocals: 2, max_call_args: 1]

@main:
[start_pc: 11, insns: 6, nlocals: 0, max_call_args: 1]

@__init__[0,0]:
0000:  63000000   ret_void
@fib[1,10]:
0001:  50000201   jmp_int_ge_imm r0, #2, 1
0002:  5F000000   ret r0
0003:  08020001   int.sub_imm r2, r0, #1
0004:  5D000101   call flg=0, r1, #1
0005:  00000001   data (rel32=1)
0006:  08020002   int.sub_imm r2, r0, #2
0007:  5D000001   call flg=0, r0, #1
0008:  00000001   data (rel32=1)
0009:  05000100   int.add r0, r1, r0
0010:  5F000000   ret r0
@main[11,16]:
0011:  02000028   load_int_imm r0, #40
0012:  5D000001   call flg=0, r0, #1
0013:  00000001   data (rel32=1)
0014:  5D1FFF01   call flg=1, #1
0015:  00000000   data (import_index=0)
0016:  63000000   ret_void

*/

int main(int argc, char *argv[])
{
    koala_initialize();

    Object *m = kl_new_module("fib");

    /* time = 3.8, python3.13.9 7.1, lua5.4 4.2 */
    // char *rom = mm_alloc(sizeof(void *) * 3 + sizeof(uint32_t) * 24);

    uint32_t _insns[] = {
        0x63000000, 0x50000201, 0x5F000000, 0x08020001, 0x5D000101, 0x00000001,
        0x08020002, 0x5D000001, 0x00000001, 0x05000100, 0x5F000000, 0x02000028,
        0x5D000001, 0x00000001, 0x5D1FFF01, 0x00000000, 0x63000000,
    };

    kl_mo_set_code(m, _insns, COUNT_OF(_insns));

    Object *obj = kl_new_code("__init__", m);
    CodeObject *code = (CodeObject *)obj;
    code->cs.start_pc = 0;
    code->cs.num_insns = 1;
    code->cs.nlocals = 0;
    code->cs.max_call_args = 0;
    kl_mo_add_func(m, obj);

    obj = kl_new_code("fib", m);
    code = (CodeObject *)obj;
    code->cs.start_pc = 1;
    code->cs.num_insns = 10;
    code->cs.nlocals = 2;
    code->cs.max_call_args = 1;
    kl_mo_add_func(m, obj);

    obj = kl_new_code("main", m);
    code = (CodeObject *)obj;
    code->cs.start_pc = 11;
    code->cs.num_insns = 6;
    code->cs.nlocals = 0;
    code->cs.max_call_args = 1;
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
