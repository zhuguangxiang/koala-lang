/*
 MIT License

 Copyright (c) 2018 James, https://github.com/zhuguangxiang

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
*/

#include "stddef.h"
#include "modinfo.h"

extern ModInfo __start_mod_0[];
extern ModInfo __stop_mod_0[];
extern ModInfo __start_mod_1[];
extern ModInfo __stop_mod_1[];
extern ModInfo __start_mod_2[];
extern ModInfo __stop_mod_2[];
extern ModInfo __start_mod_3[];
extern ModInfo __stop_mod_3[];

static struct {
  ModInfo *start;
  ModInfo *end;
} mod_levels[] = {
  {__start_mod_0, __stop_mod_0},
  {__start_mod_1, __stop_mod_1},
  {__start_mod_2, __stop_mod_2},
  {__start_mod_3, __stop_mod_3},
};

static void dummy_init(void)
{
}

REGISTER_MODULE0(dummy0, dummy_init, NULL);
REGISTER_MODULE1(dummy1, dummy_init, NULL);
REGISTER_MODULE2(dummy2, dummy_init, NULL);
REGISTER_MODULE3(dummy3, dummy_init, NULL);

static void do_modinit_level(int level)
{
  ModInfo *mod = mod_levels[level].start;
  ModInfo *end = mod_levels[level].end;
  while (mod < end) {
    if (mod->init)
      mod->init();
    mod++;
  }
}

static void do_modfini_level(int level)
{
  ModInfo *mod = mod_levels[level].start;
  ModInfo *end = mod_levels[level].end;
  while (mod < end) {
    if (mod->fini)
      mod->fini();
    mod++;
  }
}

void init_modules(void)
{
  do_modinit_level(0);
  do_modinit_level(1);
  do_modinit_level(2);
  do_modinit_level(3);
}

void fini_modules(void)
{
  do_modfini_level(0);
  do_modfini_level(1);
  do_modfini_level(2);
  do_modfini_level(3);
}
