/*
cooperate with Jiaping Wang, Zhanpeng Wang, Linyi Chen and Ziting Shen
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <string.h>
#include <sys/file.h>

#define SIZE 1024
#define ERROR_SIZE 50

void s_handler(int s_num) {
  write(1,"Helps, I think I've been shot!!!\n",ERROR_SIZE);
  exit(1);
}
//can not be terminated by control d, so it won't be terminated normally ---done
void cat_stdin() {
    char tmp[10] = "";
    while(read(0,tmp,1) > 0){
	if(write(1, tmp, 1) < 0) {
	  write(1,"output error for some reasons\n", ERROR_SIZE);
	  exit(1);
	}
    }
}

int main(int argc, char** argv) {
  pid_t pid = getpid();
  int fd,size;
  //handle signal
  for(int sig_num = 1; sig_num < 31; sig_num++)
  signal(sig_num,s_handler);
  
  if( argc < 2) {
    cat_stdin();
  }
  else {
    //handle redirection
    if(argc >= 4 && strcmp(argv[argc-2],">") == 0) {
      //we need to add mode 0666. Otherwise the file will be write-protected
      //printf("in redirection\n");
      
      //using flag to identify whether output file is also one of the input file, if so, we will deal with it specifically.
      int flag = 0;
      for(int i = 1; i < argc-2; i++) {
	if(strcmp(argv[i],argv[argc-1]) == 0) {
	  flag = i;
	  break;
	}
      }
      //start redirecting file
      int output = open(argv[argc-1], O_WRONLY| O_CREAT |O_TRUNC,0666);
      if(output < 0) {
	write(1,"oepn() failed with error.\n",ERROR_SIZE);
	exit(1);
      }
      for(int i = 1; i < argc-2; i++) {
	if(i == flag)
	  continue;
	if (strcmp(argv[i],"-") == 0) {
	  char tmp[10] = "";
	  while(read(0,tmp,1) > 0){
	    if(write(output, tmp, 1) < 0) {
	      write(1,"output error for some reasons\n", ERROR_SIZE);
	      exit(1);
	    }
	  }
	  continue;
	}
        fd = open(argv[i], O_RDONLY);
        if(fd < 0) {
          write(1,"open() failed with error.\n",ERROR_SIZE);
          exit(1);
        }
        int file_size = lseek(fd,0,SEEK_END);
	//        printf("the size of file is %d\n", file_size);
        lseek(fd,0,SEEK_SET);
        char buffer[file_size+1];
        size = read(fd, buffer, file_size+1);
	if (close(fd) < 0) {
          write(1,"can't close file normal\n",ERROR_SIZE);
          exit(1);
	}
	//printf("what is argv[argc-1]: %s",argv[argc-1]);
        if(write(output, buffer, file_size+1) < 0) {
          write(1,"output error for some reasons\n",ERROR_SIZE);
          exit(1);
        }	
      }
      //close output file                                   
        if (close(output) < 0) {
          write(1,"can't close file normal\n",ERROR_SIZE);
          exit(1);
	}
      exit(0);
    }//redirection part ends
    
    else{//handle normal print
    for (int i = 1; i < argc; i++) {
      if (strcmp(argv[i],"-") == 0) {
	cat_stdin();
	exit(0);
      }
      fd = open(argv[i], O_RDONLY);
      if(fd < 0) {
	write(1,"oepn() failed with error.\n",ERROR_SIZE);
	exit(1);
      }
      int file_size = lseek(fd,0,SEEK_END);
      //      printf("the size of file is %d\n", file_size);
      lseek(fd,0,SEEK_SET);
      char buffer[file_size+1];
      size = read(fd, buffer, file_size+1);
      buffer[size] = '\0';
      //printf("Those bytes are as follows: % s\n", str);
      
      if(write(1, buffer, file_size+1) < 0) {
	write(1,"output error for some reasons\n",ERROR_SIZE);
	exit(1);
      }
      
      //close file
      if (close(fd) < 0) {
	write(1,"can't close file normal\n",ERROR_SIZE);
	exit(1);
      }
    }
    exit(0);
    }
  }
}
