/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include "bytesobj.h"
#include "excobj.h"
#include "listobj.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

/* A `Path` instance (see `libs/std/fs/path.kl`) carries exactly one field, the
   path string. This C layout mirrors the Koala instance layout (`OBJECT_HEAD`
   followed by the first field), so a native function can read the string
   straight out of a `Path | str` argument with no `.kl` normalization shim.
   The type is registered under the name "Path" so the Koala class binds to it
   (see `_load_module` -> `find_tp_from_native`), which makes
   `IS_TYPE(o, &path_type)` a valid check on a `.kl`-created `Path`. */
typedef struct _PathObject {
    OBJECT_HEAD
    TValue path;
} PathObject;

static TypeObject path_type = {
    ._type = &type_type,
    .name = "Path",
    .flags = TP_FLAGS_CLASS,
    .objsize = sizeof(PathObject),
};

/* Extract the raw C string from a `Path | str` argument: a `str` is used
   directly; a `Path` yields the string held in its single `path` field. */
static char *_arg_path_str(TValue *args, int nargs, int index)
{
    ASSERT(index >= 0 && index < nargs);
    Object *o = to_obj(args + index);
    if (IS_STR(o)) return STR_BUF(o);
    ASSERT(IS_TYPE(o, &path_type));
    PathObject *pobj = (PathObject *)o;
    return STR_BUF(to_obj(&pobj->path));
}

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
    fobj->closed = bool_value(1);
    fobj->fd = int64_value(-1);
    return no_value;
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
        return bool_value(0);
    }

    return bool_value(1);
}

static TypeObject file_type = {
    ._type = &type_type,
    .name = "File",
    .flags = TP_FLAGS_CLASS,
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
    fobj->closed = bool_value(0);
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
    char *path = _arg_path_str(args, nargs, 0);
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
    char *path = _arg_path_str(args, nargs, 0);
    BytesObject *buf = kl_arg_obj_as(1, bytes_type);

    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return bool_value(0);

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
    char *path = _arg_path_str(args, nargs, 0);
    StringObject *sobj = kl_arg_obj_as(1, str_type);

    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return bool_value(0);

    int r = _write_all(fd, STR_BUF(sobj), STR_LEN(sobj));
    close(fd);
    return bool_value(r == 0);
}

/// Permanently delete the file `path` from the disk.
/// Return true on success, false on failure.
static TValue fs_remove(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    char *path = _arg_path_str(args, nargs, 0);
    return bool_value(unlink(path) == 0);
}

/// Rename or move `src` to `dst` within the same filesystem.
/// Return true on success, false on failure (e.g. cross-device move,
/// missing `src`, or a non-empty `dst` directory).
static TValue fs_rename(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    char *src = _arg_path_str(args, nargs, 0);
    char *dst = _arg_path_str(args, nargs, 1);
    return bool_value(rename(src, dst) == 0);
}

/// Return true if the file or directory `path` exists on the disk.
/// Accepts a `Path | str` directly; no `.kl` normalization shim is needed.
static TValue fs_exists(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    char *path = _arg_path_str(args, nargs, 0);

    struct stat sb;
    return bool_value(stat(path, &sb) == 0);
}

/// Return true if `path` exists and points to a regular file.
static TValue fs_is_file(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    char *path = _arg_path_str(args, nargs, 0);

    struct stat sb;
    return bool_value(stat(path, &sb) == 0 && S_ISREG(sb.st_mode));
}

/// Return true if `path` exists and points to a directory.
static TValue fs_is_dir(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    char *path = _arg_path_str(args, nargs, 0);

    struct stat sb;
    return bool_value(stat(path, &sb) == 0 && S_ISDIR(sb.st_mode));
}

/// Return the number of bytes of the file `path`, raise an error on failure.
static TValue fs_size(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    char *path = _arg_path_str(args, nargs, 0);

    struct stat sb;
    if (stat(path, &sb) != 0) {
        raise_exc_fmt("stat '%s' failed: %s", path, strerror(errno));
        return error_value;
    }

    return int64_value(sb.st_size);
}

