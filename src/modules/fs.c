/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include <dirent.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <glob.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include "bytesobj.h"
#include "excobj.h"
#include "listobj.h"
#include "tupleobj.h"
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
    ._nfields = 1,
    .name = "Path",
    .flags = TP_FLAGS_CLASS,
    .objsize = sizeof(PathObject),
};

/* Create a `Path` holding the string `path` */
static Object *_new_path(TValue path)
{
    Object *obj = kl_new_instance(&path_type);
    ((PathObject *)obj)->path = path;
    return obj;
}

/* A `Stat` instance (see `libs/std/fs/stat.kl`): the C layout mirrors its
   fields in declaration order, and the type is registered under the
   name "Stat" so the Koala class binds to it (as `Path` does), which lets
   `stat` build one directly. The methods are pure Koala. */
typedef struct _StatObject {
    OBJECT_HEAD
    TValue size;
    TValue mtime;
    TValue mtime_ns;
    TValue mode;
    TValue atime_ns;
    TValue ctime_ns;
    TValue uid;
    TValue gid;
    TValue nlink;
    TValue ino;
    TValue dev;
} StatObject;

static TypeObject stat_type = {
    ._type = &type_type,
    ._nfields = 11,
    .name = "Stat",
    .flags = TP_FLAGS_CLASS,
    .objsize = sizeof(StatObject),
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

/// Flush the file's data and metadata to the storage device (fsync(2)).
/// Return true on success, false on failure; raise an error if it is closed.
static TValue file_sync(TValue *self, TValue *args, int nargs)
{
    FileObject *fobj = SELF_AS(file_type);
    int fd = to_int64(&fobj->fd);
    ASSERT(nargs == 0);

    if (to_bool(&fobj->closed) == 1) {
        raise_exc_fmt("file descriptor %d is closed", fd);
        return error_value;
    }

    int r;
    do {
        r = fsync(fd);
    } while (r < 0 && errno == EINTR);

    return bool_value(r == 0);
}

static TypeObject file_type = {
    ._type = &type_type,
    ._nfields = 5,
    .name = "File",
    .flags = TP_FLAGS_CLASS,
    .objsize = sizeof(FileObject),
    .methdefs =
        (MethodDef[]){
            { "read", file_read },
            { "write", file_write },
            { "write_str", file_write_str },
            { "close", file_close },
            { "seek", file_seek },
            { "tell", file_tell },
            { "truncate", file_truncate },
            { "sync", file_sync },
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

/// Return true if `path` itself is a symbolic link (checked with lstat, so
/// the link is not followed; a dangling link is still a link).
static TValue fs_is_symlink(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    char *path = _arg_path_str(args, nargs, 0);

    struct stat sb;
    return bool_value(lstat(path, &sb) == 0 && S_ISLNK(sb.st_mode));
}

/*
 * pub func stat(p Path | str, follow_links = true) Stat?
 *
 * Return the metadata of `p`, using stat when `follow_links` is true and
 * lstat otherwise. Return nil on failure; os.errno() tells why.
 */
static TValue fs_stat(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    char *path = _arg_path_str(args, nargs, 0);
    int follow = kl_arg_bool(1);

    struct stat sb;
    int r = follow ? stat(path, &sb) : lstat(path, &sb);
    if (r != 0) return nil_value;

#ifdef __APPLE__
    struct timespec mt = sb.st_mtimespec;
    struct timespec at = sb.st_atimespec;
    struct timespec ct = sb.st_ctimespec;
#else
    struct timespec mt = sb.st_mtim;
    struct timespec at = sb.st_atim;
    struct timespec ct = sb.st_ctim;
#endif

    Object *obj = kl_new_instance(&stat_type);
    StatObject *st = (StatObject *)obj;
    st->atime_ns = int64_value((int64_t)at.tv_sec * 1000000000 + at.tv_nsec);
    st->ctime_ns = int64_value((int64_t)ct.tv_sec * 1000000000 + ct.tv_nsec);
    st->uid = int64_value(sb.st_uid);
    st->gid = int64_value(sb.st_gid);
    st->nlink = int64_value(sb.st_nlink);
    st->ino = int64_value(sb.st_ino);
    st->dev = int64_value(sb.st_dev);
    st->size = int64_value(sb.st_size);
    st->mtime = float64_value((double)mt.tv_sec + (double)mt.tv_nsec / 1e9);
    st->mtime_ns = int64_value((int64_t)mt.tv_sec * 1000000000 + mt.tv_nsec);
    st->mode = int64_value(sb.st_mode);
    return obj_value(obj);
}

/*
 * pub func real_path(p Path | str) Path?
 *
 * Return the absolute path of `p` with every symbolic link, `.` and `..`
 * resolved (realpath(3)), or nil if it does not exist or cannot be resolved.
 */
static TValue fs_real_path(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    char *path = _arg_path_str(args, nargs, 0);

    char buf[PATH_MAX];
    if (realpath(path, buf) == NULL) return nil_value;
    return obj_value(_new_path(kl_val_str(buf)));
}

/// Set the permission bits of `path` to `mode` (chmod(2)); a symbolic link is
/// followed. Return true on success, false on failure.
static TValue fs_chmod(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    char *path = _arg_path_str(args, nargs, 0);
    mode_t mode = (mode_t)kl_arg_int64(1);
    return bool_value(chmod(path, mode & 07777) == 0);
}

/// Set the owner and group of `path` (chown(2)); -1 keeps that one. A symbolic
/// link is followed. Return true on success.
static TValue fs_chown(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 3);
    char *path = _arg_path_str(args, nargs, 0);
    uid_t uid = (uid_t)kl_arg_int64(1);
    gid_t gid = (gid_t)kl_arg_int64(2);
    return bool_value(chown(path, uid, gid) == 0);
}

/// Return true if this process may access `path` as `mode` asks (access(2)):
/// F_OK (exists), or an OR of R_OK, W_OK and X_OK.
static TValue fs_access(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    char *path = _arg_path_str(args, nargs, 0);
    int mode = (int)kl_arg_int64(1);
    return bool_value(access(path, mode) == 0);
}

/// Create the symbolic link `link` pointing to `target` (symlink(2)).
/// `target` is stored as given, it need not exist. Return true on success.
static TValue fs_symlink(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    char *target = _arg_path_str(args, nargs, 0);
    char *link = _arg_path_str(args, nargs, 1);
    return bool_value(symlink(target, link) == 0);
}

/*
 * pub func read_link(p Path | str) Path?
 *
 * Return the target stored in the symbolic link `p` (readlink(2)), as given
 * when the link was made: it may be relative, or not exist. Return nil if `p`
 * is not a symbolic link or cannot be read.
 */
static TValue fs_read_link(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    char *path = _arg_path_str(args, nargs, 0);

    char buf[PATH_MAX];
    ssize_t n = readlink(path, buf, sizeof(buf));
    if (n < 0 || n >= (ssize_t)sizeof(buf)) return nil_value;
    return obj_value(_new_path(kl_val_nstr(buf, n)));
}

/*
 * func _glob(pattern Path | str) list[Path]
 *
 * Return the paths matching the shell `pattern` (glob(3)), sorted. Return an
 * empty list if nothing matches. glob() handles a `**` pattern itself.
 */
static TValue fs_glob(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    char *pattern = _arg_path_str(args, nargs, 0);

    Object *list = kl_new_list();
    glob_t g;
    if (glob(pattern, 0, NULL, &g) == 0) {
        for (size_t i = 0; i < g.gl_pathc; i++) {
            kl_list_append(list, obj_value(_new_path(kl_val_str(g.gl_pathv[i]))));
        }
    }
    globfree(&g);
    return obj_value(list);
}

/*
 * func _fnmatch(pattern str, name str) bool
 *
 * Match `name` against the shell `pattern` (fnmatch(3)); `*` and `?` do not
 * match a '/', and a leading '.' must be matched explicitly.
 */
static TValue fs_fnmatch(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    char *pattern = kl_arg_str(0);
    char *name = kl_arg_str(1);
    return bool_value(fnmatch(pattern, name, FNM_PATHNAME | FNM_PERIOD) == 0);
}

/* The temporary directory: $TMPDIR when set and not empty, else /tmp */
static char *_temp_dir(void)
{
    char *dir = getenv("TMPDIR");
    return dir && dir[0] ? dir : "/tmp";
}

/*
 * pub func temp_dir() Path
 *
 * Return the directory for temporary files: $TMPDIR, or /tmp.
 */
static TValue fs_temp_dir(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    return obj_value(_new_path(kl_val_str(_temp_dir())));
}

/* Build "<dir>/<prefix>XXXXXX<suffix>" into `buf` for mkstemps / mkdtemp.
   Return -1 (errno EINVAL) if prefix or suffix has a '/', or it is too long. */
static int _temp_template(char *buf, size_t size, TValue *args, int nargs, int dir_index,
                          char *prefix, char *suffix)
{
    if (strchr(prefix, '/') || strchr(suffix, '/')) {
        errno = EINVAL;
        return -1;
    }
    char *dir = _arg_path_str(args, nargs, dir_index);
    if (!dir[0]) dir = _temp_dir();
    int n = snprintf(buf, size, "%s%s%sXXXXXX%s", dir, dir[strlen(dir) - 1] == '/' ? "" : "/",
                     prefix, suffix);
    if (n < 0 || (size_t)n >= size) {
        errno = ENAMETOOLONG;
        return -1;
    }
    return 0;
}

/*
 * pub func make_temp_file(prefix = "tmp", suffix = "", dir Path | str = "") Path?
 *
 * Create a new empty file `<dir>/<prefix>XXXXXX<suffix>` with a unique,
 * unpredictable name, readable and writable only by this user (0600), and
 * return its path. `dir` "" is temp_dir(). Return nil on failure.
 */
static TValue fs_make_temp_file(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 3);
    char *prefix = kl_arg_str(0);
    char *suffix = kl_arg_str(1);

    char buf[PATH_MAX];
    if (_temp_template(buf, sizeof(buf), args, nargs, 2, prefix, suffix) < 0) return nil_value;

    int fd = mkstemps(buf, (int)strlen(suffix));
    if (fd < 0) return nil_value;
    close(fd);
    return obj_value(_new_path(kl_val_str(buf)));
}

/*
 * pub func make_temp_dir(prefix = "tmp", dir Path | str = "") Path?
 *
 * Create a new empty directory `<dir>/<prefix>XXXXXX` with a unique,
 * unpredictable name, usable only by this user (0700), and return its path.
 * `dir` "" is temp_dir(). Return nil on failure.
 */
static TValue fs_make_temp_dir(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    char *prefix = kl_arg_str(0);

    char buf[PATH_MAX];
    if (_temp_template(buf, sizeof(buf), args, nargs, 1, prefix, "") < 0) return nil_value;
    if (mkdtemp(buf) == NULL) return nil_value;
    return obj_value(_new_path(kl_val_str(buf)));
}

/// Set the access and modification times of `path` to now, creating it
/// (empty, 0644 before the umask) if it does not exist, like `touch`.
/// A symbolic link is followed. Return true on success.
static TValue fs_touch(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    char *path = _arg_path_str(args, nargs, 0);

    int fd = open(path, O_WRONLY | O_CREAT | O_CLOEXEC, 0644);
    if (fd < 0) return bool_value(0);
    int r = futimens(fd, NULL);
    close(fd);
    return bool_value(r == 0);
}

/* `ns` nanoseconds since the Unix epoch as a timespec */
static struct timespec _ns_to_timespec(int64_t ns)
{
    struct timespec ts;
    ts.tv_sec = ns / 1000000000;
    ts.tv_nsec = ns % 1000000000;
    if (ts.tv_nsec < 0) {
        ts.tv_sec -= 1;
        ts.tv_nsec += 1000000000;
    }
    return ts;
}

/// Set the access and modification times of `path`, in nanoseconds since
/// the Unix epoch (utimensat(2)); a symbolic link is followed. Return true
/// on success.
static TValue fs_set_times(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 3);
    char *path = _arg_path_str(args, nargs, 0);
    struct timespec times[2] = {
        _ns_to_timespec(kl_arg_int64(1)),
        _ns_to_timespec(kl_arg_int64(2)),
    };
    return bool_value(utimensat(AT_FDCWD, path, times, 0) == 0);
}

