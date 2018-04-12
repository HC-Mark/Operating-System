#ifndef _UTIL
#define _UTIL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h> 
#include <fcntl.h>
#define TRUE 1
#define FALSE 0

#define N_DBLOCKS 10
#define N_IBLOCKS 4
#define H_UNIT 512
#define TOTAL_HEAD 1024
#define INODE_SIZE sizeof(Inode)
#define DEFAULT_BUFFER 512 * 128
#define FREE_BUF 512 - sizeof(int)

char* file_buffer;
char* new_super; //buffer to store new superblock
char* new_i_region; //buffer to store new i_node lists
char* write_buffer;//used to store the output content
FILE* fp_r;
FILE* fp_w;
FILE* fp_test;
int block_size;
int write_counter;
int inode_start;
int inode_end;
int inode_num;
size_t data_start; //the beginning of data region
size_t data_end;
int w_buffer_ptr; //used to keep track next available byte in write buffer
size_t data_addr_w;  //data address to write in fp_w
int data_index_w;
int num_indir_idx;
typedef struct superblock {
int size; /* size of blocks in bytes */
int inode_offset; /* offset of inode region in blocks */
int data_offset; /* data region offset in blocks */
int swap_offset; /* swap region offset in blocks */
int free_inode; /* head of free inode list, index */
int free_block; /* head of free block list, index */
} Superblock;

typedef struct inode {
int next_inode; /* index of next free inode */
int protect; /* protection field */
int nlink; /* number of links to this file */
int size; /* numer of bytes in file */
int uid; /* owner’s user ID */
int gid; /* owner’s group ID */
int ctime; /* change time */
int mtime; /* modification time */
int atime; /* access time */
int dblocks[N_DBLOCKS]; /* pointers to data blocks */
int iblocks[N_IBLOCKS]; /* pointers to indirect blocks */
int i2block; /* pointer to doubly indirect block */
int i3block; /* pointer to triply indirect block */
} Inode;

typedef struct free_block {
  int next_free;
  char padding[FREE_BUF];
}fb;

Inode* inode_head;
Superblock* sp;

void print_inode(Inode* inode, int index, int detail);
void tutorial();
Inode* next_inode(Inode* inode);
#endif
