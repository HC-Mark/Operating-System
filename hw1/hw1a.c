#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

int read_intr() {
  FILE* fp;
  int result = 0;
  char c;
  //https://stackoverflow.com/questions/15738029/how-do-i-use-a-file-as-a-parameter-for-a-function-in-c
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
  alarm(1);
}

int main() {
  //get first interrupts number and store in current
  //open proc/stat and read
  int num_intr_p;
  int num_intr_c;
  int prev = 0;
  num_intr_c = read_intr();
  //printf("here is the number of interrupt: %d\n", num_intr);
  //knowing whether we receive signal
  signal(SIGALRM, sig_handler);

  alarm(1);

  while(1){
    pause();
    num_intr_p = num_intr_c;     
  //get current interrupts number
    num_intr_c = read_intr();    
  //prev = current - last
    prev = num_intr_c - num_intr_p;
  //print two value
    printf("number of interrupts in the previous second: %d\n", prev);
    printf("total number of interrupts since boot: %d\n", num_intr_c);
  }

  exit(0);

}
