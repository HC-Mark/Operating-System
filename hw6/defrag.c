/* collaborate with Eileen Feng, Zhanpeng Wang
   Get inspired by https://github.com/stpddream/OSHomework and 
   https://www.slashroot.in/inode-and-its-structure-linux
*/
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include "util.h"
void arrange_data(Inode* source_inode, Inode* new_inode);
int Dblock_copy(int index);
void Iblock_copy();
void write_in();
void update_inode(Inode* source, Inode* new);
void clean_inode(Inode* rest, int index);
void clean_up();
void write_in_free();
void write_in_f();
int main(int argc, char** argv) {

  if (argc != 2) {
    tutorial();
    return FALSE;
  }
  else {
    if(strcmp(argv[1],"-h") == 0) {
      tutorial();
      return FALSE;
    }
  }
  char* filename = argv[1];
  char* output_filename = malloc(sizeof(char) *TOTAL_HEAD);
  strcpy(output_filename,filename);
  strcat(output_filename,"-defrag");
  //printf("input filename is %s, output_filename is %s\n",filename, output_filename);
  fp_r = fopen(filename, "r");
  fp_w = fopen(output_filename,"wb+");
  fp_test = fopen("data_first","wb+");
  //fp_w = fopen(output_filename,"wb+");
  if(fp_r == NULL) {
    printf("the file can not be opened\n");
    return 1;
  }
  fseek(fp_r, 0, SEEK_END);
  size_t size = ftell(fp_r);
  fseek(fp_r, 0, SEEK_SET);
  //size_t input_size = fread(file_buffer, 1, size,fp_r);
  //create output file
  /*
  int fd = open(output_filename, O_RDWR | O_CREAT | O_APPEND);
  if(fd < 0) {
    printf("Create file fails\n");
    return 1;
  }
  ftruncate(fd,input_size);
  struct stat st;
  fstat(fd, &st);
  printf("the file has %ld bytes\n", (long) st.st_size);
  close(fd);
  */
  //open it again in binary form
  //fp_w = fopen(output_filename,"wb+");  
  //printf("the size of fp is %ld\n", input_size);
  //malloc necessary global
  file_buffer = malloc(sizeof(char) * size + 1);
  write_buffer = malloc(sizeof(char) * DEFAULT_BUFFER);
  size_t input_size = fread(file_buffer, 1, size,fp_r);
  //get information in super block
  sp = (Superblock*)(file_buffer + H_UNIT);

  //initialize global variable
  block_size = sp->size;
  inode_head = (Inode*)(file_buffer + H_UNIT * 2 + sp->inode_offset * block_size);
  write_counter = 0;
  inode_start = TOTAL_HEAD + sp->inode_offset * block_size;
  inode_end = TOTAL_HEAD + sp->data_offset * block_size;
  inode_num = (inode_end - inode_start) / INODE_SIZE;
  data_start = (size_t)inode_end;
  data_end = TOTAL_HEAD + sp->swap_offset * block_size;
  num_indir_idx = block_size/sizeof(int);
  //then we copy the inode and superblock to our own buffer, convenient to modify
  new_super = malloc(sizeof(char) * H_UNIT);
  new_i_region = malloc(sizeof(char) * (inode_end - inode_start));
  memcpy(new_super, file_buffer+H_UNIT, H_UNIT);
  memcpy(new_i_region, file_buffer+TOTAL_HEAD, (inode_end - inode_start));
  Inode* new_inode_head = (Inode*)(new_i_region);
  Superblock* new_sp = (Superblock*)(new_super);
  // write boot block, old superblock and inode to new file
  fwrite(file_buffer,inode_end, 1, fp_w);
  data_addr_w = inode_end;
  data_index_w = 0;
  //print the information in superblock
  printf("the block size is %d\n",sp->size);
  printf("the inode_offset is %d\n",sp->inode_offset);
  printf("the data_offset is %d\n",sp->data_offset);
  printf("the swap_offset is %d\n",sp->swap_offset);
  printf("the first free node is %d\n", sp->free_inode);
  printf("the first free block is %d\n", sp->free_block);
  printf("the inode region starts at %d\n", inode_start);
  printf("the inode region ends at  %d\n", inode_end);
  Inode* source_cur = inode_head;
  Inode* new_cur = new_inode_head;
  int num_node = 0;
  for(int i = 0; i < inode_num; i ++) {
    //if the source inode is not free, we arrange it
    if(source_cur->next_inode == 0){
      //update infor in new_cur
      //new_cur->next_inode = 0;
      if(i != num_node)
	update_inode(source_cur,new_cur);
      arrange_data(source_cur, new_cur);
      new_cur = next_inode(new_cur);
      num_node++;
    }
    source_cur = next_inode(source_cur);
  }
  //here we need to clean the rest inode in new inode list
  clean_inode(new_cur,num_node);
  //write in the last part of file
  if(w_buffer_ptr != 0) {
    write_in();
  }
  //we need to update superblock and update in the file
  new_sp->free_inode = num_node;
  new_sp->free_block = data_index_w;
  fseek(fp_w, H_UNIT, SEEK_SET);
  fwrite(new_super,H_UNIT,1,fp_w);
  //overwrite new i_node list
  fwrite(new_i_region, (inode_end-inode_start),1,fp_w);
  //put the free block in
  write_in_free();
  //copy the original swap region
  char* swap_region = (char*)(file_buffer + data_end);
  printf("the input size is %ld, and the swap_region starts at %ld\n",input_size, data_end);
  if(data_end < (long)input_size){
    fwrite(swap_region, ((long)input_size - data_end), 1, fp_w);
  }
  //fwrite(swap_region, (input_size - data_end), 1, fp_w);
  //test whether things are ok in file
  fseek(fp_w,H_UNIT,SEEK_SET);
  fread(new_super,H_UNIT,1,fp_w);
  fread(new_i_region, (inode_end-inode_start),1,fp_w);
  //print the information in new superblock
  Superblock* tmp = (Superblock*) new_super;
  printf("the block size is %d\n",tmp->size);
  printf("the inode_offset is %d\n",tmp->inode_offset);
  printf("the data_offset is %d\n",tmp->data_offset);
  printf("the swap_offset is %d\n",tmp->swap_offset);
  printf("the first free node is %d\n", tmp->free_inode);
  printf("the first free block is %d\n", tmp->free_block);
  new_cur = (Inode*) new_i_region;
  for(int i = 0; i < inode_num; i++) {
    print_inode(new_cur,i,TRUE);
    new_cur = next_inode(new_cur);
  }
  //get the content of data and write into another file
  //fp_test = fopen("data_second","wb");
  //char* tmp_buffer = malloc(input_size - inode_end);
  //fseek(fp_w, data_start, SEEK_SET);
  //fread(tmp_buffer, (input_size-inode_end), 1, fp_w);
  //fwrite(tmp_buffer, (input_size-inode_end), 1, fp);
  /*
  printf("###################print old inode list####################\n ");
  Inode* cur = inode_head;
  for(int i = 0; i < inode_num; i++) {
    print_inode(cur,i,TRUE);
    cur = next_inode(cur);
  }
  */
  clean_up();
  fclose(fp_r);
  fclose(fp_w);
  fclose(fp_test);
  return 0;
}

