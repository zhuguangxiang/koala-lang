
#ifndef _KOALA_OPTIONS_H_
#define _KOALA_OPTIONS_H_

#include "common.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

struct options {
  char *cmd;
  char *delimiter;
  char *srcpkg;
  char *outpkg;
  char *klc;
  char *kar;
  Vector klcvec;
  Vector karvec;
  Vector args;
  char __delims[2];
};

int init_options(struct options *ops, char *cmd, char delimiter);
int parse_options(int argc, char *argv[], struct options *ops);
void show_options(struct options *ops);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_OPTIONS_H_ */
