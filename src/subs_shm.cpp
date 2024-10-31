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
#define NUMOFMOTOR 12

using std::placeholders::_1;
using namespace std::chrono_literals;

#define RT_PERIOD_MS 1 //1msec
#define CPU_AFFINITY_NUM 1 // 쓰레드를 특정 CPU코어에 고정 -> 어떤 코어를 사용할지 선택 가능

pthread_t rt;
pthread_mutex_t data_mut = PTHREAD_MUTEX_INITIALIZER; // main thread와 데이터 겹치지 않게 만들어주기-> stack영역의 데이터 변경 불가

#define SHM_NAME "/mcl_420"
#define SHM_SIZE sizeof(SharedData)

// cpu 코어당 상용량 확인 : top , 1
double cnt = 0 ;
static void cleanup(void);

typedef struct {
    int motor_num[NUMOFMOTOR];
    float time_stamp;
    double motor_pos[NUMOFMOTOR];
    double motor_vel[NUMOFMOTOR];
    double ctrl_input[NUMOFMOTOR];
} SharedData;

SharedData *data; 

class HelloworldSubscriber : public rclcpp::Node
{
public:
  HelloworldSubscriber()
  : Node("Helloworld_subscriber")
  {
    auto qos_profile = rclcpp::QoS(rclcpp::KeepLast(1)).reliability(RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT);
    helloworld_subscriber_ = this->create_subscription<std_msgs::msg::String>(
      "helloworld",
      qos_profile,
      std::bind(&HelloworldSubscriber::subscribe_topic_message, this, _1));
  }
private:
  void subscribe_topic_message(const std_msgs::msg::String::SharedPtr msg) //const
  {
    (void) msg;
    // RCLCPP_INFO(this->get_logger(), "Received message: '%s'", msg->data.c_str());
  }
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr helloworld_subscriber_;
};
bool sigMainKill= 0;

uint64_t ticks;
int shm_fd;
void *shm_ptr;
uint32_t overrun = 0;
long t1 = 0;
long ts = 0;
long old_t1= 0;
long delta_t1= 0;
double sampling_ms= 0;
double pos_old = 0;
void *realtime_thread(void *arg)
{

  (void) arg;
  // auto node_ptr2 = static_cast<std::shared_ptr<HelloworldSubscriber>*>(arg);
  // 이거 실행하면 block됨.


/*타이머 설정*/
  int tfd; // clock instance
  struct timespec trt; //
  struct itimerspec timer_conf; //
  struct timespec expected; //
  clock_gettime(CLOCK_MONOTONIC, &expected);

  /*RT thread 내부에서 작동하는 timer 생성*/
  tfd = timerfd_create(CLOCK_MONOTONIC, 0); 
  if(tfd == -1) error(1, errno, "timerfd_create()");

  timer_conf.it_value = expected; //from now
  timer_conf.it_interval.tv_sec = 0;
  timer_conf.it_interval.tv_nsec = RT_PERIOD_MS*1000000; //interval with RT_PERIOD_MS

  //timer 설정 : read함수의 return 주기 : sampling time
  int err = timerfd_settime(tfd, TFD_TIMER_ABSTIME, &timer_conf, NULL); 
  if(err) error(1, errno, "timerfd_setting()");
  
  int num_loop_err=0;


  while(!sigMainKill)
  {  
    clock_gettime(CLOCK_MONOTONIC, &trt);
    cnt += RT_PERIOD_MS*0.001;
    old_t1 = t1;
    t1 = trt.tv_nsec;
    ts = trt.tv_sec;
    delta_t1 = t1 - old_t1;
    sampling_ms = (double)delta_t1*0.000001;
    double jitter = sampling_ms - RT_PERIOD_MS; 

    if(ticks>1) overrun += ticks - 1;
  // if(jitter >1 && old_t1!=0) overrun +=1;

    double curr_time=(double) trt.tv_sec + (trt.tv_nsec/1e6);

    if(trunc(1000*(data->motor_pos[0]-pos_old)) > 1 && pos_old !=0) num_loop_err++;
     
    pthread_mutex_lock(&data_mut);
    // for(int i= 0 ;i <NUMOFMOTOR; i++)
    // {
    //   data->time_stamp = (double) trt.tv_sec + (trt.tv_nsec/1e6);
    //   data->motor_num[i] = i;
    //   data->motor_pos[i] = cnt;
    //   data->motor_vel[i] = cnt;
    //   data->ctrl_input[i] = cnt;
    // }
    printf("read shared memory: phase delay = %+.4f , loop_err = %d || RT test: PERIODIC TIME = %.4f, Jitter --- %+.4f, OVERRUN --- %d\n " ,
          curr_time-data->time_stamp, num_loop_err, sampling_ms, jitter, overrun);
    
    pthread_mutex_unlock(&data_mut);
    // printf("read shared memory: delay = %+.4f, jitter = %+.4f, OVERRUN = %d , motorPos = %f, loop_err = %d\n" ,
    //        curr_time-data->time_stamp ,jitter, overrun, data->motor_pos[0], num_loop_err);
    // pos_old = data->motor_pos[0];

    // printf("PERIODIC TIME --- %.4f, Jitter --- %+.4f, OVERRUN --- %d \r\n", sampling_ms, jitter, overrun);
  // if(!pthread_mutex_trylock(&data_mut))
  //   {
  //       pthread_mutex_unlock(&data_mut);
  //   }
    err = read(tfd, &ticks,sizeof(ticks)); // RT를 유지해주는 놈.
  }  // ret = pthread_create(&rt, &rtattr, realtime_thread, NULL); //create RT thread
  munmap(shm_ptr, SHM_SIZE);
  close(shm_fd);
  pthread_exit(NULL); //while loop 종료 -> thread 종료

  return NULL;
}

//공유메모리 세팅은 반드시 main문에서 선언해야함.
int main(int argc, char * argv[])
{
  // (void) argc;  (void) argv; 
  
  mlockall(MCL_CURRENT|MCL_FUTURE); 

/*공유 메모리 설정*/
  // 공유 메모리 열기
  shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
  if (shm_fd == -1) {
      perror("shm_open");
      exit(EXIT_FAILURE);
  }
  // 공유 메모리 매핑
  shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  if (shm_ptr == MAP_FAILED) {
      perror("mmap");
      exit(EXIT_FAILURE);
  }
  data = (SharedData *)shm_ptr;
  // 공유 메모리에서 구조체 읽기

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
  // rclcpp::spin(node);
  ret = pthread_create(&rt, &rtattr, realtime_thread, &node); //create RT thread
  // ret = pthread_create(&rt, &rtattr, realtime_thread, NULL); //create RT thread
  if(ret) error(1, ret, "pthread_create(realtime_thread)");
  
  pthread_attr_destroy(&rtattr);
  while(data->motor_pos[0] < 10.0001);
  printf("rt ending");

  sigMainKill =1 ;
  usleep(1000);
  cleanup();
  // rclcpp::shutdown();
  return 0;
}

 static void cleanup(void)
 {
    // shm_unlink(SHM_NAME);
    pthread_cancel(rt);
    pthread_join(rt, NULL);
     
 }
