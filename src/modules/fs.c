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
    TValue fd;
    TValue is_tty;
} FileObject;

static TypeObject file_type;

static TValue file_read(TValue *self, TValue *args, int nargs)
{
    Object *_self = to_obj(self);
    FileObject *fobj = (FileObject *)_self;
    int fd = to_int64(&fobj->fd);

    if (to_bool(&fobj->closed) == 1) {
        raise_exc_fmt("file descriptor %d is closed", fobj->fd);
        return error_value;
    } else if (fcntl(fd, F_GETFD) == -1) {
        raise_exc_fmt("file descriptor %d is closed", fd);
        return error_value;
    }

    Object *_buf = to_obj(&args[0]);
    BytesObject *buf = (BytesObject *)_buf;
    int r = read(fd, buf->data, buf->size);
    return int64_value(r);
}

static TValue file_write(TValue *self, TValue *args, int nargs)
{
    FileObject *fobj = SELF_AS(file_type);
    int fd = to_int64(&fobj->fd);
    ASSERT(nargs == 1);
    BytesObject *buf = kl_arg_obj_as(0, bytes_type);

    if (to_bool(&fobj->closed) == 1) {
        raise_exc_fmt("file descriptor %d is closed", fobj->fd);
        return error_value;
    } else if (fcntl(fd, F_GETFD) == -1) {
        raise_exc_fmt("file descriptor %d is closed", fd);
        return error_value;
    }

    int r = write(fd, buf->data + buf->offset, buf->size);
    return int64_value(r);
}

static TValue file_write_str(TValue *self, TValue *args, int nargs)
{
    FileObject *fobj = SELF_AS(file_type);
    int fd = to_int64(&fobj->fd);
    ASSERT(nargs == 1);
    StringObject *sobj = kl_arg_obj_as(0, str_type);

    if (to_bool(&fobj->closed) == 1) {
        raise_exc_fmt("file descriptor %d is closed", fobj->fd);
        return error_value;
    } else if (fcntl(fd, F_GETFD) == -1) {
        raise_exc_fmt("file descriptor %d is closed", fd);
        return error_value;
    }

    int r = write(fd, STR_BUF(sobj), STR_LEN(sobj));
    return int64_value(r);
}

static TValue file_close(TValue *self, TValue *args, int nargs)
{
    FileObject *fobj = SELF_AS(file_type);
    int fd = to_int64(&fobj->fd);
    ASSERT(nargs == 0);
    close(fd);
    fobj->closed = BOOL_TRUE;
    fobj->fd = int64_value(-1);
    return nil_value;
}

/// Move the file offset to `offset` bytes.
/// `whence` is optional (default `0`) and selects the reference point:
/// - `0`: set the offset relative to the start of the file.
/// - `1`: set the offset relative to the current position.
/// - `2`: set the offset relative to the end of the file.
/// Return the new absolute offset, raise an error on failure.
static TValue file_seek(TValue *self, TValue *args, int nargs)
{
    FileObject *fobj = SELF_AS(file_type);
    int fd = to_int64(&fobj->fd);
    ASSERT(nargs == 1 || nargs == 2);

    if (to_bool(&fobj->closed) == 1) {
        raise_exc_fmt("file descriptor %d is closed", fd);
        return error_value;
    } else if (fcntl(fd, F_GETFD) == -1) {
        raise_exc_fmt("file descriptor %d is closed", fd);
        return error_value;
    }

    int64_t offset = kl_arg_int64(0);
    int64_t whence = kl_arg_int64(1);

    int w = 0;
    if (whence == 0) {
        w = SEEK_SET;
    } else if (whence == 1) {
        w = SEEK_CUR;
    } else if (whence == 2) {
        w = SEEK_END;
    } else {
        raise_exc_fmt("invalid whence %" PRId64, whence);
        return error_value;
    }

    off_t pos = lseek(fd, (off_t)offset, w);
    if (pos == (off_t)-1) {
        raise_exc_fmt("seek failed: %s", strerror(errno));
        return error_value;
    }

    return int64_value(pos);
}

/// Return the current file offset in bytes, raise an error on failure.
static TValue file_tell(TValue *self, TValue *args, int nargs)
{
    FileObject *fobj = SELF_AS(file_type);
    int fd = to_int64(&fobj->fd);
    ASSERT(nargs == 0);

    if (to_bool(&fobj->closed) == 1) {
        raise_exc_fmt("file descriptor %d is closed", fd);
        return error_value;
    } else if (fcntl(fd, F_GETFD) == -1) {
        raise_exc_fmt("file descriptor %d is closed", fd);
        return error_value;
    }

    off_t pos = lseek(fd, 0, SEEK_CUR);
    if (pos == (off_t)-1) {
        raise_exc_fmt("tell failed: %s", strerror(errno));
        return error_value;
    }

    return int64_value(pos);
}

