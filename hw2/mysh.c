#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cir_buf.h"

#define MYSH_LINE_BUF 1024
#define MYSH_TOK_BUF 50
#define MYSH_DELI " \t\r\n\a" //get from brennan
#define HISTSIZE 50
//the structure has three part:
//read the line from stdin
//parse the read line into an array of string
//execute that array of string
//check status to see whether we need to exit the shell loop

//the structure of shell inspired by:
//https://brennan.io/2015/01/16/write-a-shell-in-c/

char* mysh_read_line();
char** mysh_parse_line(char* line);
int mysh_execute(char** args);
void mysh_loop();
//global string array to store built in function names
char* built_in_func [] = {"exit","history", NULL};
char built_in_func_sign [] = {'!'};
//history will be defined as global
cir_buf history;
/*= {
  .size = HISTSIZE,
  .buffer = malloc(HISTSIZE * sizeof(char*)),
  .head = 0,
  .tail = 0,
  .empty = true
};
*/
int main() {
  //run the main loop of my shell
  mysh_loop();
  /*
  char* test;
  char** test_args;
  test = mysh_read_line();
  printf("what I have from read line: %s\n",test);
  test_args = mysh_parse_line(test);
  for(int i = 0 ; i < 5; i++) {
    if(test_args[i] == NULL)
      break;
    else
      printf("test whether parsing works: %s\n", test_args[i]);
  }
  */
  exit(0);
}

//https://brennan.io/2015/01/16/write-a-shell-in-c/ 
void mysh_loop() {
  char* line;
  char** arguments;
  int status;

  //initialize history struct
  //deal with environment variable
  char* hist_size = getenv("HISTSIZE");
  if(hist_size)
    history.size = atoi(hist_size);
  else
    history.size = HISTSIZE;
  //printf("history size now is %d\n", history.size);
  history.buffer = malloc(history.size * sizeof(char*));
  cir_buf_init(&history);
  
  do{
    printf("(╯‵□′)╯︵┻━┻:");
    line = mysh_read_line(); 
    arguments = mysh_parse_line(line);
    //printf("test aaaa%saaaaa\n",line);
    cir_buf_put(&history,line);
    //printf("head is %d\n",history.head);
    //printf("tail is %d\n",history.tail);
    /*test
    char* test;
    cir_buf_get_tail(&history, &test);
    int tmp_head = history.head;
    history.head = history.head - 1;
    printf("content is %s, head is %d, tail is %d\n", test, history.head, history.tail);
    history.head = tmp_head;
    */
    //printf("what I have from read line: %s\n",line); 
    status = mysh_execute(arguments);
    printf ("what is status now %d\n",status);
    //if arguments[0] == NULL, it means that no content is read by read_line
    //if(arguments[0] != NULL) {
      //free(line); //free line will fail if I directly input EOF
      free(arguments);
      //}
  }while(status);
  cir_buf_clean(&history);
  free(history.buffer); // here actually we need to have a specific function to clean all char* here, also in put, otherwise we will leak a lot of memory
}

char* mysh_read_line() {
  int buf_size = MYSH_LINE_BUF;
  char* tmp = malloc(buf_size * sizeof(char));
  char c;
  int index = 0;
  //handle unsuccesful malloc
  if(!tmp) {
    fprintf(stderr, "mysh: malloc error\n");
    exit(EXIT_FAILURE);
  }
  //get line
  do {
    c = fgetc(stdin); // what difference between getchar() and fgetc()?
    tmp[index] = c;
    index++;

    //if the buffer is not large enough ,we will enlarge it
    if(index >= buf_size) {
      buf_size = buf_size * 2;
      tmp = realloc(tmp,buf_size * sizeof(char));
      if(!tmp) {
	fprintf(stderr, "mysh: malloc error\n");
	exit(EXIT_FAILURE);
      }
      //strcpy(trans, tmp);
      //free(tmp);
      //tmp = trans;
      //printf("now the buf_size is %d\n", buf_size);
    }
  }while(c != '\n' && c != EOF);
  tmp[index-1] = '\0'; //already be added 1, we don't want that newline be stored
  
  return tmp;
}

