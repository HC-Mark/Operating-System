#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>

int read_intr() {
  FILE* fp;
  int result = 0;
  char c;
  //https://stackoverflow.com/questions/15738029/how-do-i-use-a-file-as-a-paramete\
r-for-a-function-in-c                                                              
  fp = fopen("/proc/stat","r");
  while(c != 'r') {
    c = fgetc(fp);
    if( feof(fp) ) {
      break ;
    }
  }
  c = fgetc(fp);
  while((c = fgetc(fp)) >= 48 && c <= 57) {
    result = result * 10 + c - 48;
  }
  fclose(fp);

  return result;
}

void sig_handler(int num) {
  //printf("receive the signal %d.\n", num);                                       
  //alarm(1);
}

int main(int argc, char** argv) {
  //error handling
  if(argc == 1 || argc == 3) {
    //questoin: do we have other flag other than -s?
  //get first interrupts number and store in current                               
  //open proc/stat and read                                                        
  int num_intr_p;
  int num_intr_c;
  int prev = 0;
  num_intr_c = read_intr();                     
  //knowing whether we receive signal
  signal(SIGALRM, sig_handler); //we need to have signal() even we don't need to use it to print anything. it is necessary for clock to work normally
  //get the interval from console
  int interval;
  if(argc == 3) {
    if(atoi(argv[2]) == 0 || strcmp(argv[1],"-s") !=0){
      printf("invalid command line parameters, please try again.\n");
      exit(1);
    }else
      interval = atoi(argv[2]);
  }
  else
    interval = 1;
  //printf("user wants to have interval: %d\n",interval);
  //set up itimerval: https://www.cnblogs.com/dandingyy/archive/2012/08/23/2653218.html
  struct itimerval tick;
  memset(&tick, 0, sizeof(tick)); //how to free this thing? it needs to be run forever before we type ctl-c

  tick.it_value.tv_sec = interval;
  tick.it_value.tv_usec = 0;

  tick.it_interval.tv_sec = interval;
  tick.it_interval.tv_usec = 0;
  
  //alarm(1);
  if(setitimer(ITIMER_REAL, &tick, NULL) < 0)
      printf("Set timer failed!\n");
  
  while(1){
    pause();
    num_intr_p = num_intr_c;
  //get current interrupts number                                                  
    num_intr_c = read_intr();
  //prev = current - last                                                          
    prev = num_intr_c - num_intr_p;
  //print two value                                                                
    printf("number of interrupts in the previous %d second(s): %d\n",interval, prev);
    printf("total number of interrupts since boot: %d\n", num_intr_c);
  }

  exit(0);

  }//exit normally
  else {
    printf("illegal input for hw1b, please type ./hw1b or ./hw1b -s (int)x.\n");
    exit(1);
  }
}
