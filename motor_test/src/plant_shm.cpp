#ifndef PLANT_HPP_
#define PLANT_HPP_

#include <algorithm>
#include <numeric>
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
#include <utility>

#include "ecat_func.hpp"
#include "controller.hpp"
#include "trajectory.hpp"
#include "plant_shm.hpp"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "motor_commu/msg/mcl_actuator.hpp"


using namespace std::chrono_literals;
using MclActuator = motor_commu::msg::MclActuator;

#define RT_PERIOD_MS 1 //1msec
#define CPU_AFFINITY_NUM 1 // 쓰레드를 특정 CPU코어에 고정 -> 어떤 코어를 사용할지 선택 가능
#define NUMOFMOTOR 12
#define PI 3.141592

pthread_t rt;  
pthread_mutex_t data_mut = PTHREAD_MUTEX_INITIALIZER;



uint16 RXPDO_ADDR_GTWI[3] = {2, 0x1600, 0x1605};  
uint16 TXPDO_ADDR_GTWI[4] = {3, 0x1A02, 0x1A03, 0x1A1E};


typedef struct {
    float time_stamp;
    int motor_num[NUMOFMOTOR];
    double motor_pos[NUMOFMOTOR];
    double motor_vel[NUMOFMOTOR];
    double ctrl_input[NUMOFMOTOR];
} SharedData;

#define SHM_NAME "/mcl_420"
#define SHM_SIZE sizeof(SharedData)

/*공유 메모리 변수*/
SharedData sim_data;



int sigMainKill = 0;
static void cleanup(void);

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
// cpu 코어당 상용량 확인 : top , 1
double cnt = 0 ;

const char *IFNAME = "enp6s0"; // Name of IF card
char    IOmap[4096];

int expectedWKC;
volatile int wkc;
double CL[NUMOFSLAVES];

bool _f_ECAT_PDO_Success = false;


Plant::Plant() : Node("plant")
{
    auto qos_profile = rclcpp::QoS(rclcpp::KeepLast(1)).reliability(RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT);
    plant_publisher_ = this->create_publisher<MclActuator>
        ("plant", qos_profile);

    plant_publisher_ =
        this->create_publisher<MclActuator>("plant", qos_profile);
};
Plant::~Plant(){}


void Plant::publish_motor_msg(double curr_time)
{
    MclActuator msg;
    for(int i = 0; i< NUMOFMOTOR ; i++)
    {
        msg.time_stamp = curr_time;
        msg.motor_pos[i] = 0;
        msg.motor_vel[i] = 0;
        msg.ctrl_input[i] = 0;
    }
    
    plant_publisher_ -> publish(msg);
    // RCLCPP_INFO(this->get_logger(), "Published argument_a %.2f", msg.motor_pos[0]);
    // RCLCPP_INFO(this->get_logger(), "Published argument_b %.2f", msg.ctrl_input[0]);

}


///** 2. EtherCAT initiation **///

