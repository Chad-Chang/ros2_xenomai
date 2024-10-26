// #include "plant_shm.hpp"
// #include <cstdio>
// #include <memory>
// #include <string>
// #include <utility>


// Plant::Plant() : Node("plant")
// {
//     auto qos_profile = rclcpp::QoS(rclcpp::KeepLast(1)).reliability(RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT);
//     plant_publisher_ = this->create_publisher<MclActuator>
//         ("plant", qos_profile);

//     plant_publisher_ =
//         this->create_publisher<MclActuator>("plant", qos_profile);
// };

// Plant::~Plant(){}

// void Plant::publish_motor_msg(double curr_time)
// {
//     MclActuator msg;
//     for(int i = 0; i< NUMOFMOTOR ; i++)
//     {
//         msg.time_stamp = curr_time;
//         msg.motor_pos[i] = 0;
//         msg.motor_vel[i] = 0;
//         msg.ctrl_input[i] = 0;
//     }
    
//     plant_publisher_ -> publish(msg);
//     RCLCPP_INFO(this->get_logger(), "Published argument_a %.2f", msg.motor_pos[0]);
//     RCLCPP_INFO(this->get_logger(), "Published argument_b %.2f", msg.ctrl_input[0]);

// }

