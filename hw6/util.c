#include "util.h"

void print_inode(Inode* inode,int index, int detail){
  char status[20] = "";
  if(inode->next_inode) {
    strcpy(status,"FREE");
  }
  else {
    strcpy(status,"USED");
  }
  printf("Inode %d is %s, the file size is %d\n",index, status, inode->size);
    if(detail) {        
        printf("Direct blocks: {");
        for(int i = 0; i < N_DBLOCKS; i++) {
            printf("%d, ", inode->dblocks[i]);
        }
        printf("}\n");               
        
        printf("Indirect blocks: {");
        for(int i = 0; i < N_IBLOCKS; i++) {
            printf("%d, ", inode->iblocks[i]);
        }
        printf("}\n");               
	
    }
}

void tutorial() {
  printf("Welcome to use my file defrag program :)\n");
  printf("Run the program by this format: ./defrag <fragmented disk file>\n");
  printf("the output file will have a \"-defrag\" extension, for exmaple, the output file of  ./defrag myfile is \"myfile-defrag\"\n");
  printf("use ./defrag -h to invoke this tutorial again :)\n");
}

Inode* next_inode(Inode* inode) {
  return (Inode*)((char*)inode + INODE_SIZE);
  
}
