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

 #include <chrono>
 #include <functional>
 #include <memory>
 #include <string>

 #include "rclcpp/rclcpp.hpp"
 #include "std_msgs/msg/string.hpp"
 using namespace std::chrono_literals;

 // cpu 코어당 상용량 확인 : top , 1
 double cnt = 0 ;

 class HelloworldPublisher : public rclcpp::Node
 {
 public:
   HelloworldPublisher()
   : Node("helloworld_publisher"), count_(0)
   {
     auto qos_profile = rclcpp::QoS(rclcpp::KeepLast(1)).reliability(RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT);
    //  auto qos_profile = rclcpp::QoS(rclcpp::KeepLast(1), rmw_qos_profile_sensor_data);//.reliability(RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT);
     helloworld_publisher_ = this->create_publisher<std_msgs::msg::String>(
       "helloworld", qos_profile);
    //  timer_ = this->create_wall_timer(
    //    1s, std::bind(&HelloworldPublisher::publish_helloworld_msg, this));
   }
  void publish_helloworld_msg()
  {
    auto msg = std_msgs::msg::String();
    msg.data = "Cello Worldsadfasdfasdfasfadsfasdfasdfasdfasdfasfasdfsadf sdafasdfa sdf asdfasf asdf asdf asdf asdf asdf asdf asdf asdf asdf asdf adfsadfa sdf asdf asdf asdf asdf asdf wef eq vfevczxcvaf verfvsad: " + std::to_string(count_++);
    helloworld_publisher_->publish(msg);
    // RCLCPP_INFO(this->get_logger(), "Published message: '%s' and real count = %f", msg.data.c_str(),cnt);
  }
 private:
   
   rclcpp::TimerBase::SharedPtr timer_;
   rclcpp::Publisher<std_msgs::msg::String>::SharedPtr helloworld_publisher_;
   size_t count_;
 };


 #define RT_PERIOD_MS 1 //1msec
 #define CPU_AFFINITY_NUM 0 // 쓰레드를 특정 CPU코어에 고정 -> 어떤 코어를 사용할지 선택 가능

 pthread_t rt;
 pthread_mutex_t data_mut = PTHREAD_MUTEX_INITIALIZER; // main thread와 데이터 겹치지 않게 만들어주기-> stack영역의 데이터 변경 불가


 int sigMainKill = 0;
 static void cleanup(void);
//  static void *realtime_thread(void *arg);




long t1 = 0;
long old_t1= 0;
long delta_t1= 0;
double sampling_ms= 0;
int tfd= 0;
int err;



// C 스타일의 함수로 전달할 함수를 정의
void *realtime_thread(void *arg)
{
    auto node_ptr2 = static_cast<std::shared_ptr<HelloworldPublisher>*>(arg);
    // rclcpp::spin(*node_ptr);  // ROS2 노드 실행
    // return NULL;

    int tfd; // desired timer
    struct timespec trt; // 실제 타이머
    struct itimerspec timer_conf; //
    struct timespec expected; //
    clock_gettime(CLOCK_MONOTONIC, &expected);
    tfd = timerfd_create(CLOCK_MONOTONIC, 0); //create timer descriptor
    if(tfd == -1) error(1, errno, "timerfd_create()");
    timer_conf.it_value = expected; //from now
    timer_conf.it_interval.tv_sec = 0;
    timer_conf.it_interval.tv_nsec = RT_PERIOD_MS*1000000; //interval with RT_PERIOD_MS
    // 타이머의 interval time을 설정해줌 => timer configuration을 설정해주는 부분 - nano sec단위로 해주나봄.
    int err = timerfd_settime(tfd, TFD_TIMER_ABSTIME, &timer_conf, NULL); //set the timer descriptor
    if(err) error(1, errno, "timerfd_setting()");


    uint64_t ticks;
    uint32_t overrun = 0;
    while(!sigMainKill)
    {
      old_t1 = t1;
      t1 = trt.tv_nsec;
      delta_t1 = t1 - old_t1;
      sampling_ms = (double)delta_t1*0.000001;
      double jitter = sampling_ms - 1.000; 

      // if(ticks>1) overrun += ticks - 1; 
      if(jitter >1) overrun +=1;

      (*node_ptr2)->publish_helloworld_msg();
      cnt +=0.001;
      err = read(tfd, &ticks,sizeof(ticks));  // 이게 RT를 유지해주는 놈임.
      clock_gettime(CLOCK_REALTIME, &trt); //get the system time
      
      if(err<0) error(1,errno, "read()");

      printf("PERIODIC TIME --- %.4f, Jitter --- %+.4f, OVERRUN --- %d \r\n", sampling_ms, jitter, overrun);
      
      if(!pthread_mutex_trylock(&data_mut))
      {
          pthread_mutex_unlock(&data_mut);
      }
    }
    pthread_exit(NULL); //while loop 종료 -> thread 종료
    return NULL;

    
}

int main(int argc, char *argv[])
{
    
    // (void) argc; (void) argv;
    rclcpp::init(argc, argv);
    // mlockall(MCL_CURRENT|MCL_FUTURE);
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
    
    auto node_ptr = std::make_shared<HelloworldPublisher>();

    ret = pthread_create(&rt, &rtattr, realtime_thread, &node_ptr); //create RT thread
    if(ret) error(1, ret, "pthread_create(realtime_thread)");
    pthread_attr_destroy(&rtattr);
    while(cnt < 10);
    sigMainKill = 1;
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

