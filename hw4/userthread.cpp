/* implement a user-space-level-thread library
   Collaborater: Zhanpeng Wang, Elieen Feng
   cite: https://github.com/xxlelementlxx/Thread-Library/blob/master/thread.cc
         https://github.com/ManojkumarMuralidharan/User-level-Thread-library
 */
#include <cstdlib>
#include <ucontext.h>
#include <iterator>
#include <map>
#include <iostream>
#include <queue>
#include <signal.h>
#include <vector>
#include <fstream>
#include <sys/time.h>
#include <ctime>
//#include <valgrind.h>
#include "userthread.h"

#define FAILED -1
#define SUCCEED 0
#define FALSE 0
#define TRUE 1
#define HIGH -1
#define MID 0
#define LOW 1
#define RECORD_NUM 100 //for sjf
#define QUANTUM 100000 //ms but probably need to convert to us
#define MILLION 1000000
using namespace std;

enum {SCHEDULED, STOPPED, CREATED, FINISHED};
//enum type {FIFO, SJF, PRIORITY};
//remember to let all the functions to be static, prevent from overwriting
static bool init = false;
static int id = 0;
static int type = -1;
static ucontext_t* scheduler;
static char* sch_stack;
static int total_thread = 0;
static int total_runtime = 0;
static int priority; //keep track of priority of thread
static ofstream logfile;
//for keeping track of time
static struct timeval tv;
static time_t lib_begin;
static time_t cur_time;
static struct itimerval clocktick;
static sigset_t signal_set;
//typedef void (*func)(void *); //seems a better way to represent void (*func)(void *)
struct mythread {
  //general member
  int tid;
  int state; // scheduled or stopped or finished
  int type; // type of scheduling algorithm
  ucontext_t* uc;
  char* stack; //can't use void*
  
  //for priority schedule
  int priority; //for priority type only

  //for shortest job first schedule
  time_t last_start;
  vector <double> last_run_times; // the last three run times
  double expected_time;
  
  // needs to change to all threads waiting for me
  vector <int>  wait_tids; //actual waiting threads' tid
  int wait_size; //how many threads are waiting for this thread
};

//helper to build up priority queue
struct comp {
  bool operator()(const mythread* t1, const mythread* t2) {
    return t1->expected_time > t2->expected_time;
  }
};

//declare global variable
static mythread* main_thread;
static mythread* current;
static queue<mythread*> fifoq;

static priority_queue<mythread*, vector<mythread*>, comp> sjfq;

static vector<mythread*> first, second , third; //for priority policy

static vector <mythread*> suspended;

//helper function to delete a specific thread
static void delete_thread(mythread* cur);
static void dispatcher(); //scheduler function
static int stub(void (*func) (void*), void* arg); //stub function
static void catcher(int sig); // signal handler for priority scheduling
static void protect();
static void release();

/*
// --own test part 
void test(void* trash) {
  cout << "hello, world" <<endl;
}

void more_test(void* useful) {
  
}
int main(int args, char** argv) {
  if(thread_libinit(SJF) == 0)
    cout << "Ya ho!" << endl;
  else
    cout << "Sad.. I fail" << endl;

  priority_queue <mythread*, vector<mythread*>, comp> Q;
  thread_create(test,NULL,0);
  thread_create(test,NULL,0);
  mythread* tmp; //= fifoq.front();
  //fifoq.push(tmp);
  tmp = fifoq.front(); // this should be test
  cout << "tmp is " << tmp->tid << endl;
  //setcontext(tmp->uc);
  Q.push(tmp);
  fifoq.pop();
  tmp = fifoq.front();
  Q.push(tmp);
  cout << "queue size is now " << Q.size() << endl;
  tmp = Q.top();
  cout << "the head has tid " << tmp->tid << endl;
  cout << "head has expected time " << tmp->expected_time << endl;
  //thread_yield();
  //thread_join(1);
  //cout << "size of queue is now: " << fifoq.size() << endl;
  if(thread_libterminate() == 0)
    cout << "lib term Ya ho!" << endl;
  else
    cout << "Sad.. I fail" << endl;

  //mythread* kk = fifoq.front();
  //makecontext(kk->uc, test, 0);
  //setcontext(kk->uc);
  return 0;
}
*/