/// Return true if `a` and `b` are the same file (the same device and inode,
/// symbolic links followed); false if either does not exist.
static TValue fs_samefile(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    char *a = _arg_path_str(args, nargs, 0);
    char *b = _arg_path_str(args, nargs, 1);

    struct stat sa, sb;
    if (stat(a, &sa) != 0 || stat(b, &sb) != 0) return bool_value(0);
    return bool_value(sa.st_dev == sb.st_dev && sa.st_ino == sb.st_ino);
}

/// Create the hard link `dst` to the existing file `src` (link(2)): both
/// names then refer to the same file. Return true on success.
static TValue fs_hard_link(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    char *src = _arg_path_str(args, nargs, 0);
    char *dst = _arg_path_str(args, nargs, 1);
    return bool_value(link(src, dst) == 0);
}

/*
 * pub func disk_usage(p Path | str) (int, int, int)?
 *
 * Return (total, used, free) bytes of the file system holding `p`
 * (statvfs(3)), the same numbers as `df`: `free` is what an unprivileged
 * user may still use. Return nil if `p` cannot be read.
 */
static TValue fs_disk_usage(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    char *path = _arg_path_str(args, nargs, 0);

    struct statvfs vs;
    if (statvfs(path, &vs) != 0) return nil_value;
    int64_t unit = (int64_t)vs.f_frsize;
    TValue items[3] = {
        int64_value((int64_t)vs.f_blocks * unit),
        int64_value((int64_t)(vs.f_blocks - vs.f_bfree) * unit),
        int64_value((int64_t)vs.f_bavail * unit),
    };
    return obj_value(kl_new_tuple(items, 3));
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

/* Read the entries of the directory `dir` into a new list of `Path`, each
   joined with `dir` ("dir/name"; no extra '/' if `dir` ends with one),
   sorted by name, without the special entries `.` and `..`.
   Return NULL (errno set) if the directory cannot be opened. */
static Object *_read_dir_paths(char *dir)
{
    DIR *d = opendir(dir);
    if (d == NULL) return NULL;

    Vector names = VECTOR_INIT_PTR;
    struct dirent *dent;
    while ((dent = readdir(d))) {
        if (strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0) continue;
        char *name = str_dup(dent->d_name);
        vector_push_back(&names, &name);
    }
    closedir(d);

    vector_sort(&names, _name_cmp);

    size_t dlen = strlen(dir);
    int need_sep = dlen > 0 && dir[dlen - 1] != '/';

    Object *list = kl_new_list();
    char *name;
    vector_foreach(name, &names) {
        BUF(buf);
        buf_write_nstr(&buf, dir, dlen);
        if (need_sep) buf_write_char(&buf, '/');
        buf_write_str(&buf, name);
        kl_list_append(list, obj_value(_new_path(kl_val_nstr(BUF_STR(buf), BUF_LEN(buf)))));
        FINI_BUF(buf);
        mm_free(name);
    }
    vector_fini(&names);

    return list;
}

/*
 * pub func list_dir(p Path | str) list[Path]
 *
 * Read the directory `p` and return its entries as sorted `Path`s joined
 * with `p`. Raise an error if the directory cannot be opened.
 */
static TValue fs_list_dir(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    char *path = _arg_path_str(args, nargs, 0);

    Object *list = _read_dir_paths(path);
    if (list == NULL) {
        raise_exc_fmt("open directory '%s' failed: %s", path, strerror(errno));
        return error_value;
    }
    return obj_value(list);
}

/*
 * func _try_list_dir(p Path | str) list[Path]?
 *
 * Same as `list_dir`, but return nil instead of raising an error if the
 * directory cannot be opened (used by `walk` and `remove_all`).
 */
static TValue fs_try_list_dir(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    char *path = _arg_path_str(args, nargs, 0);

    Object *list = _read_dir_paths(path);
    if (list == NULL) return nil_value;
    return obj_value(list);
}

/* Write all `n` bytes of `buf` to `fd`. Return 0 on success, -1 on error. */
static int _write_full(int fd, char *buf, ssize_t n)
{
    while (n > 0) {
        ssize_t w = write(fd, buf, n);
        if (w < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        buf += w;
        n -= w;
    }
    return 0;
}

/// Copy the content and the permission bits of the file `src` to `dst`,
/// creating or truncating `dst`. A symbolic link `src` is followed.
/// Return true on success, false on failure (missing or unreadable `src`,
/// `src` is a directory, `dst` cannot be written, or both name the same file);
/// `os.errno()` tells why.
static TValue fs_copy(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    char *src = _arg_path_str(args, nargs, 0);
    char *dst = _arg_path_str(args, nargs, 1);

    int in = open(src, O_RDONLY | O_CLOEXEC);
    if (in < 0) return bool_value(0);

    struct stat sb;
    if (fstat(in, &sb) != 0) goto fail_in;
    if (S_ISDIR(sb.st_mode)) {
        errno = EISDIR;
        goto fail_in;
    }

    /* copying a file onto itself would truncate it first */
    struct stat db;
    if (stat(dst, &db) == 0 && db.st_dev == sb.st_dev && db.st_ino == sb.st_ino) {
        errno = EINVAL;
        goto fail_in;
    }

    int out = open(dst, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
    if (out < 0) goto fail_in;

    char buf[65536];
    for (;;) {
        ssize_t r = read(in, buf, sizeof(buf));
        if (r < 0) {
            if (errno == EINTR) continue;
            goto fail_out;
        }
        if (r == 0) break;
        if (_write_full(out, buf, r) != 0) goto fail_out;
    }

    /* the permission bits of `src`, not limited by the umask */
    if (fchmod(out, sb.st_mode & 07777) != 0) goto fail_out;

    close(in);
    return bool_value(close(out) == 0);

fail_out:
    close(out);
fail_in:
    close(in);
    return bool_value(0);
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
    kl_reg_func(lib, "is_symlink", fs_is_symlink);
    kl_reg_func(lib, "stat", fs_stat);
    kl_reg_func(lib, "real_path", fs_real_path);
    kl_reg_func(lib, "size", fs_size);
    kl_reg_func(lib, "remove", fs_remove);
    kl_reg_func(lib, "rename", fs_rename);
    kl_reg_func(lib, "mkdir", fs_mkdir);
    kl_reg_func(lib, "rmdir", fs_rmdir);
    kl_reg_func(lib, "copy", fs_copy);
    kl_reg_func(lib, "chmod", fs_chmod);
    kl_reg_func(lib, "chown", fs_chown);
    kl_reg_func(lib, "touch", fs_touch);
    kl_reg_func(lib, "set_times", fs_set_times);
    kl_reg_func(lib, "samefile", fs_samefile);
    kl_reg_func(lib, "hard_link", fs_hard_link);
    kl_reg_func(lib, "disk_usage", fs_disk_usage);
    kl_reg_func(lib, "access", fs_access);
    kl_reg_func(lib, "symlink", fs_symlink);
    kl_reg_func(lib, "read_link", fs_read_link);
    kl_reg_func(lib, "_glob", fs_glob);
    kl_reg_func(lib, "temp_dir", fs_temp_dir);
    kl_reg_func(lib, "make_temp_file", fs_make_temp_file);
    kl_reg_func(lib, "make_temp_dir", fs_make_temp_dir);
    kl_reg_func(lib, "_fnmatch", fs_fnmatch);

    kl_reg_func(lib, "list_dir", fs_list_dir);
    kl_reg_func(lib, "_try_list_dir", fs_try_list_dir);

    kl_reg_type(lib, &file_type);
    /* Bind the Koala `Path` class to the C `path_type` so natives can read a
       `Path | str` argument directly. No methdefs: `Path` is pure Koala. */
    kl_reg_type(lib, &path_type);
    /* Bind the Koala `Stat` class to `stat_type`, so `stat` can build one */
    kl_reg_type(lib, &stat_type);
}

#ifdef __cplusplus
}
#endif
