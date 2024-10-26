// #ifndef PLANT_HPP_
// #define PLANT_HPP_

#include <chrono>
#include <memory>
#include <string>
#include <utility>
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "motor_commu/msg/mcl_actuator.hpp"

using namespace std::chrono_literals;

class Plant : public rclcpp::Node
{
public:
    using MclActuator = motor_commu::msg::MclActuator;
    explicit Plant();
    virtual ~Plant();
    void publish_motor_msg(double curr_time);
    
private:
    /* data */
    rclcpp::Publisher<MclActuator>::SharedPtr plant_publisher_;
};


// #endif  // ARITHMETIC__ARGUMENT_HPP_