int thread_libinit(int policy) {
  //can't double init thread lib
  if(init)
    return FAILED;
  init = true;
  type = policy;
  if(type != FIFO && type != SJF && type != PRIORITY)
    return FAILED;
  //initialize the time of library
  gettimeofday(&tv, NULL);
  lib_begin = tv.tv_usec + tv.tv_sec*MILLION;
  //create a log file here, but ignore currently
  
  mythread* main;
  try{
    main = new mythread;
    //init the thread member
    //store main thread into
    main -> uc = new ucontext_t;
    getcontext(main->uc);
    main -> stack = new char[STACKSIZE];
    main->uc->uc_stack.ss_sp = main->stack;
    main->uc->uc_stack.ss_size = STACKSIZE;
    main->uc->uc_stack.ss_flags = SS_DISABLE;
    main->uc->uc_link = NULL;
    main->tid = id;
    main->wait_size = 0;
    id++;
    main->state = CREATED;
    current = main; // update current
    main_thread = main;
    //need to also input the priority of main thread
    if(type == PRIORITY){
      cout << "we are in PRIORITY" << endl;
      //initialize the random generator
      srand(time(NULL));
      //call the signal func
      signal(SIGALRM, catcher);
      clocktick.it_value.tv_sec = 0;
      clocktick.it_value.tv_usec = QUANTUM;
      clocktick.it_interval.tv_sec = 0;
      clocktick.it_interval.tv_usec = QUANTUM;
      sigemptyset(&signal_set);
      sigaddset(&signal_set,SIGALRM);
    }
    else if(type == SJF) {
      cout << "we are in SJF" << endl;
    }
    else {

    }
      //depends on the policy
      //fifoq.push(main);
      //probably we should not push main thread initially in the queue.
    //then initialize the schduler
    cout<< "everything ok?" <<endl;
    scheduler = new ucontext_t;
    getcontext(scheduler);
    sch_stack = new char[STACKSIZE];
    scheduler->uc_stack.ss_sp = sch_stack;
    scheduler->uc_stack.ss_size = STACKSIZE;
    scheduler->uc_stack.ss_flags = SS_DISABLE;
    scheduler->uc_link = NULL;
    makecontext(scheduler, (void (*)())dispatcher, 0);
  }catch(std::bad_alloc b){
    //if we fail to malloc, clean up everything
    delete main->uc;
    delete[] main->stack;
    delete main;
    return FAILED;
  }
  logfile.open("logfile.txt");
  logfile <<"Note: the main thread is thread with TID 0" << endl;
  return SUCCEED;
}

int thread_libterminate(void){
  if(!init)
    return FAILED;
  //depends on the policy
  if(type == FIFO) {
    while(fifoq.size() >0) {
      current = fifoq.front();
      delete_thread(current);
      fifoq.pop();
    }
  }
  else if(type == SJF) {
    while(sjfq.size() >0) {
      current = sjfq.top();
      delete_thread(current);
      sjfq.pop();
    }
  }
  else{
    while(first.size() >0) {
      current = first.back();
      delete_thread(current);
      first.pop_back();
    }
    while(second.size() >0) {
      current = second.back();
      delete_thread(current);
      second.pop_back();
    }
    while(third.size() >0) {
      current = third.back();
      delete_thread(current);
      third.pop_back();
    }
  }
  //clean other stuffs
  while(!suspended.empty()) {
    //mythread* tmp = suspended.back();
    //delete_thread(tmp);
    //suspended.pop_back();
    suspended.clear();
  }
  //clean scheduler
  delete[] sch_stack;
  delete scheduler;
  //clean main
  if(main_thread->uc != NULL && main_thread-> stack != NULL && main_thread != NULL){
    delete[] main_thread->stack;
    delete main_thread->uc;
    delete main_thread;
  }
  //close the file descrpitor of log file

  return SUCCEED;
}
//return tid of the created thread
int thread_create(void (*func)(void *), void *arg, int priority) {
  //test whether be initialize
  if(!init)
    return FAILED;

  mythread* tmp;
  try {
    tmp = new mythread;
    
    tmp->uc = new ucontext_t;
    getcontext(tmp->uc);
    
    tmp->stack = new char [STACKSIZE];
    tmp->uc->uc_stack.ss_sp = tmp->stack;
    tmp->uc->uc_stack.ss_size = STACKSIZE;
    tmp->uc->uc_stack.ss_flags = SS_DISABLE;
    tmp->uc->uc_link = NULL;
    tmp->wait_size = 0;
    makecontext(tmp->uc, (void (*)())stub,2,func,arg);
    
    protect();
    tmp->tid = id;
    id++;
    release();
    tmp->state = CREATED;
    //add to different queue
    if(type == FIFO) {
      fifoq.push(tmp); 
    }
    else if(type == SJF) {
      if(total_thread == 0)
	tmp->expected_time = QUANTUM / 2 + tmp->tid; // we need to have a global variable counting expected time
      else
	tmp->expected_time = total_runtime/total_thread;

      sjfq.push(tmp);
    }
    else {
      tmp->priority = priority;
      if(priority == HIGH) {
	       first.push_back(tmp);
      }
      else if(priority == MID) {
	       second.push_back(tmp);
      }
      else {
	       third.push_back(tmp);
      }
    }
  }
  catch (std::bad_alloc b) {
    delete tmp->uc;
    delete tmp->stack;
    delete tmp;
    return FAILED;
  }
  return tmp->tid;
}

