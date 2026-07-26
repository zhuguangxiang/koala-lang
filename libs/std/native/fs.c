/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include <fcntl.h>
#include <unistd.h>
#include "bytesobj.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _FileObject {
    INST_OBJECT_HEAD
    TValue path;
    TValue mode;
    int fd;
} FileObject;

static TypeObject file_type;

static Object *kl_new_file(char *path, int fd)
{
    FileObject *fobj = mm_alloc_obj(fobj);
    INIT_OBJECT_HEAD(fobj, &file_type);
    fobj->size = 2;
    fobj->path = obj_value(kl_new_str(path));
    fobj->mode = none_value;
    fobj->fd = fd;
    return (Object *)fobj;
}

static TValue file_open(TValue *self, TValue *args, int nargs)
{
    Object *_path = to_obj(&args[0]);
    char *path = STR_BUF(_path);
    int fd = open(path, O_RDONLY);
    if (fd < 0) return none_value;
    Object *fobj = kl_new_file(path, fd);
    return obj_value(fobj);
}

static TValue file_read(TValue *self, TValue *args, int nargs)
{
    Object *_self = to_obj(self);
    FileObject *fobj = (FileObject *)_self;
    Object *_buf = to_obj(&args[0]);
    BytesObject *buf = (BytesObject *)_buf;
    int r = read(fobj->fd, buf->data, buf->size);
    return int64_value(r);
}

static TValue file_write(TValue *self, TValue *args, int nargs) { return none_value; }

static TValue file_close(TValue *self, TValue *args, int nargs) { return none_value; }

static TValue file_seek(TValue *self, TValue *args, int nargs) { return none_value; }

static Object *file_alloc(TypeObject *tp)
{
    FileObject *fobj = mm_alloc_obj(fobj);
    INIT_OBJECT_HEAD(fobj, tp);
    return (Object *)fobj;
}

static TypeObject file_type = {
    ._type = &type_type,
    .name = "File",
    .flags = TP_FLAGS_CLASS | TP_FLAGS_PUBLIC,
    .alloc = file_alloc,
};

void fs_native_lib_init(NativeLib *lib)
{
    kl_reg_func(lib, "open", file_open);
    kl_reg_type(lib, &file_type);
    kl_reg_meth(lib, "File", "read", file_read);
    kl_reg_meth(lib, "File", "write", file_write);
    kl_reg_meth(lib, "File", "close", file_close);
    kl_reg_meth(lib, "File", "seek", file_seek);
}

#ifdef __cplusplus
}
#endif
