//redir test file -- try to store the screen out put to a file
// CPP program to illustrate dup2() 
#include<stdlib.h>
#include<unistd.h>
#include<stdio.h>
#include<fcntl.h>
#include  <sys/types.h>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
using namespace std;
#define TRUE 1
#define FALSE 0
#define FAIL -1
// check if there is a redirection '>' or ">>" or '<' in the give arguments
// check if the command only has > redirection.
vector<string> split(const string &s, char delim);

int only_right_redirection(char **arg, int size)
{
  for (int i = 1; i < size; i++)
  {
    if (*arg[i] == '<')
      return FALSE;
  }
  return TRUE;
}
string conversion(char **arg, int size) {
	string temp = "";
	for (int i = 1; i < size; i++) {
		temp = temp + " " + arg[i];
	}
	return temp;
}

// check if it is the right redirection ">"
int IsRightRedirection(char **arg, int size)
{
	if(!only_right_redirection(arg, size))
		return FALSE;
	string command = string(conversion(arg, size));
	vector<string> command_string_list = split(command, '>'); // split it by '>'
	if (command_string_list.size() > 1)
		return TRUE;
	return FALSE;
}

char* add_space(char* ptr, size_t size) {
	char* tmp = (char*)malloc(size);
	strcpy(tmp,ptr);
	//printf("tmp is now %s, size is now %ld\n",tmp,size);
	free(ptr);
	return tmp;
}

//redirection only works when the second last element is arrow
int redirection(char** args, int argn) {
	//first parse the input, last one is the input file name
     int file_desc = f_open(string(args[argn-1]), "w");
 //    //printf("the fd is %d\n",file_desc);,
 //    // here the newfd is the file descriptor of stdout (i.e. 1)
 //    dup2(file_desc, 1) ; 
	// char* file_name = args[argn-1];
	// for(int i = 0; i < argn - 2; i++) {
	//  	//printf("this element is %s\n",args[i]);
	//  }
	//transfer the rest of args to string and concat
	//clean the command list
	 //bzero(args[argn-1],strlen(args[argn-1]));
	 //bzero(args[argn-2],strlen(args[argn-2]));
	// strcpy(args[argn-1] "";
	// args[argn-2] = "";
	 string comb = "";
	  for(int i = 0; i < argn - 2; i++) {
	 	comb += string(args[i]);
	 	if(i != argn -3)
	 		comb += " ";
	  }
	 char* content = (char*)(malloc(comb.length() + 1));
	 strcpy(content,comb.c_str());
	printf("the content we have is %s\n",content);
	FILE* fp = popen(content,"r");
	size_t b_size = 10;
	size_t counter = 0;
    char* tmp_buffer = (char*)malloc(b_size);
    char p;
    if(fp==NULL){printf("popen err:%s\n",strerror(errno));exit(1);}
    char* tmp_ptr = tmp_buffer;
    //get the file size of fp
    while((p=fgetc(fp))!=EOF){
        sprintf(tmp_ptr,"%c",p);
        if(counter >= b_size) {
        	b_size = b_size * 2;
        	tmp_buffer = add_space(tmp_buffer,b_size);
        	tmp_ptr = tmp_buffer + b_size/2;
        }
        tmp_ptr++;
        counter++;
    }
    printf("now we have %s\n",tmp_buffer);
	pclose(fp);
	int status = f_write(tmp_buffer,strlen(tmp_buffer)+1,1,file_desc);
	if(status < 0) {
		perror("fail\n");
	}
	f_close(file_desc);
	//this is how we call the argument before the ">"
	// if (pid == 0) {          /* for the child process:         */
	// 	if ((execvp(args[0], args)) == FAIL)
	// 		{
	// 			printf("Didn't execute the command: %s! Either don't know what it is, or it is unavailable. \n", args[0]);
	// 			exit(EXIT_FAILURE);
	// 		}
	// 	}
 //     else {                                  /* for the parent:      */
 //          while (wait(&status) != pid);      /* wait for completion  */
             
	// }
	// close(file_desc);
	return 0;
}

int main(int args, char** argv)
{
    char** sim_list = (char**)(malloc(sizeof(char*) * (args-1)));
    for(int i = 0; i < args -1; i++) {
    	sim_list[i] = argv[i+1];
    	printf("the sim_list is %s\n",sim_list[i]);
    } 
    // FILE* fp = popen("echo hello","r");
    // char* test = (char*)malloc(10);
    // char p;
    // if(fp==NULL){printf("popen err:%s\n",strerror(errno));exit(1);}
    // // while((p=fgetc(fp))!=EOF){
    // //     printf("%c",p);
    // // }
    // fread(test,10,1,fp);
    // printf("test%s\n",test);
    redirection(sim_list,args-1);
    // All the printf statements will be written in the file
    // "tricky.txt"
    //printf("I will be printed in the file tricky.txt\n");
     
return 0;
}

//http://ysonggit.github.io/coding/2014/12/16/split-a-string-using-c.html for split
vector<string> split(const string &s, char delim)
{
	stringstream ss(s);
	string item;
	vector<string> tokens;
	while (getline(ss, item, delim))
	{
		if (item != "")
			tokens.push_back(item);
	}
	return tokens;
}