int thread_yield(void) {
  if(!init) {
    printf("Thread library is not initialized, please initialize before using it\n");
    return FAILED;
  }
  if(current->tid == 0){
    makecontext(scheduler,dispatcher,0);
    swapcontext(current->uc, scheduler);
    return SUCCEED;
  }
  if(type == FIFO) {
    fifoq.push(current);
    printf("queue size is %d\n", fifoq.size());
    makecontext(scheduler,dispatcher,0); //update scheduler
    swapcontext(current->uc, scheduler);
  }
  else if(type == SJF) {
    //assume the last_start is when the thread run
    gettimeofday(&tv,NULL);
    double interval;
    interval = tv.tv_usec + tv.tv_sec*MILLION - current->last_start;
    current->last_run_times.push_back(interval);
    //cout << "run time in yield is " << interval << endl;
    if(current->last_run_times.size() < 3)
      current->expected_time = current->last_run_times.back();
    else {
      int result = 0;
      for(int i = 0; i < 3; i++) {
	result += current->last_run_times[current->last_run_times.size()-1-i];
      }
      current->expected_time = result / 3;
    }
    total_thread ++;
    total_runtime += interval;
    cout << "expected time of " << current->tid << " is " << current->expected_time << endl;
    sjfq.push(current);
    makecontext(scheduler,dispatcher,0);
    swapcontext(current->uc, scheduler);
  }
  //priority scheduling
  else {
    protect();
    if(current->priority == HIGH) {
      first.push_back(current);
    }
    else if(current->priority == MID) {
      second.push_back(current);
    }
    else {
      third.push_back(current);
    }
    release();
    makecontext(scheduler,dispatcher,0); //update scheduler                
    swapcontext(current->uc, scheduler);
  }
  return SUCCEED;
}
int thread_join(int tid) {
  int q_size;
  if(!init) {
    printf("Thread library is not initialized, please initialize before using it\n");
    return FAILED;
  }
  if(tid > id) {
    printf("waiting for an invalid thread\n");
    return FAILED;
  }
  /*
  if(current->tid == 0) {
    makecontext(scheduler,dispatcher,0); //update scheduler
    swapcontext(current->uc, scheduler);
  }
  */
  if(type == FIFO) {
    q_size = fifoq.size();
    //vector <mythread*> tmp;
    for(int i = 0; i < q_size;i++) {
      mythread* cur = fifoq.front();
      //if we find out the thread we want to wait
      if(cur->tid == tid) {
	cur->wait_tids.push_back(current->tid);
	cur->wait_size++;
      }
      fifoq.pop();
      fifoq.push(cur);
    }
    suspended.push_back(current);
    current->state = STOPPED;
    printf("suspended size is %d\n", suspended.size());
    makecontext(scheduler,dispatcher,0); //update scheduler
    swapcontext(current->uc, scheduler);
  }
  else if(type == SJF) {
    q_size = sjfq.size();
    vector <mythread*> tmp;
    for(int i = 0; i < q_size;i++) {
      mythread* cur = sjfq.top();
      //if we find out the thread we want to wait
      if(cur->tid == tid) {
	cur->wait_tids.push_back(current->tid);
	cur->wait_size++;
	break;
      }
      tmp.push_back(cur);
      sjfq.pop();
    }
    //restore the elements
    while(tmp.size() > 0) {
      mythread* tmpo = tmp.back();
      sjfq.push(tmpo);
      tmp.pop_back(); 
    }
    //update time
    gettimeofday(&tv,NULL);
    cur_time = tv.tv_sec*MILLION + tv.tv_usec;
    time_t run_time = cur_time - current->last_start;
    //cout << "run time in join is " << run_time << endl;
    current->last_run_times.push_back(run_time);
    if(current->last_run_times.size() < 3)
      current->expected_time = current->last_run_times.back();
    else {
      int result = 0;
      for(int i = 0; i < 3; i++) {
	result += current->last_run_times[current->last_run_times.size()-1-i];
      }
      current->expected_time = result / 3;
    }
    total_thread++;
    total_runtime += run_time;
    cout << "expected time of " << current->tid << " is " << current->expected_time << endl;
    suspended.push_back(current);
    current->state = STOPPED;
    printf("suspended size is %d\n", suspended.size());
    makecontext(scheduler,dispatcher,0); //update scheduler
    swapcontext(current->uc, scheduler);
  }
  else {
    int f_size = first.size();
    int s_size = second.size();
    int t_size = third.size();
    bool flag = false;
    //vector <mythread*> tmp;                                                   
    for(int i = 0; i < f_size;i++) {
      mythread* cur = first[i];
      //if we find out the thread we want to wait                              
      if(cur->tid == tid) {
        cur->wait_tids.push_back(current->tid);
        cur->wait_size++;
	flag = true;
      }
    }
    if(flag == false) {
      for(int i = 0; i < s_size;i++) {
	mythread* cur = second[i];
	//if we find out the thread we want to wait                              
	if(cur->tid == tid) {
	  cur->wait_tids.push_back(current->tid);
	  cur->wait_size++;
	  flag = true;
	}
      }
      if(flag == false) {
	for(int i = 0; i < t_size;i++) {
	  mythread* cur = third[i];
	  //if we find out the thread we want to wait
	  if(cur->tid == tid) {
	    cur->wait_tids.push_back(current->tid);
	    cur->wait_size++;
	  }
	}
      }
    }
    suspended.push_back(current);
    current->state = STOPPED;
    //printf("suspended size is %d\n", suspended.size());
    makecontext(scheduler,dispatcher,0); //update scheduler                   
    swapcontext(current->uc, scheduler);
  }
  return SUCCEED;
}

