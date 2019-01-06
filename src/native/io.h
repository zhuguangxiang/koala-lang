/*
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _KOALA_NATIVE_IO_H_
#define _KOALA_NATIVE_IO_H_

#include "object.h"

typedef struct fdobject {
  OBJECT_HEAD
  int fd;
} FdObject;

extern Klass Fd_Klass;
FdObject *Fd_New(int fd);
FdObject *Fd_Open(char *path, int flag);
int Fd_Write(FdObject *fd, char *data, int size);
int Fd_Read(FdObject *fd, char *buf, int size);
int Fd_Close(FdObject *fd);

void Init_Nio_Package(void);
void Fini_Nio_Package(void);

#endif /* _KOALA_NATIVE_IO_H_ */
