/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangXiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "image.h"
#include "opcode.h"

void test_klc(void)
{
    klc_image_t *klc = klc_create();
    klc_add_var(klc, "foo", &kl_type_int, ACCESS_FLAGS_PUB);
    klc_add_var(klc, "bar", &kl_type_bool, ACCESS_FLAGS_FINAL);
    TypeDesc *proto = to_proto("ii", "z");
    klc_add_var(klc, "fn1", proto, 0);
    proto = to_proto("Pi:i", "Pz:");
    klc_add_var(klc, "fn2", proto, ACCESS_FLAGS_PUB);
    klc_add_int8(klc, 10);
    klc_add_int8(klc, 10);
    klc_add_int8(klc, 20);
    klc_add_int8(klc, -20);
    klc_add_int(klc, -20);
    klc_add_int16(klc, 10);
    klc_add_float32(klc, 10.2);
    klc_add_float(klc, -10.2);
    klc_add_string(klc, "hello汉");
    klc_add_char(klc, 0xe6b189);

    klc_add_bool(klc, 1);

    /*
    func add(a int, b int, c int) int {
        return a + b + c
    }
    */
    proto = to_proto("iii", "i");
    uint8_t codes[] = {
        OP_LOAD_0,
        OP_LOAD_1,
        OP_LOAD_2,
        OP_ADD,
        OP_ADD,
        OP_RET,
    };
    codeinfo_t ci = {
        .name = "add",
        .flags = ACCESS_FLAGS_PUB,
        .desc = proto,
        .codesize = COUNT_OF(codes),
        .codes = codes,
    };
    klc_add_func(klc, &ci);

    /*
    func sub(a int, b int, c int) int {
        return a - b - c
    }
    */
    proto = to_proto("iii", "i");
    uint8_t codes2[] = {
        OP_LOAD_0,
        OP_LOAD_1,
        OP_LOAD_2,
        OP_SUB,
        OP_SUB,
        OP_RET,
    };
    ci = (codeinfo_t) {
        .name = "sub",
        .flags = 0,
        .desc = proto,
        .codesize = COUNT_OF(codes2),
        .codes = codes2,
    };
    klc_add_func(klc, &ci);

    klc_show(klc);
    klc_write_file(klc, "foo.klc");
    klc_destroy(klc);

    klc = klc_read_file("foo.klc");
    klc_show(klc);
    klc_destroy(klc);
}

int main(int argc, char *argv[])
{
    test_klc();
    return 0;
}