//write in the data from write buffer to fp_w
void write_in_f() {
  //move to next available byte
  fseek(fp_w,data_addr_w,SEEK_SET);
  fwrite(write_buffer, w_buffer_ptr, 1, fp_w);
  data_addr_w += w_buffer_ptr;
  w_buffer_ptr = 0; //reset the pointer of write buffer
}

void write_in() {
  //write in test file
  //int test_addr_w = data_addr_w - inode_end;
  //fseek(fp_test, test_addr_w, SEEK_SET);
  fwrite(write_buffer, w_buffer_ptr, 1, fp_test);
  //move to next available byte
  fseek(fp_w,data_addr_w,SEEK_SET);
  fwrite(write_buffer, w_buffer_ptr, 1, fp_w);
  data_addr_w += w_buffer_ptr;
  w_buffer_ptr = 0; //reset the pointer of write buffer
}
//copy the Direct block content to write in buffer
int Dblock_copy(int index) {
  if(w_buffer_ptr == DEFAULT_BUFFER) {
    write_in();
  }
  size_t data_address = data_start + index*block_size;
  //printf("the address we found is %ld\n",data_address);
  //memcpy(test, (void*)data_address, block_size);
  for (int i = 0; i < block_size; i++) {
    write_buffer[w_buffer_ptr++] = file_buffer[data_address+i];
  }
  return data_index_w++;
}
//copy the index block of indirect block into buffer
int get_index(int indir_idx, int internal_idx) {
  size_t data_address = data_start + indir_idx*block_size;
  int* index_table = (int*)(file_buffer + data_address);
  return index_table[internal_idx];
}
//copy the index array into the buffer
int index_copy(int* idx_arr) {
  if(w_buffer_ptr == DEFAULT_BUFFER){
    write_in();
  }
  
  for(int i = 0; i < num_indir_idx; i++) {
   memcpy(write_buffer+w_buffer_ptr, &idx_arr[i], sizeof(int));
   //printf("Writing %d...\n", idx_arr[i]);
   w_buffer_ptr += sizeof(int); 
  }
  return data_index_w++;
}


