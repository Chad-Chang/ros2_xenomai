#ifndef COSINE_TRAJECTORY_HPP
#define COSINE_TRAJECTORY_HPP

#include <iostream>
#include "stdio.h"
#include <cmath>
#include <chrono>
#include <thread>
class Trajectory
{
private:
    double Ts_ = 0.001;
    double traj_t_cos_= 0;
    double traj_t_sin_= 0;
    /* data */
public:
    Trajectory();
    ~Trajectory();
    double cos_j_traj(double init_pos, double targ_pos, double cur_pos, double duration);
    double sin_j_traj(double current_position, double amplitude, double freq);
};


#endif // COSINE_TRAJECTORY_HPP