char** mysh_parse_line(char* line) {
  int buf_size = MYSH_TOK_BUF;
  char** tokens = malloc(buf_size * sizeof(char*));
  char* token;
  int index = 0;
  if(!tokens) {
    fprintf(stderr, "mysh: malloc error\n");
    exit(EXIT_FAILURE);
  }
  token = strtok(line, MYSH_DELI);
  //printf("what is token aaaa%saaaaa\n",token);
  while( token != NULL) {
    //printf("token is %s\n",token);
    tokens[index] = token;
    index++;
    token = strtok(NULL,MYSH_DELI);
    if(index >= buf_size) {
      buf_size = buf_size * 2;
      tokens = realloc(tokens, buf_size * sizeof(char*));
      if(!tokens) {
        fprintf(stderr, "mysh: realloc error\n");
        exit(EXIT_FAILURE);
      }
      //printf("size is %d\n",buf_size);
    }
  }
  tokens[index] = NULL; //ending array by NULL
  return tokens;
}

//http://www.csl.mtu.edu/cs4411.ck/www/NOTES/process/fork/exec.html
int mysh_foreign (char** args) {
  pid_t pid;
  int status;
  pid = fork();
  //child process
  if(pid == 0) {
    if(execvp(args[0], args) < 0) {
      fprintf(stderr, "ERROR: exec failed\n");
      exit(EXIT_FAILURE);
    }
  }
  else if(pid < 0) {
    fprintf(stderr, "ERROR: fork() failed\n");
    exit(EXIT_FAILURE);
  }
  else {
    while( wait(&status) != pid);
  }
  return 1;
}