void arrange_data(Inode* source_inode, Inode* new_inode) {
  int remain = source_inode->size;
  
  //arrange the direct blocks
  printf("******************Rearranging direct blocks*******************\n");
  for(int i = 0; i < N_DBLOCKS ; i++) {
    new_inode->dblocks[i] = Dblock_copy(source_inode->dblocks[i]);
    remain = remain - block_size;
    if(remain <= 0)
      return;
  }

  //arrange indirect blocks
  //arrange first level indirect blocks
  printf("*********************Rearranging indirect blocks************************\n");
  for(int i = 0; i < N_IBLOCKS; i++) {
    int index_arr[num_indir_idx];
    bzero(index_arr,sizeof(index_arr));
    for(int j = 0; j < num_indir_idx ;j++) {
      //get the index of the data block we want
      int b_index = get_index(source_inode->iblocks[i],j);
      if(b_index <=0 || b_index > sp->swap_offset) {
	printf("Abnormal event, get access to invalid index block!\n");
	new_inode->iblocks[i] = index_copy(index_arr);
	return;
      }
      else {
	index_arr[j] = Dblock_copy(b_index);
	remain = remain - block_size;
	if(remain <= 0) {
	  new_inode->iblocks[i] = index_copy(index_arr);
	  return;
	}
      }
    }
    new_inode->iblocks[i] = index_copy(index_arr);
  }

  //arrange second level indirect block
  int first_indir_arr[num_indir_idx];
  bzero(first_indir_arr,sizeof(first_indir_arr));
  for(int i = 0; i < num_indir_idx; i++) {
    //get the block index of first level indirect block
    int f_index = get_index(source_inode->i2block,i);
    if(f_index <=0 || f_index > sp->swap_offset) {
      printf("Abnormal event, get access to invalid first level indirect block!\n");
      new_inode->i2block = index_copy(first_indir_arr);
      return;
    }
    //array to store data block
    int index_arr[num_indir_idx];
    bzero(index_arr,sizeof(index_arr));
    for(int j = 0; j < num_indir_idx ;j++) {
      //get the index of the data block we want
      int b_index = get_index(f_index,j);
      if(b_index <=0 || b_index > sp->swap_offset) {
        printf("Abnormal event, get access to invalid index block!\n");
        first_indir_arr[i] = index_copy(index_arr);
	new_inode->i2block = index_copy(first_indir_arr);
	return;
      }
      else {
        index_arr[j] = Dblock_copy(b_index);
        remain = remain - block_size;
        if(remain <= 0) {
          first_indir_arr[i] = index_copy(index_arr);
	  new_inode->i2block = index_copy(first_indir_arr);
          return;
        }
      }
    }
    first_indir_arr[i] = index_copy(index_arr);
  }
  new_inode->i2block = index_copy(first_indir_arr);

  //arrange triple level indirect block
  int second_indir_arr[num_indir_idx];
  bzero(second_indir_arr,sizeof(second_indir_arr));
  for(int k = 0; k < num_indir_idx; k++) {
    int s_index = get_index(source_inode->i3block,k);
    if(s_index <=0 || s_index > sp->swap_offset) {
      printf("Abnormal event, get access to invalid second level indirect block!\n");
      new_inode->i3block = index_copy(second_indir_arr);
      return;
    }
    int first_indir_arr[num_indir_idx];
    bzero(first_indir_arr,sizeof(first_indir_arr));
    for(int i = 0; i < num_indir_idx; i++) {
      //get the block index of first level indirect block
      int f_index = get_index(s_index,i);
      if(f_index <=0 || f_index > sp->swap_offset) {
	printf("Abnormal event, get access to invalid first level indirect block!\n");
	second_indir_arr[k] = index_copy(first_indir_arr);
	new_inode->i3block = index_copy(second_indir_arr);
	return;
      }
      //array to store data block
      int index_arr[num_indir_idx];
      bzero(index_arr,sizeof(index_arr));
      for(int j = 0; j < num_indir_idx ;j++) {
	//get the index of the data block we want
	int b_index = get_index(f_index,j);
	if(b_index <=0 || b_index > sp->swap_offset) {
	  printf("Abnormal event, get access to invalid index block!\n");
	  first_indir_arr[i] = index_copy(index_arr);
	  second_indir_arr[k] = index_copy(first_indir_arr);
	  new_inode->i3block = index_copy(second_indir_arr);
	  return;
	}
	else {
	  index_arr[j] = Dblock_copy(b_index);
	  remain = remain - block_size;
	  if(remain <= 0) {
	    first_indir_arr[i] = index_copy(index_arr);
	    second_indir_arr[k] = index_copy(first_indir_arr);
	    new_inode->i3block = index_copy(second_indir_arr);
	    return;
	  }
	}
      }
      first_indir_arr[i] = index_copy(index_arr);
    }
    second_indir_arr[k] = index_copy(first_indir_arr);
  }
  new_inode->i3block = index_copy(second_indir_arr);
}

