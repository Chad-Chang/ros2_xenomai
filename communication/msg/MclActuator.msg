# Messages
#builtin_interfaces/Time stamp
std_msgs/Header header

#bool mode  # simulation:0 or real:1

string[<=30] motor_num
float32[<=30] motor_pos
float32[<=30] motor_vel
float32[<=30] ctrl_input

#float32[<=30] jki
#float32[<=30] jkd
#float32[<=30] jkp

#float32 qdob
#float32 qodob
#float32 qjdob
