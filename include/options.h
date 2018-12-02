
#ifndef _KOALA_OPTIONS_H_
#define _KOALA_OPTIONS_H_

#include "common.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OPTIONS_DELIMTER ':'

struct options {
	char *delimiter;
	char *srcpath;
	char *klcpath;
	Vector klcvec;
	Vector cmdvec;
	char *output;
	char *kar;
	char __delims[2];
};

int init_options(struct options *ops, char delimiter);
int parse_options(int argc, char *argv[], struct options *ops);
void show_options(struct options *ops);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_OPTIONS_H_ */
