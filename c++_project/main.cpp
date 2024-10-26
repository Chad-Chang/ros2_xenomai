#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> //C POSIX lib header. Header for accessing to the POSIX OS API
#include <fcntl.h> //C POSIX lib header. Header for opening and locking files and processing other tasks.
#include <signal.h> //Header for signal processing
#include <sys/timerfd.h> //
#include <string.h>
#include <malloc.h> //Memory allocation
#include <pthread.h> //Header for using Thread operation from xenomai
#include <error.h> //
#include <errno.h> //Header for defining macros for reporting and retrieving error conditions using the symbol 'errno'
#include <sys/mman.h> //
#include <rtdm/ipc.h> //
#include <iostream>

#define RT_PERIOD_MS 1 //1msec
#define CPU_AFFINITY_NUM 0 // 쓰레드를 특정 CPU코어에 고정

pthread_t rt; 
pthread_mutex_t data_mut = PTHREAD_MUTEX_INITIALIZER;

int sigMainKill = 0;
static void cleanup(void);
static void *realtime_thread(void *arg);



int main(int argc, char *argv[])
{
    mlockall(MCL_CURRENT|MCL_FUTURE);
    pthread_attr_t rtattr;
    sigset_t set;
    int sig;
    cpu_set_t cpus;

    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    sigaddset(&set, SIGHUP);
    pthread_sigmask(SIG_BLOCK, &set, NULL); // thread로 시그널을 보낸다?
    struct sched_param p;
    int ret; 

    ret = pthread_attr_init(&rtattr); // pthread attribute 시작
    if(ret) error(1, ret, "pthread_attr_int()");

    ret = pthread_attr_setinheritsched(&rtattr, PTHREAD_EXPLICIT_SCHED); //내부적으로 세팅된 pthread 스케쥴링을 외부적으로 바꿀수 있게 해줌.
    if(ret) error(1, ret, "pthread_attr_setinheritsched()");

    ret = pthread_attr_setschedpolicy(&rtattr, SCHED_FIFO); //pthread scheduling policy setting as FIFO
    if(ret) error(1, ret, "pthread_attr_setschedpolicy()");

    p.sched_priority = 99;
    ret = pthread_attr_setschedparam(&rtattr, &p); //setting scheduler parameter - priority 99 (Highest)
    if(ret) error(1, ret, "pthread_attr_setschedparam()");

    CPU_ZERO(&cpus);
    CPU_SET(CPU_AFFINITY_NUM, &cpus);
    ret = pthread_attr_setaffinity_np(&rtattr, sizeof(cpus), &cpus); //give cpu affinity to be used to calculate for the RT thread
    if(ret) error(1, ret, "pthread_attr_setaffinity_np()");

    ret = pthread_create(&rt, &rtattr, realtime_thread, NULL); //create RT thread
    if(ret) error(1, ret, "pthread_create(realtime_thread)");
    
    pthread_attr_destroy(&rtattr); //delete pthread attribute union
    while (1);
    sigMainKill = 1;
    usleep(2000);
    cleanup(); 
}
//main thread가 종료되면 확실히 종료될때까지 기다리는 exec()함수 실행 
//gui exit 되면 return.
// exec() 전 rt 실행, exec()이후 RT thread 종료


static void cleanup(void)
{
    pthread_cancel(rt);
    pthread_join(rt, NULL);
}

static void *realtime_thread(void *arg)
{
    struct timespec trt; //
    long t1 = 0;
    long old_t1 = 0;
    long delta_t1 = 0;
    double sampling_ms = 0;
    int tfd;
    struct itimerspec timer_conf; //
    struct timespec expected; //

    clock_gettime(CLOCK_MONOTONIC, &expected); //get the system's current time

    tfd = timerfd_create(CLOCK_MONOTONIC, 0); //create timer descriptor
    if(tfd == -1) error(1, errno, "timerfd_create()");

    timer_conf.it_value = expected; //from now
    timer_conf.it_interval.tv_sec = 0;
    timer_conf.it_interval.tv_nsec = RT_PERIOD_MS*1000000; //interval with RT_PERIOD_MS

    int err = timerfd_settime(tfd, TFD_TIMER_ABSTIME, &timer_conf, NULL); //set the timer descriptor
    if(err) error(1, errno, "timerfd_setting()");

    uint32_t overrun = 0;
    uint64_t ticks;
    
    while(!sigMainKill)
    {
        std::cout <<"helloworld"<<std::endl;
        err = read(tfd, &ticks,sizeof(ticks));
        clock_gettime(CLOCK_REALTIME, &trt);
        if(err<0) error(1,errno, "read()");

        old_t1 = t1;
        t1 = trt.tv_nsec;
        delta_t1 = t1 - old_t1;
        sampling_ms = (double)delta_t1*0.000001;

        double jitter = sampling_ms - 1.000;

        if(ticks>1) overrun += ticks - 1; 

        if(!pthread_mutex_trylock(&data_mut))
        {
            pthread_mutex_unlock(&data_mut);
        }
        printf("PERIODIC TIME --- %.4f, Jitter --- %+.4f, OVERRUN --- %d/r/n", sampling_ms, jitter, overrun);
    }
    pthread_exit(NULL);
    return NULL;
}