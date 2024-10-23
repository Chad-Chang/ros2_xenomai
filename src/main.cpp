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

#define SHM_NAME "/mcl_420"
#define SHM_SIZE sizeof(SharedData)

typedef struct {
    int motor_num;
    float time_stamp;
    float motor_pos;
    float load_pos;
    float motor_vel;
    float load_vel;
} SharedData;

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
    helloworld_publisher_ = this->create_publisher<std_msgs::msg::String>(
      "helloworld", qos_profile);
  //  timer_ = this->create_wall_timer(
  //    1s, std::bind(&HelloworldPublisher::publish_helloworld_msg, this));
  }
  
void publish_helloworld_msg()
{
  auto msg = std_msgs::msg::String();
  msg.data = " " + std::to_string(count_++);
  helloworld_publisher_->publish(msg);
  // RCLCPP_INFO(this->get_logger(), "Published message: '%s' and real count = %f", msg.data.c_str(),cnt);
}
private:
  
  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr helloworld_publisher_;
  size_t count_;
};

//

#define RT_PERIOD_MS 1 //1msec
#define CPU_AFFINITY_NUM 0 // 쓰레드를 특정 CPU코어에 고정 -> 어떤 코어를 사용할지 선택 가능

pthread_t rt;
pthread_mutex_t data_mut = PTHREAD_MUTEX_INITIALIZER; // main thread와 데이터 겹치지 않게 만들어주기-> stack영역의 데이터 변경 불가


int sigMainKill = 0;
static void cleanup(void);
//  static void *realtime_thread(void *arg);


/*loop time측정용임.*/
long t1 = 0;
long ts = 0;
long old_t1= 0;
long delta_t1= 0;
double sampling_ms= 0;
int tfd= 0;
int err;
int shm_fd;
void *shm_ptr;
uint64_t ticks;
uint32_t overrun = 0;

/*공유 메모리 변수*/
SharedData data;

// C 스타일의 함수로 전달할 함수를 정의
void *realtime_thread(void *arg) // ros2의 통신을 사용할때는 argument를 instance를 가져와서 사용함.
{
  
  /*ros2통신 사용할때 on*/ 
    // auto node_ptr2 = static_cast<std::shared_ptr<HelloworldPublisher>*>(arg); 
    // rclcpp::spin(*node_ptr);  // ROS2 노드 실행 => 노드가 메시지, 서비스, 타이머
    // return NULL;

  /*공유 메모리로 통신할때*/
  (void) arg;  
  int tfd; // desired timer
  struct timespec trt; // 실제 타이머
  struct itimerspec timer_conf; //
  struct timespec expected; //
  clock_gettime(CLOCK_MONOTONIC, &expected); 
  tfd = timerfd_create(CLOCK_MONOTONIC, 0); //create timer descriptor
  if(tfd == -1) error(1, errno, "timerfd_create()");

  //타이머 configuration 설정
  timer_conf.it_value = expected; //from now
  timer_conf.it_interval.tv_sec = 0;
  timer_conf.it_interval.tv_nsec = RT_PERIOD_MS*1000000; //interval with RT_PERIOD_MS
  
  // 타이머 세팅 instance에 적용
  int err = timerfd_settime(tfd, TFD_TIMER_ABSTIME, &timer_conf, NULL); //set the timer descriptor
  if(err) error(1, errno, "timerfd_setting()");

  
  while(!sigMainKill) // cnt가 어느정도 이상되면 main에서 sigMainkill을 true로 바꿔줌
  {
    // clock_gettime(CLOCK_REALTIME, &trt); //get the system time
    err = read(tfd, &ticks,sizeof(ticks));  //해당 타이머가 설정한 간격으로 발생한 타이머 틱수를 읽어옴.
    clock_gettime(CLOCK_MONOTONIC, &trt); //get the system time
    /*
    1. 타이머 이벤트 대기 : timer된 sampling마다 타이머가 생존하고, 이후에 타이머가 만료됨-> 만료될때까지 block함. 
    만료되면 읽기 작업 완료 -> ticks값에 타이머 만료된 횟수 저장
    2. 타이머가 한번이상 만료되면 일정시간 동안 처리 실패-> 오버런.
    3. 정확한 주기 보장*/ 
    old_t1 = t1;
    t1 = trt.tv_nsec;
    ts = trt.tv_sec;
    delta_t1 = t1 - old_t1;
    sampling_ms = (double)delta_t1*0.000001;
    double jitter = sampling_ms - 1.000; 

    if(ticks>1) overrun += ticks - 1; 
    // if(jitter >1) overrun +=1; // 차이가 1ms이상

    /*통신으로 설정할 때*/
    // (*node_ptr2)->publish_helloworld_msg();

    cnt +=0.001;
    
    if(err<0) error(1,errno, "read()");

    data.motor_num = 1;
    data.motor_pos = cnt;
    data.time_stamp = (double) trt.tv_sec + (trt.tv_nsec/1e9);
    memcpy(shm_ptr, &data, sizeof(SharedData));

  /*shared memory 전달한 값 확인 */
    // printf("Data written to shared memory: motor_num=%d, motor_pos=%.4f, time_stamp = %.4f\n",
    //       data.motor_num, data.motor_pos, data.time_stamp);
  /* 루프타임 확인 */
    printf("PERIODIC TIME --- %.4f, Jitter --- %+.4f, OVERRUN --- %d \r\n", sampling_ms, jitter, overrun);
    
    if(!pthread_mutex_trylock(&data_mut))
    {
        pthread_mutex_unlock(&data_mut);
    }
  }
  
  // 매핑 해제 및 공유 메모리 닫기
  munmap(shm_ptr, SHM_SIZE);
  close(shm_fd);

  pthread_exit(NULL); //while loop 종료 -> thread 종료
  return NULL;

    
}

int main(int argc, char *argv[])
{
    mlockall(MCL_CURRENT|MCL_FUTURE); 
    // 공유 메모리 생성 및 열기
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    // 공유 메모리 크기 설정
    if (ftruncate(shm_fd, SHM_SIZE) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    // 공유 메모리 매핑
    shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap");
        // SharedData data;
        exit(EXIT_FAILURE);
    }   

    // (void) argc; (void) argv;
    rclcpp::init(argc, argv);
    // mlockall(MCL_CURRENT|MCL_FUTURE);
    
    /*xenomai RT thread 만들기*/
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
    /*끝*/
    sigMainKill = 1;
    usleep(1000);
    cleanup();
    rclcpp::shutdown();
  return 0;
}

 static void cleanup(void)
 {
     pthread_cancel(rt);
     pthread_join(rt, NULL);
 }

