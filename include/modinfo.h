/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_MODINFO_H_
#define _KOALA_MODINFO_H_

#ifdef  __cplusplus
extern "C" {
#endif

typedef void (*InitFunc)(void);
typedef void (*FiniFunc)(void);

typedef struct __attribute__((aligned(16))) modinfo {
  char *name;
  InitFunc init;
  FiniFunc fini;
} ModInfo;

#define REGISTER_MODULE_LEVEL(name, init, fini, lvl) \
  static ModInfo mod_##lvl##_##name        \
  __attribute__((__section__("mod_"#lvl))) \
  __attribute__((__used__)) = {            \
    #name, init, fini                      \
  }

#define REGISTER_MODULE0(name, init, fini) \
  REGISTER_MODULE_LEVEL(name, init, fini, 0)

#define REGISTER_MODULE1(name, init, fini) \
  REGISTER_MODULE_LEVEL(name, init, fini, 1)

#define REGISTER_MODULE2(name, init, fini) \
  REGISTER_MODULE_LEVEL(name, init, fini, 2)

#define REGISTER_MODULE3(name, init, fini) \
  REGISTER_MODULE_LEVEL(name, init, fini, 3)

/* call all registered modules' initial functions */
void init_modules(void);

/* call all registered modules' finalized functions */
void fini_modules(void);

#ifdef  __cplusplus
}
#endif

#endif /* _KOALA_MODINFO_H_ */
