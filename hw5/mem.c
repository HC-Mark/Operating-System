#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/mman.h>
#include <unistd.h
#include <string.h>
#include "mem.h"

#define E_NO_SPACE            1
#define E_CORRUPT_FREESPACE   2
#define E_PADDING_OVERWRITTEN 3
#define E_BAD_ARGS            4
#define E_BAD_POINTER         

#define HEADER_SIZE 32
#define UNIT 8

#define TRUE 1
#define FALSE 0
#define FREE 0
#define ALLOC 1
#define SUCCEED 0
#define FAILED -1
typedef struct header_node {
  char pad_start;
  struct header_node* next;
  struct header_node* prev;
  struct header_node* next_free;
  int status;
  char pad_end;
}header;

typedef struct main_header_node {
  header* allocated_start;
  header* free_list;
  long chunk_size;
  int global_coalesce;
} main_header;

main_header* main_head = NULL;
int m_error;

void split_block(header* h, long size);
long get_block_size(header* cur);

int Mem_Init(long sizeOfRegion) {
  if(main_head != NULL) {
    m_error = E_BAD_ARGS;
    return FAILED;#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include "mem.h"

#define E_NO_SPACE            1
#define E_CORRUPT_FREESPACE   2
#define E_PADDING_OVERWRITTEN 3
#define E_BAD_ARGS            4
#define E_BAD_POINTER         5

#define HEADER_SIZE 32
#define UNIT 8
#define MIN_BLOCK_SIZE 40
#define PAD 7

#define TRUE 1
#define FALSE 0
#define FREE 0
#define ALLOC 1
#define SUCCEED 0
#define FAILED -1
typedef struct header_node {
  char pad_start;
  struct header_node* next;
  struct header_node* prev;
  struct header_node* next_free;
  int status;
  char pad_end;
}header;

typedef struct main_header_node {
  header* allocated_start;
  header* free_list;
  long chunk_size;
  int global_coalesce;
  //long remaining_space;
} main_header;

main_header* main_head = NULL;
int m_error;

void split_block(header* h, long size);
long get_block_size(header* cur);
int validate_ptr(void* ptr);
void global_coalesce(header* h);
void local_coalesce(header* h);

int Mem_Init(long sizeOfRegion) {
  if(main_head != NULL) {
    m_error = E_BAD_ARGS;
    return FAILED;
  }
  if(sizeOfRegion <= 0) {
    m_error = E_BAD_ARGS;
    return FAILED;
  }
  long user_size = sizeOfRegion;
  long set_num = (long)round(sizeOfRegion * 1.0f/UNIT + 0.5f);
  long total_byte = set_num * 40;
  long round_up = (long)round(total_byte * 1.0f/getpagesize() + 0.5f);
  //we need to assume the worst case for user allocation
  //printf("round up is now %ld\n",round_up);
  sizeOfRegion = round_up * getpagesize();
  //printf("sizeofRegion is now %ld\n", sizeOfRegion);
  if((main_head = mmap(NULL,sizeOfRegion, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED) {
    m_error = E_BAD_ARGS;
    return FAILED;
  }
  //set up the main head block
  main_head->allocated_start = NULL;
  main_head->free_list = NULL;
  main_head -> chunk_size = sizeOfRegion;
  main_head->global_coalesce = FALSE;
  //main_head->remaining_space = user_size;
  //free_list = allocated_start;
  //set up the first block
  header* first_head = (header*) ((char*)main_head + HEADER_SIZE);
  first_head->pad_start = PAD;
  first_head->next = NULL;
  first_head->prev = NULL;
  main_head->free_list = first_head; //the free list share same block  
  first_head->next_free = NULL;
  first_head->status = FREE;
  first_head->pad_end = PAD;
  main_head->allocated_start = first_head;
  return SUCCEED;
}

void *Mem_Alloc(long size) {
  if(main_head == NULL) {
    m_error = E_BAD_ARGS;
    return NULL;
  }
  //round up size to align 8
  long round_up = round((size-1) * 1.0f/UNIT + 0.5f);
  size = round_up * UNIT;
  //printf("we assign %ld to user\n",size);

  header* current = main_head->free_list;
  header* prev_free = NULL; //keep track of the free block before the one we choose
  header* free_block = NULL; //the free block we choose
  header* tmp_prev = NULL;
  //int flag = FALSE;
  long max_size = 0;
  //find the worst fit block
  while(current != NULL) {
    //if this is the first time run
    if(current->pad_start != PAD || current -> pad_end != PAD) {
      m_error = E_CORRUPT_FREESPACE;
      return NULL;
    }
    long block_size = get_block_size(current);
    if(block_size > size && block_size > max_size) {
      max_size = block_size;
      prev_free = tmp_prev;
      free_block = current;
    }
    tmp_prev = current; // keep track of the one before the largest free block
    current = current->next_free;
  }

  //if the max_size doesn't change, we can't hold this alloc
  if(max_size == 0) {
    printf("Don't have enough space\n");
    m_error = E_NO_SPACE;
    return NULL;
  }

  //test whether we need to split the found block
  if(max_size > size + HEADER_SIZE + MIN_BLOCK_SIZE) {
    header* next_block = free_block->next;
    header* new_free = (header*)((char*)free_block + HEADER_SIZE + size);
    
    if(next_block != NULL)
      next_block->prev = new_free;
    new_free->prev = free_block;
    new_free->next_free = free_block->next_free;
    new_free->next = next_block;
    new_free->status = FREE;
    new_free->pad_start = PAD;
    new_free->pad_end = PAD;
    free_block->next = new_free;
    //check whether we need to update free_list -- if not work, move the whole function back to here
    if(prev_free == NULL)
      main_head->free_list = new_free;
    else
      prev_free->next_free = new_free;
    
  }
  else {
    //if the largest block is the first, we need to update free_list pointer
    if(prev_free == NULL){
      main_head->free_list = free_block->next_free;
    }
  }
  free_block->next_free = NULL; //already be allocated
  free_block->status = ALLOC;
  char* return_address = (char*)free_block + HEADER_SIZE;
  return (void*) return_address;
}

int Mem_Free(void *ptr, int coalesce) {
  if(main_head == NULL) {
    m_error = E_BAD_ARGS;
    return FAILED;
  }
  if(ptr == NULL && coalesce == 0) {
    printf("can't free NULL Pointer");
    return FAILED;
  }
  //validate the input ptr
  if(ptr == NULL && coalesce == 1) {
    header* current = main_head->free_list;
    global_coalesce(current);
    main_head->global_coalesce = FALSE;
    return SUCCEED;
  }
  if(validate_ptr(ptr) == SUCCEED) {
    //do the free here, without coalesce
    //first edit the block before ptr
    header* cur = (header*)((char*)ptr - HEADER_SIZE);
    if(cur->pad_start != PAD || cur -> pad_end != PAD) {
      m_error = E_CORRUPT_FREESPACE;
      return FAILED;
    }
    cur->status = FREE;
    header* prev_free = cur->prev;
    while( prev_free != NULL && prev_free->status != FREE) {
      prev_free = prev_free->prev;
    }
    if(prev_free != NULL) {
      //printf("prev_free is not empty!\n");
      cur->next_free = prev_free -> next_free;
      prev_free -> next_free = cur;
    }
    //what if prev_free is null? update free list head
    else {
      //printf("prev_free is null\n");
      cur->next_free = main_head->free_list;
      main_head->free_list = cur;
    }
  }
  else {
    printf("It is not a valid ptr allocated by library or has been freed\n");
    m_error = E_BAD_ARGS;
    return FAILED;
  }
  header* current;
  if(coalesce) {
    //if the ptr is null, coalesce the whole list
    if(main_head->global_coalesce == TRUE) {
      current = main_head->free_list;
      global_coalesce(current);
      main_head->global_coalesce = FALSE;
    }
    else {
      current = (header*)((char*)ptr - HEADER_SIZE);
      local_coalesce(current);
    }
  }
  else {
    main_head->global_coalesce = TRUE; // we need to coalesce this when next time we coalesce the list
  }
  
  return SUCCEED;
}
void Mem_Dump() {
  if(main_head == NULL) {
    m_error = E_BAD_ARGS;
    return;
  }
  header* current = main_head->allocated_start;
  int counter = 1;
  long total_size = main_head->chunk_size;
  printf("\n\n#########  the whole block of mmap is %ld ########\n",total_size);
  printf("-----Here are our complete memory list:------\n");
  while(current != NULL) {
    char status[HEADER_SIZE];
    long block_size = get_block_size(current);
    if(current->status == ALLOC)
      strcpy(status, "Allocated");
    else
      strcpy(status,"Free");
    printf("%d memory block is %s, has size %ld\n",counter,status,block_size);
    counter++;
    current = current -> next;
  }
  printf("\n\n");

  current = main_head->free_list;
  counter = 1;
  printf("*********Here are our complete free list:***********\n");
  while(current != NULL) {
    char status[HEADER_SIZE];
    long block_size = get_block_size(current);
    if(current->status == ALLOC)
      strcpy(status, "Allocated");
    else
      strcpy(status,"Free");
    printf("%d memory block is %s, has size %ld\n",counter,status,block_size);
    counter++;
    current = current -> next_free;
  }
  printf("\n\n");
}
int validate_ptr(void* ptr) {
  header* tmp = (header*)((char*)ptr - HEADER_SIZE);
  if(tmp->status == ALLOC)
    return SUCCEED;
  if(tmp->status == FREE)
    printf("The block has been freed, no need double free it\n");
  return FAILED;
}
void global_coalesce(header* h) {
  //assume the h can only be the head of free list
  //printf("I am in global_coalesce\n");
  while(h != NULL) {
    if(h -> next != NULL) {
      while (h ->next ->status == FREE) {
	//printf("get here?\n");
	h-> next_free = h->next->next_free;
	h->next = h->next->next;
	if(h->next != NULL)
	  h->next->prev = h;
	else
	  return;
	    //h = h->next;
	    //Mem_Dump();
      }
    }
    else
      return; //return when reaches the end of free list
    h = h -> next_free;
  }
}
void local_coalesce(header* h) {
  //coalesce the next
  //printf("I am in local_coalesce\n");
  if(h-> next != NULL) {
    while (h -> next ->status == FREE) {
      h-> next_free = h->next->next_free;
      h->next =	h->next->next;
      if(h->next != NULL)
        h->next->prev =	h;
      else
	break;
      //h= h->next;
    }
    // Mem_Dump();
  }//if we reaches the end of free list
  //coalesce the prevous blocks
  //if we reaches the head of free list
  if(h->prev != NULL) {
    while( h->prev-> status == FREE) {
      //printf("in prev\n");
      h->prev->next_free = h->next_free;
      h->prev->next = h->next;
      if(h->next != NULL)
	h->next->prev = h->prev;
      h = h->prev;
      if(h->prev == NULL)
	break;
    }
    //  Mem_Dump();
  }
}

void split_block(header* h, long size) {
  header* next_block = h->next;
  header* new_free = (header*)((char*)h + HEADER_SIZE + size);

  if(next_block != NULL)
    next_block->prev = new_free;
  new_free->prev = h;
  new_free->next_free = h->next_free;
  new_free->next = next_block;
  new_free->status = FREE;
  new_free->pad_start = 0;
  new_free->pad_end = 0;
  h->next = new_free;
}

long get_block_size(header* cur) {
  long block_size;
  if(cur->next == NULL) {
      block_size = (char*)main_head + main_head->chunk_size - ((char*)cur + HEADER_SIZE);
    }
    else {
      block_size = (char*)cur->next - ((char*)cur + HEADER_SIZE);
    }
  return block_size;
}
  }
  if(sizeOfRegion <= 0) {
    m_error = E_BAD_ARGS;
    return FAILED;
  }
  long set_num = (long)round(sizeOfRegion * 1.0f/UNIT + 0.5f);
  long total_byte = set_num * 40;
  long round_up = (long)round(total_byte * 1.0f/getpagesize() + 0.5f);
  //we need to assume the worst case for user allocation
  //printf("round up is now %ld\n",round_up);
  sizeOfRegion = round_up * getpagesize();
  //printf("sizeofRegion is now %ld\n", sizeOfRegion);
  if((main_head = mmap(NULL,sizeOfRegion, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED) {
    m_error = E_BAD_ARGS;
    return FAILED;
  }
  //set up the main head block
  main_head->allocated_start = NULL;
  main_head->free_list = NULL;
  main_head -> chunk_size = sizeOfRegion;
  main_head->global_coalesce = FALSE;
  //free_list = allocated_start;
  //set up the first block
  header* first_head = (header*) ((char*)main_head + HEADER_SIZE);
  first_head->pad_start = 0;
  first_head->next = NULL;
  first_head->prev = NULL;
  main_head->free_list = first_head; //the free list share same block  
  first_head->next_free = NULL;
  first_head->status = FREE;
  first_head->pad_end = 0;
  main_head->allocated_start = first_head;
  return SUCCEED;
}

void *Mem_Alloc(long size) {
  if(main_head == NULL) {
    m_error = E_BAD_ARGS;
    return NULL;
  }
  //round up size to align 8
  long round_up = round(size * 1.0f/UNIT + 0.5f);
  size = round_up * UNIT;
  printf("we assign %ld to user\n",size);

  header* current = main_head->free_list;
  header* prev_free = NULL; //keep track of the free block before the one we choose
  header* free_block = NULL; //the free block we choose
  header* tmp_prev = NULL;
  int flag = FALSE;
  long max_size = 0;
  //find the worst fit block
  while(current != NULL) {
    //if this is the first time run
    long block_size = get_block_size(current);
    if(block_size > size && block_size > max_size) {
      max_size = block_size;
      prev_free = tmp_prev;
      free_block = current;
    }
    tmp_prev = current; // keep track of the one before the largest free block
    current = current->next_free;
  }

  //if the max_size doesn't change, we can't hold this alloc
  if(max_size == 0) {
    printf("Don't have enough space\n");
    m_error = E_NO_SPACE;
    return NULL;
  }

  //test whether we need to split the found block
  if(max_size > size + HEADER_SIZE) {
    header* next_block = free_block->next;
    header* new_free = (header*)((char*)free_block + HEADER_SIZE + size);
    
    if(next_block != NULL)
      next_block->prev = new_free;
    new_free->prev = free_block;
    new_free->next_free = free_block->next_free;
    new_free->next = next_block;
    new_free->status = FREE;
    new_free->pad_start = 0;
    new_free->pad_end = 0;
    free_block->next = new_free;
    //check whether we need to update free_list -- if not work, move the whole function back to here
    if(prev_free == NULL)
      main_head->free_list = new_free;
    else
      prev_free->next_free = new_free;
    
  }
  else {
    //if the largest block is the first, we need to update free_list pointer
    if(prev_free == NULL){
      main_head->free_list = free_block->next_free;
    }
  }
  free_block->next_free = NULL; //already be allocated
  free_block->status = ALLOC;
  char* return_address = (char*)free_block + HEADER_SIZE;
  return (void*) return_address;
}

int Mem_Free(void *ptr, int coalesce);

void Mem_Dump() {
  header* current = main_head->allocated_start;
  int counter = 1;
  printf("-----Here are our complete memory list:------\n");
  while(current != NULL) {
    char status[HEADER_SIZE];
    long block_size = get_block_size(current);
    if(current->status == ALLOC)
      strcpy(status, "Allocated");
    else
      strcpy(status,"Free");
    printf("%d memory block is %s, has size %ld\n",counter,status,block_size);
    counter++;
    current = current -> next;
  }
  printf("\n\n");

  current = main_head->free_list;
  counter = 1;
  printf("-----Here are our complete free list:------\n");
  while(current != NULL) {
    char status[HEADER_SIZE];
    long block_size = get_block_size(current);
    if(current->status == ALLOC)
      strcpy(status, "Allocated");
    else
      strcpy(status,"Free");
    printf("%d memory block is %s, has size %ld\n",counter,status,block_size);
    counter++;
    current = current -> next_free;
  }
  printf("\n\n");
}

void split_block(header* h, long size) {
  header* next_block = h->next;
  header* new_free = (header*)((char*)h + HEADER_SIZE + size);

  if(next_block != NULL)
    next_block->prev = new_free;
  new_free->prev = h;
  new_free->next_free = h->next_free;
  new_free->next = next_block;
  new_free->status = FREE;
  new_free->pad_start = 0;
  new_free->pad_end = 0;
  h->next = new_free;
}

long get_block_size(header* cur) {
  long block_size;
  if(cur->next == NULL) {
      block_size = (char*)main_head + main_head->chunk_size - ((char*)cur + HEADER_SIZE);
    }
    else {
      block_size = (char*)cur->next - ((char*)cur + HEADER_SIZE);
    }
  return block_size;
}

int main() {
  if(Mem_Init(4000) < 0)
    printf("Sad, I failed\n");
  else
    printf("init successfully\n");
  /*
  if(main_head == NULL)
    printf("where is first head\n");
  else {
    if(main_head->allocated_start->status == FREE)
      printf("seems like it does work\n");
  }
  printf("chunk size is now %ld\n",main_head->chunk_size);
  */
  char* alloc_list[10];
  for(int i = 0; i < 10; i++) {
    alloc_list[i] = Mem_Alloc(100);
  }
  Mem_Dump();
  return 0;
}
