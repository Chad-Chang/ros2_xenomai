#include <stdio.h>
#include <iostream>
#include <algorithm>
#include <numeric>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/timerfd.h>
#include <string.h>
#include <malloc.h>
#include <pthread.h>
#include <fcntl.h>
#include <error.h>
#include <errno.h>
#include <sys/mman.h>
#include <rtdm/ipc.h>
#include <math.h>
#include <inttypes.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <termios.h>
#include <826api.h>
#include <math.h>
#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#include <chrono>

#include "s826DAQ.hpp"
#include "ethercat.h"
#include "ecat_func.hpp"
#include <iostream>

#define  NUMOFMOTOR 12
using namespace std::chrono_literals;
using namespace std;
#define RT_PERIOD_MS 1 //1msec
#define CPU_AFFINITY_NUM 0 // 쓰레드를 특정 CPU코어에 고정 -> 어떤 코어를 사용할지 선택 가능
pthread_t rt;
pthread_mutex_t data_mut = PTHREAD_MUTEX_INITIALIZER; // main thread와 데이터 겹치지 않게 만들어주기-> stack영역의 데이터 변경 불가

int sigMainKill = 0;

static void cleanup(void);


int main(int argc, char *argv[])
{
    (void) argc;(void) argv;
    mlockall(MCL_CURRENT|MCL_FUTURE);
    std:: cout<< "ascqc"<<std::endl;
    pthread_attr_t rtattr;
    sigset_t set;
    cpu_set_t cpus;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    sigaddset(&set, SIGHUP);
    pthread_sigmask(SIG_BLOCK, &set, NULL);
    struct sched_param p;
    int ret;
    ret = pthread_attr_init(&rtattr);
    if(ret) error(1, ret, "pthread_attr_int()");
    ret = pthread_attr_setinheritsched(&rtattr, PTHREAD_EXPLICIT_SCHED);
    if(ret) error(1, ret, "pthread_attr_setinheritsched()");
    ret = pthread_attr_setschedpolicy(&rtattr, SCHED_FIFO);
    if(ret) error(1, ret, "pthread_attr_setschedpolicy()");
    p.sched_priority = 99;
    ret = pthread_attr_setschedparam(&rtattr, &p);
    if(ret) error(1, ret, "pthread_attr_setschedparam()");
    CPU_ZERO(&cpus);
    CPU_SET(CPU_AFFINITY_NUM, &cpus);
    ret = pthread_attr_setaffinity_np(&rtattr, sizeof(cpus), &cpus); //give cpu affinity to be used to calculate for the RT thread
    if(ret) error(1, ret, "pthread_attr_setaffinity_np()");

}


 static void cleanup(void)
 {
     pthread_cancel(rt);
     pthread_join(rt, NULL);
 }
