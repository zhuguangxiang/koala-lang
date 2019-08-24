/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "opcode.h"
#include "intobject.h"
#include "floatobject.h"
#include "stringobject.h"

Object *num_add(Object *x, Object *y)
{
  Object *z;
  if (Integer_Check(x) && Integer_Check(y)) {
    int64_t a, r;
    a = Integer_AsInt(x);
    if (Integer_Check(y)) {
      int64_t b = Integer_AsInt(y);
      r = (int64_t)((uint64_t)a + b);
    } else if (Byte_Check(y)) {
      int b = Byte_AsInt(y);
      r = (int64_t)((uint64_t)a + b);
    } else if (Float_Check(y)) {
      double b = Float_AsFlt(y);
      r = (int64_t)((double)a + b);
    } else {
      panic("Not implemented");
    }
    z = Integer_New(r);
  } else if (Float_Check(x) && Float_Check(y)) {
    double a, r;
    a = Float_AsFlt(x);
    if (Float_AsFlt(y)) {

    } else if (Integer_Check(y)) {

    } else if (Byte_Check(y)) {

    } else {
      panic("Not implemented");
    }
  } else if (String_Check(x) && String_Check(y)) {
    STRBUF(sbuf);
    strbuf_append(&sbuf, String_AsStr(x));
    strbuf_append(&sbuf, String_AsStr(y));
    String_Set(x, strbuf_tostr(&sbuf));
    strbuf_fini(&sbuf);
    z = OB_INCREF(x);
  } else {
    z = Object_Call(x, opcode_map(OP_ADD), y);
  }
}