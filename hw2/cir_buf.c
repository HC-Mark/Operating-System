#include <stdlib.h>
#include <stdio.h>
#include "cir_buf.h"


int cir_buf_init(cir_buf* cbuf) {
  int r = -1; //default to be not successful

  if(cbuf) {
    cbuf->head = 0;
    cbuf->tail = 0;
    cbuf->empty = true;
    cbuf->full = false;
    r = 0;
  }

  return r;
}

bool cir_buf_empty(cir_buf cbuf) {
  //it will be empty only when head = tail
  return ((cbuf.head == cbuf.tail) && cbuf.empty);
}

bool cir_buf_full(cir_buf cbuf) {
  return ((cbuf.head == cbuf.tail) && !(cbuf.empty));
}

int cir_buf_put(cir_buf* cbuf, char* data) {
  int r = -1;

  if(cbuf) {
    if(cbuf->head == cbuf->tail)
      cbuf->empty = false;
    cbuf->buffer[cbuf->head] = data;
    cbuf->head = (cbuf->head + 1) % cbuf->size;
    //printf("the data I get is %s\n", data);
    //if head meets with tail, then the cir_buf is full
    if(cbuf->head == cbuf->tail) {
      //before we move to next spot, we need to free the current tail string
      free(cbuf->buffer[cbuf->tail]);
      cbuf->full = true; //will be assigned multiple times, but its ok
      cbuf->tail = (cbuf->tail +1) % cbuf->size;
    }

    r = 0;
  }

  return r;

}

//the return value will be stored at the pointer passed in?
//I will try to use return first

//use get_tail can not output the content that head is pointing to
int cir_buf_get_tail(cir_buf* cbuf, char** output) {
  int r = -1;
  if(cbuf && !cir_buf_empty(*cbuf)) {
    *output = cbuf->buffer[cbuf->tail];
    //printf("the output now is %s\n",*output);
    //cbuf->tail = (cbuf->tail +1) % cbuf->size;
    r = 0;
  }

  return r;
}

//use get_head can not output the content that tail is pointing to
int cir_buf_get_head(cir_buf* cbuf, char** output) {
  int r = -1;
  if(cbuf  && !cir_buf_empty(*cbuf)) {
    *output = cbuf-> buffer[cbuf->head];
    /*
    if( cbuf-> head == 0)
      cbuf->head = cbuf->size-1;
    else
      cbuf->head = cbuf->head - 1;
    */
    r=0;
  }
  return r;
}

void cir_buf_clean(cir_buf* cbuf) {
  int clean_num;
  if (cbuf->full)
    clean_num = cbuf->size - 1;
  else
    clean_num = cbuf->head - cbuf->tail;
  //here actually we have two situations, the array is full or not.
  //if it is full, we want to clean size-1 item
  //if not, we want to clean head - tail
  for(int i = 0; i < clean_num; i++) {
    //in case we double free array element
    if(cbuf->buffer[cbuf->tail])
      free(cbuf->buffer[cbuf->tail]);
    cbuf->tail = (cbuf->tail + 1) % cbuf->size;
  }
}
/*
int main() {
  cir_buf cbuf;
  cbuf.size = 5;
  cbuf.buffer = malloc(cbuf.size * sizeof(char*));
  cir_buf_init(&cbuf);
  char* hello = "Hello, World!\n";
  char* Mark  = "Mark, hey";
  for(int i = 0; i < cbuf.size; i++) {
    //printf("head is %d, tail is %d\n",cbuf.head, cbuf.tail);
    cir_buf_put(&cbuf, hello);
    
  }
  cir_buf_put(&cbuf, Mark);
  char* test;
  for (int i = 0; i < cbuf.size; i++) {
    printf("head is %d, tail is %d\n",cbuf.head, cbuf.tail);
    cir_buf_get_head(&cbuf, &test);
    printf("I get %s\n", test);
    //printf("head is %d\t, tail is %d\n", cbuf.head, cbuf.tail);
    printf(" is empty? %d\n", cir_buf_empty(cbuf));
  }
  return 0;

}
*/

