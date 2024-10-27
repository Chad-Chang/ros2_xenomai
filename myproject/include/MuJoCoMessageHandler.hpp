#ifndef MUJOCOMESSAGEHANDLER_H_
#define MUJOCOMESSAGEHANDLER_H_

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

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <ratio>
#include <thread>
#include "MCLmodel.hpp"
#include <mujoco/mujoco.h>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "motor_commu/msg/mcl_actuator.hpp"


using std::placeholders::_1;
using namespace std::chrono_literals;

class MuJoCoMessageHandler: public rclcpp::Node
{
private:
    /* data */
    rclcpp::Publisher<motor_commu::msg::MclActuator>::SharedPtr
      joint_state_publisher_;
    rclcpp::Subscription<motor_commu::msg::MclActuator>::SharedPtr
      actuator_cmd_subscription_;
    // std::shared_ptr<rclcpp::ParameterEventHandler> param_subscriber_;
    // std::shared_ptr<rclcpp::ParameterCallbackHandle> cb_handle_;
    std::shared_ptr<MCL::MCLmodel_> mcl_model_;
    
    

    

public:
  void actuator_cmd_callback(
    const motor_commu::msg::MclActuator::SharedPtr msg) const; // 보증을 의미함.
  void joint_callback();
  MuJoCoMessageHandler(MCL::MCLmodel_ *mcl_model);
  ~MuJoCoMessageHandler();
};


#endif // ECAT_FUNC_H