void delete_thread(mythread* cur) {
  delete[] cur->stack;
  cur->uc->uc_stack.ss_sp = NULL;
  cur->uc->uc_stack.ss_size = 0;
  cur->uc->uc_stack.ss_flags = 0;
  cur->uc->uc_link = NULL;
  delete cur->uc;
  delete cur;
  cur = NULL;
}
//use global variable current to keep track of the thread we work on
static void dispatcher() {
  //cout<< "in scheduler" << endl;
  //all policy need to update current context to its uc 
  //getcontext(current->uc);

  if(setitimer(ITIMER_REAL, &clocktick, NULL) < 0)
    printf("Set timer failed!\n");
  
  mythread* tmp;
  //find the appropriate thread to run
  if(type == FIFO) {
    if(fifoq.size() == 0) {
      cout << "warning fifo queue is empty!" << endl;
      swapcontext(scheduler, main_thread->uc);
    }

    //revive suspended threads. -- need to move into each type later
    if(current->state == FINISHED){
      cout << "#############thread " << current->tid << " finished successfully###################### "<< endl; 
      if(!current->wait_tids.empty()) {
	     for(int i = 0; i < current->wait_size ; i++) {
	       for(int j = 0; j < suspended.size() ; j++) {
	         if(current->wait_tids[i] == suspended[j]->tid) {
	           fifoq.push(suspended[j]);
	           suspended.erase(suspended.begin() + j);
	           cout << "size of suspended after moving " << current->wait_tids[i] << " is " << suspended.size() <<endl;
	           break;
	    }
	  }
	}
	current->wait_tids.clear(); //clean its vector
      }
      delete_thread(current);
  }
    tmp = fifoq.front();
    fifoq.pop(); 
    current = tmp;
    printf("current queue size is %d\n", fifoq.size());
    printf("we will run thread %d\n", current->tid);
    makecontext(scheduler, dispatcher, 0);
    swapcontext(scheduler, current->uc);
  }
  else if(type == SJF) {
    if(sjfq.size() == 0) {
      cout << "warning sjf queue is empty!" << endl;
      swapcontext(scheduler, main_thread->uc);
    }

    //revive suspended threads. -- need to move into each type later
    if(current->state == FINISHED){
      cout << "#############thread " << current->tid << " finished successfully###################### "<< endl;
      if(!current->wait_tids.empty()) {
	for(int i = 0; i < current->wait_size ; i++) {
	  for(int j = 0; j < suspended.size() ; j++) {
	    if(current->wait_tids[i] == suspended[j]->tid) {
	      sjfq.push(suspended[j]);
	      suspended.erase(suspended.begin() + j);
	      break;
	    }
	  }
	}
	current->wait_tids.clear(); //clean its vector
      }
      delete_thread(current);
  }
    tmp = sjfq.top();
    sjfq.pop(); 
    current = tmp;
    printf("current sjf queue size is %d\n", sjfq.size());
    printf("we will run thread %d\n", current->tid);
    makecontext(scheduler, dispatcher, 0);
    swapcontext(scheduler, current->uc);
  }
  else {
    if(first.empty() && second.empty() && third.empty()) {
      cout << "warning all three queues are empty!" << endl;
      protect();
      swapcontext(scheduler, main_thread->uc);
    }

    
    //revive suspended threads. -- need to move into each type later            
    if(current->state == FINISHED){
      cout << "#############thread " << current->tid << " finished successfully###################### "<< endl; 
      if(!current->wait_tids.empty()) {
        for(int i = 0; i < current->wait_size ; i++) {
          for(int j = 0; j < suspended.size() ; j++) {
            if(current->wait_tids[i] == suspended[j]->tid) {
              protect();
	      int pt = suspended[j] -> priority;
	      if(pt == -1)
		first.push_back(suspended[j]);
	      else if(pt == 0)
		second.push_back(suspended[j]);
	      else
		third.push_back(suspended[j]);
              suspended.erase(suspended.begin() + j);
              release();
              //cout << "size of suspended after moving " << current->wait_tids[i] << " is " << suspended.size() <<endl;
              break;
            }
          }
        }
        current->wait_tids.clear(); //clean its vector                          
      }
      delete_thread(current);
  }
    //use random generator to decide who is the next
    int tester = rand() % 18;
    int temp_pri;
    cout << "tester is " << tester << " now." << endl;
    if(tester < 9){
      temp_pri = HIGH;
      if(first.empty()){
	if(second.empty()){
	  temp_pri = LOW;
	}
	else
	  temp_pri = MID;
      }
    }
    else if(tester >= 9 && tester <= 14) {
      temp_pri = MID;
      if(second.empty()) {
	if(first.empty())
	  temp_pri = LOW;
	else
	  temp_pri = HIGH;
      }
    }
    else {
      temp_pri = LOW;
      if(third.empty()){
	if(first.empty()){
	  temp_pri = MID;
	}
	else
	  temp_pri = HIGH;
      }
    }
    //what if higher priority queue is empty -- move to next level
    protect();
    if(temp_pri == HIGH) {
      tmp = first.front();
      first.erase(first.begin());
    }
    else if(temp_pri == MID) {
      tmp = second.front();
      second.erase(second.begin());
    }
    else {
      tmp = third.front();
      third.erase(third.begin());
    }
    release();
    current = tmp;
    //printf("current queue size is %d\n", fifoq.size());
    printf("we will run thread %d, its prirority is %d\n", current->tid, current->priority);
    makecontext(scheduler, dispatcher, 0);
    swapcontext(scheduler, current->uc);
  }
}

static int stub(void (*func)(void *), void *arg) {
  time_t tmp_before, tmp_after, run_time;
  gettimeofday(&tv,NULL);
  tmp_before = tv.tv_sec*MILLION + tv.tv_usec;
  if(type == SJF)
    current->last_start = tmp_before;
  cout << "expected time is " << current->expected_time << endl;
  func(arg); //execute the function

  gettimeofday(&tv,NULL);
  tmp_after = tv.tv_sec*MILLION + tv.tv_usec;
  run_time = tmp_after - tmp_before;
  //cout << "run time in stub is " << run_time << endl;
  total_thread++;
  total_runtime += run_time;

  current->state = FINISHED;
  makecontext(scheduler, dispatcher,0);
  swapcontext(current->uc, scheduler);
  return SUCCEED;
}

static void catcher(int sig) {
  //cout << "current thread has priority " << current->priority << endl;
  thread_yield();
}

static void protect() {
  sigprocmask(SIG_BLOCK, &signal_set, NULL);
}

static void release() {
  sigprocmask(SIG_UNBLOCK, &signal_set, NULL);
}