/// Truncate the file to `size` bytes. The current file offset is not changed.
/// Return `nil`, raise an error on failure.
static TValue file_truncate(TValue *self, TValue *args, int nargs)
{
    FileObject *fobj = SELF_AS(file_type);
    int fd = to_int64(&fobj->fd);
    ASSERT(nargs == 0 || nargs == 1);

    if (to_bool(&fobj->closed) == 1) {
        raise_exc_fmt("file descriptor %d is closed", fd);
        return error_value;
    } else if (fcntl(fd, F_GETFD) == -1) {
        raise_exc_fmt("file descriptor %d is closed", fd);
        return error_value;
    }

    int64_t size = kl_arg_int64(0);

    if (ftruncate(fd, (off_t)size) == -1) {
        raise_exc_fmt("truncate failed: %s", strerror(errno));
        return error_value;
    }

    return nil_value;
}

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
            { "tell", file_tell },
            { "truncate", file_truncate },
            { NULL, NULL },
        },
};

static Object *kl_new_file(TValue path, TValue mode, int fd)
{
    FileObject *fobj = mm_alloc_obj(fobj);
    INIT_OBJECT_HEAD(fobj, &file_type, 5);
    fobj->path = path;
    fobj->mode = mode;
    fobj->closed = BOOL_FALSE;
    fobj->fd = int64_value(fd);
    fobj->is_tty = bool_value(isatty(fd));
    return (Object *)fobj;
}

/// `mode` selects the access:
/// - `"r"`: read-only, the file must exist.
/// - `"w"`: write-only, create or truncate.
/// - `"x"`: write-only, exclusive create, fail if the file exists.
/// - `"a"`: write-only, create if absent, always append at the end.
/// - `"r+"`: read and write, the file must exist.
/// - `"w+"`: read and write, create or truncate.
/// - `"x+"`: read and write, exclusive create, fail if the file exists.
/// - `"a+"`: read and write, create if absent, always append at the end.
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
    } else if (strcmp(mode, "x") == 0) {
        flags = O_WRONLY | O_CREAT | O_EXCL;
    } else if (strcmp(mode, "a") == 0) {
        flags = O_WRONLY | O_CREAT | O_APPEND;
    } else if (strcmp(mode, "r+") == 0) {
        flags = O_RDWR;
    } else if (strcmp(mode, "w+") == 0) {
        flags = O_RDWR | O_CREAT | O_TRUNC;
    } else if (strcmp(mode, "x+") == 0) {
        flags = O_RDWR | O_CREAT | O_EXCL;
    } else if (strcmp(mode, "a+") == 0) {
        flags = O_RDWR | O_CREAT | O_APPEND;
    } else {
        raise_exc_fmt("invalid file mode '%s'", mode);
        return error_value;
    }

    int fd = open(path, flags, 0644);
    if (fd < 0) return nil_value;
    Object *fobj = kl_new_file(args[0], args[1], fd);
    return obj_value(fobj);
}

/* Write all `len` bytes to `fd`, retry on partial writes and EINTR.
   Return 0 on success, -1 on failure. */
static int _write_all(int fd, const void *data, size_t len)
{
    const uint8_t *p = (const uint8_t *)data;
    while (len > 0) {
        ssize_t w = write(fd, p, len);
        if (w < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        p += w;
        len -= (size_t)w;
    }
    return 0;
}

/// Read the file `path` into `out` and return the count read.
/// Reading stops when `out` is full or end of file is reached.
/// Return 0 for an empty file, or -1 if the file cannot be opened or read.
static TValue fs_read_bytes(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    char *path = kl_arg_str(0);
    BytesObject *out = kl_arg_obj_as(1, bytes_type);

    int fd = open(path, O_RDONLY);
    if (fd < 0) return int64_value(-1);

    int64_t total = 0;
    while ((uint32_t)total < out->size) {
        ssize_t r = read(fd, out->data + total, out->size - (uint32_t)total);
        if (r < 0) {
            if (errno == EINTR) continue;
            close(fd);
            return int64_value(-1);
        }
        if (r == 0) break; /* EOF */
        total += r;
    }

    close(fd);
    return int64_value(total);
}

/// Write `data` to the file `path`, creating or truncating it.
/// All bytes are written or the operation fails.
/// Return true on success, false if the file cannot be opened or written.
static TValue fs_write_bytes(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    char *path = kl_arg_str(0);
    BytesObject *buf = kl_arg_obj_as(1, bytes_type);

    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return BOOL_FALSE;

    int r = _write_all(fd, buf->data + buf->offset, buf->size);
    close(fd);
    return bool_value(r == 0);
}

/// Encode `data` as UTF-8 and write it to the file `path`,
/// creating or truncating it.
/// All bytes are written or the operation fails.
/// Return true on success, false if the file cannot be opened or written.
static TValue fs_write_str(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    char *path = kl_arg_str(0);
    StringObject *sobj = kl_arg_obj_as(1, str_type);

    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return BOOL_FALSE;

    int r = _write_all(fd, STR_BUF(sobj), STR_LEN(sobj));
    close(fd);
    return bool_value(r == 0);
}

void fs_native_lib_init(NativeLib *lib)
{
    kl_reg_func(lib, "open", file_open);
    kl_reg_func(lib, "read_bytes", fs_read_bytes);
    kl_reg_func(lib, "write_bytes", fs_write_bytes);
    kl_reg_func(lib, "write_str", fs_write_str);
    kl_reg_type(lib, &file_type);
}

#ifdef __cplusplus
}
#endif
