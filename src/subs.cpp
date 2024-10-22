#include <functional>
#include <memory>
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

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
using std::placeholders::_1;
using namespace std::chrono_literals;

#define RT_PERIOD_MS 1 //1msec
#define CPU_AFFINITY_NUM 1 // 쓰레드를 특정 CPU코어에 고정 -> 어떤 코어를 사용할지 선택 가능

pthread_t rt;
pthread_mutex_t data_mut = PTHREAD_MUTEX_INITIALIZER; // main thread와 데이터 겹치지 않게 만들어주기-> stack영역의 데이터 변경 불가


 // cpu 코어당 상용량 확인 : top , 1
 double cnt = 0 ;
 
 int sigMainKill = 0;
 static void cleanup(void);

class HelloworldSubscriber : public rclcpp::Node
{
public:
  HelloworldSubscriber()
  : Node("Helloworld_subscriber")
  {
    auto qos_profile = rclcpp::QoS(rclcpp::KeepLast(1)).reliability(RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT);
    // auto qos_profile = rclcpp::QoS(rclcpp::KeepLast(1)).deadline(rclcpp::Duration(100ms));
    
    helloworld_subscriber_ = this->create_subscription<std_msgs::msg::String>(
      "helloworld",
      qos_profile,
      std::bind(&HelloworldSubscriber::subscribe_topic_message, this, _1));
    clock_gettime(CLOCK_MONOTONIC, &expected);
  //RT thread 내부에서 작동하는 timer 생성
    tfd = timerfd_create(CLOCK_MONOTONIC, 0); 
    if(tfd == -1) error(1, errno, "timerfd_create()");

    timer_conf.it_value = expected; //from now
    timer_conf.it_interval.tv_sec = 0;
    timer_conf.it_interval.tv_nsec = RT_PERIOD_MS*1000000; //interval with RT_PERIOD_MS

    //timer 설정 : read함수의 return 주기 : sampling time
    // int err = timerfd_settime(tfd, TFD_TIMER_ABSTIME, &timer_conf, NULL); 
        
    if(err) error(1, errno, "timerfd_setting()");

    t1 = 0;
    old_t1 = 0;
    delta_t1 = 0;
    sampling_ms = 0;
    // looptime = 0;
    // looptime_old = 0;
  }

private:
  
  void subscribe_topic_message(const std_msgs::msg::String::SharedPtr msg) //const
  {
    (void) msg;
    clock_gettime(CLOCK_REALTIME, &trt);
    // looptime_old = trt.tv_nsec; 
    // (void) msg;
    old_t1 = t1;
    t1 = trt.tv_nsec;
    delta_t1 = t1 - old_t1;
    sampling_ms = (double)delta_t1*0.000001;
    double jitter = sampling_ms - 1.000; 
    
    // if(ticks>1) overrun += ticks - 1; 
    if(jitter >1) overrun +=1;
    
    // RCLCPP_INFO(this->get_logger(), "Received message: '%s' , cnt = '%f'", msg->data.c_str(), cnt);
    cnt += 0.001;
    // clock_gettime(CLOCK_REALTIME, &trt);
    // looptime = trt.tv_nsec;
    // double duration = (double)(looptime - looptime_old);
    printf("PERIODIC TIME --- %.4f, Jitter --- %+.4f, OVERRUN --- %d , Received message: '%s'\r\n", sampling_ms, jitter, overrun, msg->data.c_str());
    // printf("loop time = %f\r\n", duration);

  }
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr helloworld_subscriber_;
  struct timespec trt; //
  long t1 ;
  // long looptime ;
  // long looptime_old ;
  long old_t1;
  long delta_t1;
  double sampling_ms;
  int tfd;
  struct itimerspec timer_conf; //
  struct timespec expected; //
  int err;
  
  uint32_t overrun = 0;
  uint64_t ticks;
};


// C 스타일의 함수로 전달할 함수를 정의
void *realtime_thread(void *arg)
{
    auto node_ptr2 = static_cast<std::shared_ptr<HelloworldSubscriber>*>(arg);
   
    rclcpp::spin(*node_ptr2);


    pthread_exit(NULL); //while loop 종료 -> thread 종료
    return NULL;

    
}

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  
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

    auto node = std::make_shared<HelloworldSubscriber>();
    ret = pthread_create(&rt, &rtattr, realtime_thread, &node); //create RT thread
    if(ret) error(1, ret, "pthread_create(realtime_thread)");
    pthread_attr_destroy(&rtattr);
    while(cnt<9);

    usleep(2000);
    cleanup();
  rclcpp::shutdown();
  return 0;
}

 static void cleanup(void)
 {
     pthread_cancel(rt);
     pthread_join(rt, NULL);
 }