int _f_DS402_control_command;
void *realtime_thread(void *arg)
{
    using namespace std;
    std::fill_n(CL, NUMOFSLAVES, 20.0);
    int8_t      modeOP[NUMOFSLAVES];            std::fill_n(modeOP,NUMOFSLAVES,0);
    uint16_t    controlword[NUMOFSLAVES];       std::fill_n(controlword,NUMOFSLAVES,0);
    uint16 tmp_ctrCmd = 0;
    uint32_t    digital_output[NUMOFSLAVES];    std::fill_n(digital_output,NUMOFSLAVES,0);

    /*real plant*/
    int32_t     position_raw[NUMOFSLAVES];      std::fill_n(position_raw,NUMOFSLAVES,0);
    int32_t     velocity_raw[NUMOFSLAVES];      std::fill_n(velocity_raw,NUMOFSLAVES,0);
    int16_t     torque_raw[NUMOFSLAVES];        std::fill_n(torque_raw,NUMOFSLAVES,0);
    int16_t     Ain1_raw[NUMOFSLAVES];          std::fill_n(Ain1_raw,NUMOFSLAVES,0);
    uint32_t    DCvolt_raw[NUMOFSLAVES];        std::fill_n(DCvolt_raw,NUMOFSLAVES,0);
    uint32_t    Din_raw[NUMOFSLAVES];           std::fill_n(Din_raw,NUMOFSLAVES,0);
    uint16_t    statusword[NUMOFSLAVES];        std::fill_n(statusword,NUMOFSLAVES,0);
    int8_t      modeofOP_disp[NUMOFSLAVES];     std::fill_n(modeofOP_disp,NUMOFSLAVES,0);
    int32_t     spring_pos_raw[NUMOFSLAVES];    std::fill_n(spring_pos_raw,NUMOFSLAVES,0);
    int32_t     spring_pos_raw_offset[NUMOFSLAVES];    std::fill_n(spring_pos_raw_offset,NUMOFSLAVES,0);

    double      motor_pos_raw[NUMOFSLAVES];         std::fill_n(motor_pos_raw,NUMOFSLAVES,0.0);
    double      motor_vel_raw[NUMOFSLAVES];         std::fill_n(motor_vel_raw,NUMOFSLAVES,0.0);
    double      motor_torque_raw[NUMOFSLAVES];      std::fill_n(motor_torque_raw,NUMOFSLAVES,0.0);
    
    double      motor_pos[NUMOFSLAVES];             std::fill_n(motor_pos,NUMOFSLAVES,0.0);
    double      motor_vel[NUMOFSLAVES];             std::fill_n(motor_vel,NUMOFSLAVES,0.0);
    double      motor_acc[NUMOFSLAVES];             std::fill_n(motor_acc,NUMOFSLAVES,0.0);
    double      motor_torque[NUMOFSLAVES];          std::fill_n(motor_torque,NUMOFSLAVES,0.0);
    // double      old_motor_pos[NUMOFSLAVES];         std::fill_n(old_motor_pos,NUMOFSLAVES,0.0);

    double      spring_torque_raw[NUMOFSLAVES];     std::fill_n(spring_torque_raw,NUMOFSLAVES,0.0);

    double      tmp_motor_pos_offset[NUMOFSLAVES];      std::fill_n(tmp_motor_pos_offset,NUMOFSLAVES,0.0);
    double      tmp_motor_vel_offset[NUMOFSLAVES];      std::fill_n(tmp_motor_vel_offset,NUMOFSLAVES,0.0);
    double      motor_pos_offset[NUMOFSLAVES];      std::fill_n(motor_pos_offset,NUMOFSLAVES,0.0);
    double      motor_vel_offset[NUMOFSLAVES];      std::fill_n(motor_vel_offset,NUMOFSLAVES,0.0);
    double      motor_torque_offset[NUMOFSLAVES];   std::fill_n(motor_torque_offset,NUMOFSLAVES,0.0);

    double      ctrl_input[NUMOFSLAVES];             std::fill_n(ctrl_input,NUMOFSLAVES,0.0);
    double      ctrl_current[NUMOFSLAVES];             std::fill_n(ctrl_current,NUMOFSLAVES,0.0);

    output_GTWI_t *out_twitter[NUMOFSLAVES];   // RXPDO mapping data (output to the slaves)
    input_GTWI_t  *in_twitter[NUMOFSLAVES];    // TXPDO mapping data (input from the slaves)
    uint16_t    slave_state[NUMOFSLAVES+1];     std::fill_n(slave_state,NUMOFSLAVES+1,0);  
    

    /*simulation plant*/
    double      motor_sim_pos[NUMOFSLAVES];             std::fill_n(motor_sim_pos,NUMOFSLAVES,0.0);
    double      motor_sim_vel[NUMOFSLAVES];             std::fill_n(motor_sim_vel,NUMOFSLAVES,0.0);
    double      ctrl_sim_input[NUMOFSLAVES];             std::fill_n(ctrl_sim_input,NUMOFSLAVES,0.0);

    double      target_m_pos[NUMOFSLAVES];             std::fill_n(target_m_pos,NUMOFSLAVES,0.0);
    double      target_sim_pos[NUMOFSLAVES];             std::fill_n(target_sim_pos,NUMOFSLAVES,0.0);
    
    std::fill_n(modeOP,NUMOFSLAVES,DS402_MODE_OP_TORQUE_PROFILED);
    

    /* control */
    double gain_real[3]{0}; 
    double gain_sim[3]{0};
    double sin_freq[2]{0}; 
    double cos_duration[2]{0};
    sin_freq[0]= 0.5; sin_freq[1]= 0.5;
    
    
    double gearRatio = 100.0;
    double torqueConst = 0.035;  
    
    Controller real_C; Controller sim_C;
    Trajectory real_traj; Trajectory sim_traj;

/* ethercat 통신 */
    if(ecat_init(IFNAME))       //Initialize EtherCAT Master
    {
        sigMainKill = 0;
        std::cout <<"working" <<std::endl;
    }
    else
    {  
        sigMainKill = 1;
        std::cout <<"cannot open" <<std::endl;
    }

    for (int i=1 ; i<=NUMOFSLAVES ; i++)
    {
        slave_state[i] = ec_slave[i].state;
        printf("Initialize EtherCAT Master. Slave %d state=%d\n", i, slave_state[i]);
    }
    if(ec_slavecount == NUMOFSLAVES)
    {
        sigMainKill = 0;
        for (int i=1 ; i <= NUMOFSLAVES ; i++)
        {
            std::cout << "this"<<std::endl;
            ec_slave[i].PO2SOconfig = ecat_PDO_Config;  // link slave specific setup to preop->safeop hook
        }
    }
    else
    {   
        std::cout << "ec_slavecount == NUMOFSLAVES"<<std::endl;
        sigMainKill = 1;
    }
    ec_config_map(&IOmap);
    ec_configdc();
    for(int i=1 ; i<=NUMOFSLAVES ; i++)
    {
        slave_state[i] = ec_slave[i].state;
        printf("Configure PDO mapping. Slave %d state=%d\n",i, slave_state[i]);
    }
    ec_statecheck(0, EC_STATE_SAFE_OP,  EC_TIMEOUTSTATE);
    expectedWKC = (ec_group[0].outputsWKC * 2) + ec_group[0].inputsWKC; //expected working counter (wkc) calculation
    printf("expected working counter : %d",expectedWKC);
    ec_slave[0].state = EC_STATE_OPERATIONAL;
    /* send one valid process data to make outputs in slaves happy*/
    ec_send_processdata();
    ec_receive_processdata(EC_TIMEOUTRET);
    /* request OP state for all slaves */
    ec_writestate(0);
    ec_statecheck(0, EC_STATE_OPERATIONAL,  EC_TIMEOUTSTATE * 4);
    for (int i=1 ; i<=NUMOFSLAVES ; i++)
    {
        slave_state[i] = ec_slave[i].state;
        printf("Slaves to be operational state. Slave %d state=%d\n",i, slave_state[i]);
    }
    uint32 obytes1 = ec_slave[1].Obytes;
    uint32 ibytes1 = ec_slave[1].Ibytes;
    uint32 obytes2 = ec_slave[2].Obytes;
    uint32 ibytes2 = ec_slave[2].Ibytes;

    printf("Slave1: Out-byte=%d, In-byte=%d, Slave2: Out-byte=%d, In-byte=%d\n", obytes1, ibytes1, obytes2, ibytes2);

    for (int i = 0 ; i < NUMOFSLAVES ; i++)
    {
        out_twitter[i] = (output_GTWI_t*) ec_slave[i+1].outputs;
        in_twitter[i] = (input_GTWI_t*) ec_slave[i+1].inputs;
    }
//==================================//==================================//==================================


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
    

    double Ts_r = 0.001;
    double Ts_sim = 0.001;

    while(!sigMainKill) // cnt가 어느정도 이상되면 main에서 sigMainkill을 true로 바꿔줌
    {
        clock_gettime(CLOCK_MONOTONIC, &trt); //get the system time

        old_t1 = t1;
        t1 = trt.tv_nsec;
        ts = trt.tv_sec;
        delta_t1 = t1 - old_t1;
        sampling_ms = (double)delta_t1 * 0.000001;
        double jitter = sampling_ms - 1.000; 

        // if(ticks>1) overrun += ticks - 1; 
        if(jitter >1) overrun +=1; // 차이가 1ms이상

        /// B-2. EtherCAT
        ec_send_processdata();
        wkc = ec_receive_processdata(EC_TIMEOUTRET);
        for (int i=1 ; i<=NUMOFSLAVES ; i++)
        {
            slave_state[i] = ec_slave[i].state;
        }
        if(expectedWKC > wkc)   // This means the etherCAT frame cannot be successfully read or wrote on at least one slaves.
            _f_ECAT_PDO_Success = false;
        else
            _f_ECAT_PDO_Success = true;

        for (int i = 0 ; i < NUMOFSLAVES ; i++) //Copy the data from received PDO
        {
            /*실제 데이터*/
            position_raw[i]     = in_twitter[i]->TXPDO_ACTUAL_POSITION_DATA;
            velocity_raw[i]     = in_twitter[i]->TXPDO_ACTUAL_VELOCITY_DATA;
            torque_raw[i]       = in_twitter[i]->TXPDO_ACTUAL_TORQUE_DATA;
            //Ain1_raw[i]         = in_twitter[i]->TXPDO_ANALOG_INPUT_1_DATA;
            //DCvolt_raw[i]       = in_twitter[i]->TXPDO_DC_LINK_CIRCUIT_VOLTAGE_DATA;
            Din_raw[i]          = in_twitter[i]->TXPDO_DIGITAL_INPUTS_DATA;
            statusword[i]       = in_twitter[i]->TXPDO_STATUSWORD_DATA;
            modeofOP_disp[i]    = in_twitter[i]->TXPDO_MODE_OF_OPERATION_DISPLAY_DATA;
            spring_pos_raw[i]   = in_twitter[i]->TXPDO_AUXILIARY_POSITION_ACTUAL_VALUE_DATA;

            motor_pos_raw[i]    = ((double)position_raw[i])/20000.0*2.0*PI;
            motor_vel_raw[i] = ((double)velocity_raw[i])/20000.0*2.0*PI;
            motor_torque_raw[i] = ((double)torque_raw[i])/1000.0*CL[i];
            spring_torque_raw[i]   = ((double)spring_pos_raw[i])/524288.0*2.0*PI*1900;
            spring_torque_raw[i] = -spring_torque_raw[i];

            /*실제 로봇 */
            motor_pos[i] = motor_pos_raw[i]/gearRatio;

            /*시뮬레이션 데이터*/
            motor_sim_pos[i] = sim_data.motor_pos[0];
            motor_sim_vel[i] = sim_data.motor_vel[0];
        }
        
        
        
        real_C.j_set_gain(5,0,0.2,5, Ts_r); sim_C.j_set_gain(5,0,0.2,5, Ts_sim);
        
        
        
        if(0<cnt &&cnt<1)
        {
            std::fill_n(controlword,NUMOFSLAVES,DS402_controlword(DS402_CTR_CMD_FAULT_RESET, tmp_ctrCmd));
            cout << "fault reset"<<endl;
        }

        else if(0<cnt &&cnt<1)
        {
            cout << "shut down"<<endl;
            std::fill_n(controlword,NUMOFSLAVES,DS402_controlword(DS402_CTR_CMD_SHUTDOWN, tmp_ctrCmd));
        }
        else if(1<cnt &&cnt <2) 
        {
            std::fill_n(controlword,NUMOFSLAVES,DS402_controlword(DS402_CTR_CMD_DISABLE_OPERATION, tmp_ctrCmd));
            cout << "disable operation"<<endl;
        }
        else if(2<cnt &&cnt <5) 
        {
            
            std::fill_n(controlword,NUMOFSLAVES,DS402_controlword(DS402_CTR_CMD_SWITCH_ON_AND_ENABLE, tmp_ctrCmd));
            cout <<"enable" <<endl;
        }

        else 
        {
            std::fill_n(controlword,NUMOFSLAVES,DS402_controlword(DS402_CTR_CMD_ENABLE_OPERATION, tmp_ctrCmd));
            // offset 만들기
            
            

            /* PID컨트롤러 짜기 */
            for(int i = 0 ; i<NUMOFSLAVES; i++)
            {
                target_m_pos[i] = real_traj.sin_j_traj(motor_pos[i],1,sin_freq[0]);
                ctrl_input[i] = real_C.j_posPID(target_m_pos[i],motor_pos[i]);
                ctrl_current[i]  = ctrl_input[i]/(torqueConst*gearRatio);

                target_sim_pos[i] = sim_traj.sin_j_traj(motor_sim_pos[i],1,sin_freq[1]);
                ctrl_sim_input[i] = sim_C.j_posPID(target_sim_pos[i],motor_sim_pos[i]);
            }
            if(ctrl_current[0]>10) ctrl_current[0] = 10 ;
            else if(ctrl_current[0]<-10) ctrl_current[0] = -10 ;
            cout <<"error = " << target_m_pos[0]- motor_pos[0]<< " target = "<< target_m_pos[0]<< " "<< "control input = " << " "<<ctrl_current[0]<<endl;
        }
        


        //outputs for ECAT
        for (int i = 0 ; i < NUMOFSLAVES ; i++) //Copy the data from received PDO
        {
            out_twitter[i]->RXPDO_TARGET_POSITION_DATA     = 0;//(int32)target_position[i];
            out_twitter[i]->RXPDO_TARGET_POSITION_DATA_0   = 0;//(int32)target_position[i];
            out_twitter[i]->RXPDO_TARGET_VELOCITY_DATA     = (int32)(10*20000.0/(2*PI));
            //    out_twitter[i]->RXPDO_TARGET_TORQUE_DATA       = (int16)(target_current[i]*1000.0/CL[i]);
            out_twitter[i]->RXPDO_TARGET_TORQUE_DATA       = (int16)(ctrl_current[i]*1000.0/CL[i]);
            out_twitter[i]->RXPDO_DIGITAL_OUTPUTS_DATA     = digital_output[i];
            out_twitter[i]->RXPDO_MAXIMAL_TORQUE           = 1000;
            out_twitter[i]->RXPDO_CONTROLWORD_DATA         = controlword[i];
            out_twitter[i]->RXPDO_CONTROLWORD_DATA_0       = controlword[i];
            out_twitter[i]->RXPDO_MODE_OF_OPERATION_DATA   = modeOP[i];

            sim_data.time_stamp = (double) trt.tv_sec + (trt.tv_nsec/1e6);
            sim_data.motor_num[i] = i;
            // sim_data.motor_pos[i] = motor_pos_raw[i];
            // sim_data.motor_vel[i] = motor_pos_raw[i];
            sim_data.ctrl_input[i] = ctrl_sim_input[i];

        }

       


        /*통신으로 설정할 때*/
        // (*node_ptr2)->publish_helloworld_msg();


        cnt +=RT_PERIOD_MS*0.001;



        memcpy(shm_ptr, &sim_data, sizeof(SharedData));

        /*shared memory 전달한 값 확인 */
        // printf("written to shared memory: motor_num=%d, motor_pos=%.4f, time_stamp = %.4f\n",
        //       sim_data.motor_num[0], sim_data.motor_pos[0], sim_data.time_stamp);
        /* 루프타임 확인 */
        // printf("PERIODIC TIME --- %.4f, Jitter --- %+.4f, OVERRUN --- %d \r\n", sampling_ms, jitter, overrun);

        if(!pthread_mutex_trylock(&data_mut))
        {
            pthread_mutex_unlock(&data_mut);
        }
        err = read(tfd, &ticks,sizeof(ticks));  //해당 타이머가 설정한 간격으로 발생한 타이머 틱수를 읽어옴.
        if(err<0) error(1,errno, "read()");
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
    
    auto node_ptr = std::make_shared<Plant>();

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



#endif  // ARITHMETIC__ARGUMENT_HPP_