/* Create `path` and every missing parent directory (mkdir -p).
   An existing final directory is treated as success.
   Return 0 on success, -1 on failure. */
static int _mkdir_all(char *path)
{
    char buf[PATH_MAX];
    size_t len = strlen(path);

    if (len == 0 || len >= sizeof(buf)) return -1;
    memcpy(buf, path, len + 1);

    while (len > 1 && buf[len - 1] == '/') buf[--len] = '\0';

    for (char *p = buf + 1; *p; ++p) {
        if (*p != '/') continue;
        *p = '\0';
        if (mkdir(buf, 0755) != 0 && errno != EEXIST) return -1;
        *p = '/';
    }

    if (mkdir(buf, 0755) != 0 && errno != EEXIST) return -1;

    struct stat sb;
    return (stat(buf, &sb) == 0 && S_ISDIR(sb.st_mode)) ? 0 : -1;
}

/// Create the directory `path`, including any missing parent directories (mkdir -p).
/// An existing directory is treated as success.
/// Return true on success, false on failure.
static TValue fs_mkdir(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    char *path = _arg_path_str(args, nargs, 0);

    return bool_value(_mkdir_all(path) == 0);
}

/// Remove the empty directory `path`.
/// Return true on success, false if it is not empty or cannot be removed.
static TValue fs_rmdir(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    char *path = _arg_path_str(args, nargs, 0);
    return bool_value(rmdir(path) == 0);
}

static int _name_cmp(const void *a, const void *b) { return strcmp(*(char **)a, *(char **)b); }

/// Read the directory `path` and return the entry names as a sorted list
/// of `str`, the special entries `.` and `..` are skipped.
/// Raise an error if the directory cannot be opened.
static TValue fs_list_dir(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    char *path = kl_arg_str(0);

    DIR *dir = opendir(path);
    if (dir == NULL) {
        raise_exc_fmt("open directory '%s' failed: %s", path, strerror(errno));
        return error_value;
    }

    Vector names = VECTOR_INIT_PTR;
    struct dirent *dent;
    while ((dent = readdir(dir))) {
        if (strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0) continue;
        char *name = str_dup(dent->d_name);
        vector_push_back(&names, &name);
    }
    closedir(dir);

    vector_sort(&names, _name_cmp);

    Object *list = kl_new_list();
    char *name;
    vector_foreach(name, &names) {
        kl_list_append(list, kl_val_str(name));
        mm_free(name);
    }
    vector_fini(&names);

    return obj_value(list);
}

void fs_native_lib_init(NativeLib *lib)
{
    kl_reg_func(lib, "open", file_open);

    /* Public natives read a `Path | str` argument directly (see
       `_arg_path_str`), so no `.kl` `_path_str` shim is needed. */
    kl_reg_func(lib, "read_bytes", fs_read_bytes);
    kl_reg_func(lib, "write_bytes", fs_write_bytes);
    kl_reg_func(lib, "write_str", fs_write_str);
    kl_reg_func(lib, "exists", fs_exists);
    kl_reg_func(lib, "is_file", fs_is_file);
    kl_reg_func(lib, "is_dir", fs_is_dir);
    kl_reg_func(lib, "size", fs_size);
    kl_reg_func(lib, "remove", fs_remove);
    kl_reg_func(lib, "rename", fs_rename);
    kl_reg_func(lib, "mkdir", fs_mkdir);
    kl_reg_func(lib, "rmdir", fs_rmdir);

    /* `list_dir` stays a `.kl` wrapper over the raw `_list_dir` primitive,
       which still takes a plain `str` and returns `list[str]`. */
    kl_reg_func(lib, "_list_dir", fs_list_dir);

    kl_reg_type(lib, &file_type);
    /* Bind the Koala `Path` class to the C `path_type` so natives can read a
       `Path | str` argument directly. No methdefs: `Path` is pure Koala. */
    kl_reg_type(lib, &path_type);
}

#ifdef __cplusplus
}
#endif
