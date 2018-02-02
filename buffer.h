
#ifndef _KOALA_BUFFER_H_
#define _KOALA_BUFFER_H_

#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BLOCK_SIZE  4096

typedef struct block {
  struct list_head link;
  int used;
  uint8 data[0];
} Block;

typedef struct buffer {
  int size;   /* total bytes in buffer */
  int blocks; /* number of blocks */
  int bsize;  /* Block size */
  struct list_head head;
} Buffer;

int Buffer_Init(Buffer *buf, int bsize);
int Buffer_Fini(Buffer *buf);
int Buffer_Write(Buffer *buf, uint8 *data, int sz);
uint8 *Buffer_RawData(Buffer *buf);

static inline int Buffer_Write_Byte(Buffer *buf, uint8 data)
{
  return Buffer_Write(buf, &data, 1);
}

static inline int Buffer_Write_2Bytes(Buffer *buf, uint16 data)
{
  return Buffer_Write(buf, (uint8 *)&data, 2);
}

static inline int Buffer_Write_4Bytes(Buffer *buf, uint32 data)
{
  return Buffer_Write(buf, (uint8 *)&data, 4);
}

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_BUFFER_H_ */
