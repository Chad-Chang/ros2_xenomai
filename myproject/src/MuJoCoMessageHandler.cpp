#include "MuJoCoMessageHandler.hpp"


MuJoCoMessageHandler::MuJoCoMessageHandler(MCL::MCLmodel_ *mcl_model): 
    Node("MuJoCoMessageHandler"), mcl_model_(mcl_model)
{
    RCLCPP_INFO(this->get_logger(), "Start MuJoCoMessageHandler ...");
    auto qos_profile = rclcpp::QoS(rclcpp::KeepLast(1)).reliability(RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT);
    joint_state_publisher_ = this->create_publisher<motor_commu::msg::MclActuator>(
       "joint_states", qos_profile);

    actuator_cmd_subscription_ =
        this->create_subscription<motor_commu::msg::MclActuator>(
        "actuators_cmds", qos_profile,
        std::bind(&MuJoCoMessageHandler::actuator_cmd_callback, this,
                  std::placeholders::_1));

    // mcl_model_ = std::make_shared<MCL::MCL_model>(); 
}

MuJoCoMessageHandler::~MuJoCoMessageHandler()
{
    RCLCPP_INFO(this->get_logger(), "close node ...");
}

void MuJoCoMessageHandler::actuator_cmd_callback(
    const motor_commu::msg::MclActuator::SharedPtr msg) const 
{
    RCLCPP_INFO(this->get_logger(), "subscribe actuator cmds");

    mcl_model_-> motor_num = msg->motor_num[0];
    mcl_model_-> motor_pos = msg->motor_pos[0];
    mcl_model_-> motor_vel = msg->motor_vel[0];
    mcl_model_-> ctrl_input = msg->ctrl_input[0];
}

void MuJoCoMessageHandler::joint_callback() 
{
    // const std::lock_guard<std::mutex> lock(sim_->mtx);
    motor_commu::msg::MclActuator jointState;
    jointState.motor_num[0] = mcl_model_->motor_num;
    jointState.motor_pos[0] = mcl_model_->motor_pos;
    jointState.motor_vel[0] = mcl_model_->motor_vel;

    RCLCPP_INFO(this->get_logger(), "publish joint sensor");

    joint_state_publisher_->publish(jointState);
}