//thisfunction will be a giant, need to wrap up in several.
int mysh_builtin (char** args) {
  char* checked = args[0];
  //now we only need to implement exit
  if(strcmp(checked, "exit") == 0)
    return 0;
  //weird thing: if you enter !! before two commands get in, you will get null
  if(checked[0] == '!') {
    //this series of command will not be counted into the history
    //but we still need to free them first
    history.head = history.head - 1;
    char* free_item;
    cir_buf_get_head(&history,&free_item);
    //printf("free item is %s\n",free_item);
    free(free_item);
    //may not work if strtok store the splited token in original string
    //end of free for each ! series command
    if(strcmp(checked, "!!") == 0) {
      if(history.head == 0) {
	fprintf(stderr, "Can not repeat last command if it is the first command, error input\n");
	exit(1);
      }
      int tmp = history.head;
      char* output;
      //printf("head is %d\n",history.head);
      if(history.head - 1 < 0)
	history.head = history.size - (history.head - 1);
      else
	history.head = history.head - 1;
      //printf("head after dealing is %d\n", history.head);
      cir_buf_get_head(&history,&output);
      char* last_command = malloc(strlen(output)+1);
      memcpy(last_command,output,strlen(output)+1);
      //printf("we have last_command is %s\n", last_command);
      //recover to the normal head
      history.head = tmp; //otherwise the repeated command will be overwritten
      //printf("we have %s\n",output);
      //actually execute the found command
      int tmp_status;
      char** tmp_args;
      tmp_args = mysh_parse_line(last_command);
      cir_buf_put(&history,last_command);
      tmp_status = mysh_execute(tmp_args);
      if(tmp_args[0] != NULL)
	free(tmp_args);
      return tmp_status;
    }

    //"!-n" type
    else if(checked[1] == '-'){
      //the valid element number of history                
      int total = history.head - history.tail;
      if (total < 0)
        total = history.size;
      //vulnerble in security 
      char num[strlen(checked)-2];
      strncpy(num, checked+2, strlen(checked)-2);
      int ret =(int)strtol(num, NULL, 10);
      //printf("our target is %ld\n", ret);
      if(ret > total || ret <= 0) {
	fprintf(stderr,"number is too large or too small, can't store that many commands in data base.");
	exit(1);
      }
      //bookkeeping the tail and head 
      int tmp_head = history.head;
      //int tmp_tail = history.tail;
      char* output;
      char** tmp_args;
      int tmp_status;
      //ret need to be one less, since we start at 0
      history.head = (history.head - ret) % history.size;
      cir_buf_get_head(&history,&output);
      //get a new char to store into history, in case of double free
      char* last_command = malloc(strlen(output)+1);
      memcpy(last_command, output, strlen(output)+1);
      //printf("head is %s\n", output);
      history.head = tmp_head;
      tmp_args = mysh_parse_line(last_command);
      cir_buf_put(&history,last_command);
      tmp_status = mysh_execute(tmp_args);
      if(tmp_args[0] != NULL)
	free(tmp_args);
      return tmp_status;
    }

    //"!n" type
    else {
      //the valid element number of history
      int total = history.head - history.tail;
      if (total < 0)
	total = history.size;
      //vulnerble in security
      char num[strlen(checked)-1];
      strncpy(num, checked+1, strlen(checked)-1);
      int ret =(int)strtol(num, NULL, 10);
      //printf("our target is %ld\n", total-ret);
      if(ret > total || ret <= 0) {
	fprintf(stderr,"number is too large or too small, can't store that many commands in data base.");
        exit(1);
      }

      //bookkeeping the tail and head
      int tmp_head = history.head;
      int tmp_tail = history.tail;
      char* output;
      char** tmp_args;
      int tmp_status;
      //ret need to be one less, since we start at 0
      history.tail = (history.tail + ret-1) % history.size;
      cir_buf_get_tail(&history,&output);
      char* last_command = malloc(strlen(output)+1);
      memcpy(last_command,output,strlen(output)+1);
      //printf("tail is %s\n", output);
      history.tail = tmp_tail;
      tmp_args = mysh_parse_line(last_command);
      cir_buf_put(&history,last_command);
      tmp_status = mysh_execute(tmp_args);
      if(tmp_args[0] != NULL)
	free(tmp_args);
      return tmp_status;
    }
      

    
  }
  //I didn't think of that when head = 4, tail = 0, we should put it into the full part and keep track of that.
  if(strcmp(checked, "history") == 0) {
    int print_time = history.head - history.tail;
    int i;
    char* output;
    if(history.full)
      print_time = history.size; //since we will not remove element from history, the only possible situation that head < tail is when the history's buffer is full.
    //bookkeeping the tail
    int tmp_head = history.head;
    int tmp_tail = history.tail;
    if(print_time == history.size) {
      for(i = 0; i < print_time; i++) {
	cir_buf_get_head(&history,&output);
	printf(" %d  %s\n",i+1,output);
	history.head = (history.head + 1) % history.size;
      }
    }
    else {
      for(i = 0; i < print_time; i++) {
        cir_buf_get_tail(&history,&output);
        printf(" %d  %s\n",i+1,output);
        history.tail = (history.tail + 1) % history.size;
      }
    }
    //when it meets head, it will stop and no longer print, so we print the last one manually
    //and we need to recover tail first, otherwise, get_head will also not work
    //modify head, since now it points to the next empty memory.
    //history.head = history.head - 1;
    history.tail = tmp_tail;
    history.head = tmp_head;
    //cir_buf_get_head(&history, &output);
    //printf(" %d  %s\n",i+1, output);

  }
  return 1;
}

bool mysh_find_builtin (char** args) {
  int index = 0;
  char* checked = args[0];
  //assume the only the first argument can be built in command
  while(built_in_func[index] != NULL) {
    if(strcmp(checked, built_in_func[index]) == 0)
      return true;
    index++;
  }
  index = 0;
  while(built_in_func_sign[index]) {
    if(checked[0] == built_in_func_sign[index])
      return true;
    index++;
  }
  return false;

}

int mysh_execute(char** args) {
  if (args[0] == NULL)
    return 1;
  else if(mysh_find_builtin (args)) {
    //printf("in builtin\n");
    return mysh_builtin(args);
  }
  else{
    //printf("in foreign\n");
    return mysh_foreign(args);
  }
}
