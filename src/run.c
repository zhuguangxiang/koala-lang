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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include "koala.h"
#include "osmodule.h"
#include "iomodule.h"
#include "fmtmodule.h"
#include "gc.h"
#include "log.h"

static void init_types(void)
{
  print("########\n");

  int res = type_ready(&type_type);
  if (res != 0)
    panic("Cannot initalize 'Type' type.");

  init_any_type();
  init_descob_type();
  init_number_type();
  init_integer_type();
  init_byte_type();
  init_bool_type();
  init_string_type();
  init_char_type();
  init_float_type();

  init_array_type();
  init_tuple_type();
  init_map_type();
  init_field_type();
  init_method_type();
  init_proto_type();
  init_class_type();
  init_module_type();

  res = type_ready(&code_type);
  if (res != 0)
    panic("Cannot initalize 'Code' type.");
  init_label_type();
  init_range_type();
  init_iter_type();
  init_closure_type();
  init_fmtter_type();

  print("########\n");
}

static void fini_types(void)
{
  print("########\n");
  type_fini(&type_type);
  type_fini(&any_type);
  type_fini(&descob_type);
  type_fini(&number_type);
  type_fini(&byte_type);
  type_fini(&integer_type);
  type_fini(&bool_type);
  type_fini(&string_type);
  type_fini(&char_type);
  type_fini(&float_type);
  type_fini(&array_type);
  type_fini(&tuple_type);
  type_fini(&map_type);
  type_fini(&field_type);
  type_fini(&method_type);
  type_fini(&proto_type);
  type_fini(&class_type);
  type_fini(&module_type);
  type_fini(&code_type);
  type_fini(&label_type);
  type_fini(&range_type);
  type_fini(&iter_type);
  type_fini(&fmtter_type);
  type_fini(&closure_type);
  fini_bool_type();
  print("########\n");
}

void koala_initialize(void)
{
  init_atom();
  init_typedesc();
  init_types();
  init_lang_module();
  init_os_module();
  init_io_module();
  init_fmt_module();
  init_assert_module();
  init_parser();
}

void koala_finalize(void)
{
  fini_parser();
  fini_assert_module();
  fini_fmt_moudle();
  fini_io_module();
  fini_os_module();
  fini_lang_module();
  fini_modules();
  gc();
  fini_types();
  fini_typedesc();
  fini_atom();
}

int check_dotkl(char *path);
int check_dir(char *path);
int isdotkl(char *filename);
int isdotklc(char *filename);

static struct timespec *get_mtimespec(struct stat *sb)
{
#if defined(__clang__)
  return &sb->st_mtimespec;
#elif defined(__GNUC__)
  return &sb->st_mtim;
#elif defined(_MSC_VER)
  return &sb->st_mtim;
#else
  return &sb->st_mtim;
#endif
}

static inline int timespec_cmp(struct timespec *t1, struct timespec *t2)
{
  if (t1->tv_sec == t2->tv_sec) {
    return t1->tv_nsec > t2->tv_nsec;
  } else {
    return t1->tv_sec > t2->tv_sec;
  }
}

int file_need_compile(char *path)
{
  struct stat sb1, sb2;
  char *klcpath;
  int need;

  if (isdotkl(path)) {
    klcpath = str_dup_ex(path, "c");
  } else {
    klcpath = str_dup_ex(path, ".klc");
  }
  int status = lstat(klcpath, &sb1);
  kfree(klcpath);

  if (status) {
    // .klc not exist
    warn("%s: %s", klcpath, strerror(errno));
    need = 1;
  } else {
    if (lstat(path, &sb2)) {
      error("%s: %s", path, strerror(errno));
      need = 0;
    } else {
      // last modified time .kl file > .klc file
      need = timespec_cmp(get_mtimespec(&sb2), get_mtimespec(&sb1));
    }
  }

  debug("file %s need compile:%d", path, need);
  return need;
}

int dir_need_compile(char *path)
{
  struct stat sb1, sb2;
  char *klcpath = str_dup_ex(path, ".klc");
  int need;
  int status = lstat(klcpath, &sb1);
  kfree(klcpath);

  if (status) {
    // .klc not exist
    warn("%s: %s", klcpath, strerror(errno));
    need = 1;
  } else {
    DIR *dir = opendir(path);
    if (dir == NULL) {
      fprintf(stderr, "%s: No such file or directory\n", path);
      need = 0;
    } else {
      char *filename;
      char fullpath[4096];
      int pathlen = strlen(path) + 1;
      struct dirent *dent;
      while ((dent = readdir(dir))) {
        filename = dent->d_name;
        if (!isdotkl(filename))
          continue;

        if (pathlen + strlen(filename) >= sizeof(fullpath)) {
          fprintf(stderr, "path '%s/%s' is too long\n", path, filename);
          continue;
        }

        snprintf(fullpath, sizeof(fullpath) - 1, "%s/%s", path, filename);

        if (lstat(fullpath, &sb2) || !S_ISREG(sb2.st_mode))
          continue;

        // last modified time .kl file > .klc file
        need = timespec_cmp(get_mtimespec(&sb2), get_mtimespec(&sb1));
        if (need)
          break;
      }
      closedir(dir);
    }
  }

  debug("dir %s need compile:%d", path, need);
  return need;
}

static int need_compile(char *path)
{
  int res = 0;
  // for script with no suffix .kl
  struct stat sb;
  if (!lstat(path, &sb) && S_ISREG(sb.st_mode)) {
    if (file_need_compile(path))
      res = 1;
  } else {
    // check path.kl file exist?
    char *klpath = str_dup_ex(path, ".kl");
    if (!lstat(klpath, &sb) && S_ISREG(sb.st_mode)) {
      if (!check_dotkl(klpath) && file_need_compile(klpath))
        res = 1;
    } else {
      // module directory
      if (!check_dir(path) && dir_need_compile(path)) {
        res = 1;
      }
    }
    kfree(klpath);
  }
  return res;
}

/*
 * The following commands are valid.
 * ~$ koala a/b/foo.kl [a/b/foo.klc] [a/b/foo]
 */
void koala_run(char *path)
{
  if (isdotkl(path)) {
    path = str_ndup(path, strlen(path) - 3);
  } else if (isdotklc(path)) {
    path = str_ndup(path, strlen(path) - 4);
  } else {
    // trim slash at the tail.
    char *end = path + strlen(path) - 1;
    while (*end == '/') --end;
    path = str_ndup(path, end - path + 1);
  }

  // path: a/b/foo, try compile it
  if (need_compile(path)) {
    debug("try to compile '%s'", path);
    koala_compile(path);
  }

  extern pthread_key_t kskey;
  Object *mo = module_load(path);
  if (mo != NULL) {
    Object *_init_ = module_lookup(mo, "__init__");
    if (_init_ != NULL) {
      expect(method_check(_init_));
      KoalaState kstate = {NULL};
      kstate.top = -1;
      pthread_setspecific(kskey, &kstate);
      Object *code = method_getcode(_init_);
      koala_evalcode(code, mo, NULL);
      OB_DECREF(code);
      OB_DECREF(_init_);
    } else {
      warn("__init__ is empty");
    }
    OB_DECREF(mo);
  }

  kfree(path);
}
