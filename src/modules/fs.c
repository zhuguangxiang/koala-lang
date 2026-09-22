/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include <fcntl.h>
#include <unistd.h>
#include "bytesobj.h"
#include "excobj.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _FileObject {
    OBJECT_HEAD
    TValue path;
    TValue mode;
    TValue closed;
    int fd;
} FileObject;

static TypeObject file_type;

static Object *kl_new_file(TValue path, TValue mode, int fd)
{
    FileObject *fobj = mm_alloc_obj(fobj);
    INIT_OBJECT_HEAD(fobj, &file_type, 3);
    fobj->path = path;
    fobj->mode = mode;
    fobj->closed = BOOL_FALSE;
    fobj->fd = fd;
    return (Object *)fobj;
}

static TValue file_open(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);

    if (is_int64(&args[0])) {
        int fd = (int)kl_arg_int64(0);

        if (fcntl(fd, F_GETFD) == -1) {
            return nil_value;
        }

        Object *fobj = kl_new_file(nil_value, args[1], fd);
        return obj_value(fobj);
    }

    char *path = kl_arg_str(0);
    char *mode = kl_arg_str(1);

    int flags = 0;

    if (strcmp(mode, "r") == 0) {
        flags = O_RDONLY;
    } else if (strcmp(mode, "w") == 0) {
        flags = O_WRONLY | O_CREAT | O_TRUNC;
    } else if (strcmp(mode, "a") == 0) {
        flags = O_WRONLY | O_CREAT | O_APPEND;
    } else {
        flags = O_RDONLY;
    }

    int fd = open(path, flags, 0644);
    if (fd < 0) return nil_value;
    Object *fobj = kl_new_file(args[0], args[1], fd);
    return obj_value(fobj);
}

static TValue file_read(TValue *self, TValue *args, int nargs)
{
    Object *_self = to_obj(self);
    FileObject *fobj = (FileObject *)_self;

    if (to_bool(&fobj->closed) == 1) {
        raise_exc_fmt("file descriptor %d is closed", fobj->fd);
        return error_value;
    } else if (fcntl(fobj->fd, F_GETFD) == -1) {
        raise_exc_fmt("file descriptor %d is closed", fobj->fd);
        return error_value;
    }

    Object *_buf = to_obj(&args[0]);
    BytesObject *buf = (BytesObject *)_buf;
    int r = read(fobj->fd, buf->data, buf->size);
    return int64_value(r);
}

static TValue file_write(TValue *self, TValue *args, int nargs)
{
    FileObject *fobj = SELF_AS(file_type);
    ASSERT(nargs == 1);
    BytesObject *buf = kl_arg_obj_as(0, bytes_type);

    if (to_bool(&fobj->closed) == 1) {
        raise_exc_fmt("file descriptor %d is closed", fobj->fd);
        return error_value;
    } else if (fcntl(fobj->fd, F_GETFD) == -1) {
        raise_exc_fmt("file descriptor %d is closed", fobj->fd);
        return error_value;
    }

    int r = write(fobj->fd, buf->data + buf->offset, buf->size);
    return int64_value(r);
}

static TValue file_write_str(TValue *self, TValue *args, int nargs)
{
    FileObject *fobj = SELF_AS(file_type);
    ASSERT(nargs == 1);
    StringObject *sobj = kl_arg_obj_as(0, str_type);

    if (to_bool(&fobj->closed) == 1) {
        raise_exc_fmt("file descriptor %d is closed", fobj->fd);
        return error_value;
    } else if (fcntl(fobj->fd, F_GETFD) == -1) {
        raise_exc_fmt("file descriptor %d is closed", fobj->fd);
        return error_value;
    }

    int r = write(fobj->fd, STR_BUF(sobj), STR_LEN(sobj));
    return int64_value(r);
}

static TValue file_close(TValue *self, TValue *args, int nargs)
{
    FileObject *fobj = SELF_AS(file_type);
    ASSERT(nargs == 0);
    close(fobj->fd);
    fobj->closed = BOOL_TRUE;
    fobj->fd = -1;
    return nil_value;
}

static TValue file_seek(TValue *self, TValue *args, int nargs) { return nil_value; }

static TypeObject file_type = {
    ._type = &type_type,
    .name = "File",
    .flags = TP_FLAGS_CLASS,
    .priv_size = sizeof(int),
    .methdefs =
        (MethodDef[]){
            { "read", file_read },
            { "write", file_write },
            { "write_str", file_write_str },
            { "close", file_close },
            { "seek", file_seek },
            { NULL, NULL },
        },
};

void fs_native_lib_init(NativeLib *lib)
{
    kl_reg_func(lib, "open", file_open);
    kl_reg_type(lib, &file_type);
}

#ifdef __cplusplus
}
#endif
