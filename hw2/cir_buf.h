/* cir_buf.h
   Declarations for an abstract sequence type backed by a char**
*/

#ifndef CIR_BUF_
#define CIR_BUF_
#include <stdbool.h>

typedef struct {
  char** buffer;
  int head;
  int tail;
  int size;
  bool empty;
  bool full;
} cir_buf;

int cir_buf_init(cir_buf* cbuf);
bool cir_buf_empty(cir_buf cbuf);
bool cir_buf_full(cir_buf cbuf);
int cir_buf_put(cir_buf* cbuf, char* data);
int cir_buf_get_tail(cir_buf* cbuf, char** output);
int cir_buf_get_head(cir_buf* cbuf, char** output);
void cir_buf_clean(cir_buf* cbuf);

#endif
