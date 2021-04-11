/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2020-2021 James <zhuguangxiang@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "atom.h"
#include "klvm_type.h"

int main(int argc, char *argv[])
{
    InitAtom();

    KLVMInitTypes();

    KLVMTypeRef proto = KLVMProtoFromStr("Llang.Tuple;si", "b");
    VectorRef params = KLVMProtoParams(proto);
    KLVMTypeRef type = NULL;
    VectorGet(params, 0, &type);
    assert(type->kind == KLVM_TYPE_KLASS);
    char *path = KLVMGetKlassPath(type);
    char *name = KLVMGetKlassName(type);
    assert(!strcmp(path, "lang"));
    assert(!strcmp(name, "Tuple"));

    type = NULL;
    VectorGet(params, 1, &type);

    assert(type->kind == KLVM_TYPE_STR);
    type = NULL;
    VectorGet(params, 2, &type);
    assert(type->kind == KLVM_TYPE_INT64);

    type = NULL;
    VectorGet(params, 3, &type);
    assert(!type);

    type = KLVMProtoRet(proto);
    assert(type->kind == KLVM_TYPE_BOOL);

    KLVMFiniTypes();

    FiniAtom();

    return 0;
}
