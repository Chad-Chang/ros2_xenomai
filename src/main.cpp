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

 class HelloworldPublisher : public rclcpp::Node
 {
 public:
   HelloworldPublisher()
   : Node("helloworld_publisher"), count_(0)
   {
     auto qos_profile = rclcpp::QoS(rclcpp::KeepLast(10));
     helloworld_publisher_ = this->create_publisher<std_msgs::msg::String>(
       "helloworld", qos_profile);
     timer_ = this->create_wall_timer(
       1s, std::bind(&HelloworldPublisher::publish_helloworld_msg, this));
   }

 private:
   void publish_helloworld_msg()
   {
     auto msg = std_msgs::msg::String();
     msg.data = "Hello World: " + std::to_string(count_++);
     RCLCPP_INFO(this->get_logger(), "Published message: '%s'", msg.data.c_str());
     helloworld_publisher_->publish(msg);
   }
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


 // cpu 코어당 상용량 확인 : top , 1
 double cnt = 0 ;


// C 스타일의 함수로 전달할 함수를 정의
void *realtime_thread(void *arg)
{
    auto node_ptr = static_cast<std::shared_ptr<HelloworldPublisher>*>(arg);
    rclcpp::spin(*node_ptr);  // ROS2 노드 실행
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
    while(cnt < 5);

    usleep(2000);
    cleanup();
}

 static void cleanup(void)
 {
     pthread_cancel(rt);
     pthread_join(rt, NULL);
 }

// static void *realtime_thread(void * arg)
// {
//     (void) arg;
//     std::cout << "hello world"<<std::endl;
//     return NULL;
// }