void update_inode(Inode* source, Inode* new) {
  new->next_inode = 0;
  new->protect = source->protect;
  new->nlink = source->nlink;
  new->size = source->size;
  new->uid = source->uid;
  new->gid = source->gid;
  new->ctime = source->ctime;
  new->mtime = source->mtime;
  new->atime = source->atime;
  for(int i = 0; i < N_DBLOCKS; i++) {
      new->dblocks[i] = 0;
  }
  for(int i = 0; i < N_IBLOCKS; i++) {
      new->iblocks[i] = 0;
  }
  new->i2block = 0;
  new->i3block = 0;
}

void clean_inode(Inode* rest, int index) {\
  for(int i = index; i < inode_num; i++) {
    rest->next_inode = index + 1;
    rest->protect = 0;
    rest->nlink = 0;
    rest->size = 0;
    rest->uid = 0;
    rest->gid = 0;
    rest->ctime = 0;
    rest->mtime	= 0;
    rest->atime	= 0;
    for(int i = 0; i < N_DBLOCKS; i++) {
      rest->dblocks[i] = 0;
    }
    for(int i = 0; i < N_IBLOCKS; i++) {
      rest->iblocks[i] = 0;
    }
    rest->i2block = 0;
    rest->i3block = 0;
    rest = next_inode(rest);
  }
}
/*
void Iblock_copy(size_t remain, int index) {
  if(w_buffer_ptr == DEFAULT_BUFFER) {
    write_in();
  }
  //we just need to take care one indirect block, and deal with                                             
  //first we copy in the index in indirect block                                                            
  int index_arr[num_indir_idx];
  bzero(index_arr);
  for(int i = 0; i < num_indir_idx ; i++) {
  }
}
*/
void clean_up() {
  free(file_buffer);
  free(new_super);
  free(new_i_region);
  free(write_buffer);
}

void write_in_free() {
  int free_block_start = data_index_w + (inode_end-TOTAL_HEAD)/block_size;
  printf("the data end is at :%d, the free block starts at: %d\n", sp->swap_offset, data_index_w);
  for(int i = free_block_start; i < sp->swap_offset; i++) {
    if(w_buffer_ptr == DEFAULT_BUFFER){
      write_in_f();
    }
    
    fb* new_bf = malloc(sizeof(fb));
    bzero(new_bf, sizeof(fb));
    if(i == sp->swap_offset -1){
      new_bf->next_free = -1;
    }
    else {
      new_bf->next_free = i - inode_end/block_size + 1;
    }
    void* buffer_ptr = (void*)(write_buffer + w_buffer_ptr);
    memcpy(buffer_ptr, new_bf, block_size);
    w_buffer_ptr += block_size;
    free(new_bf);
  }
  if(w_buffer_ptr != 0)
    write_in_f();
}
