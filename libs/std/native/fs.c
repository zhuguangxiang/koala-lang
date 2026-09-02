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
    OBJECT_HEAD
    TValue path;
    TValue mode;
    int fd;
} FileObject;

static TypeObject file_type;

static Object *kl_new_file(TValue path, TValue mode, int fd)
{
    FileObject *fobj = mm_alloc_obj(fobj);
    INIT_OBJECT_HEAD(fobj, &file_type, 2);
    fobj->path = path;
    fobj->mode = mode;
    fobj->fd = fd;
    return (Object *)fobj;
}

static TValue file_open(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);

    if (is_int64(&args[0])) {
        int fd = (int)kl_arg_int64(0);

        if (fcntl(fd, F_GETFD) == -1) {
            return none_value;
        }

        Object *fobj = kl_new_file(none_value, args[1], fd);
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
    if (fd < 0) return none_value;
    Object *fobj = kl_new_file(args[0], args[1], fd);
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

static TValue file_write(TValue *self, TValue *args, int nargs)
{
    FileObject *fobj = SELF_AS(file_type);
    ASSERT(nargs == 1);
    BytesObject *buf = kl_arg_obj_as(0, bytes_type);
    if (fcntl(fobj->fd, F_GETFD) == -1) {
        // printf("file descriptor %d is closed\n", fobj->fd);
        return none_value;
    }

    int r = write(fobj->fd, buf->data + buf->offset, buf->size);
    return int64_value(r);
}

static TValue file_close(TValue *self, TValue *args, int nargs)
{
    FileObject *fobj = SELF_AS(file_type);
    ASSERT(nargs == 0);
    close(fobj->fd);
    return none_value;
}

static TValue file_seek(TValue *self, TValue *args, int nargs) { return none_value; }

static TypeObject file_type = {
    ._type = &type_type,
    .name = "File",
    .flags = TP_FLAGS_CLASS,
    .priv_size = sizeof(int),
    .methdefs =
        (MethodDef[]){
            { "read", file_read },
            { "write", file_write },
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
