#include "trajectory.hpp"

Trajectory::Trajectory()
{
};

Trajectory::~Trajectory()
{
};

double Trajectory::cos_j_traj(double init_pos, double targ_pos, double cur_pos, double duration)
{
    traj_t_cos_ ++;
    double traj_pos;

    if(traj_t_cos_*Ts_ < duration)
    {
        traj_pos= 0.5*(targ_pos - init_pos)*(1-cos(M_PI/duration*(traj_t_cos_*Ts_))) + init_pos;
    }
    else
    {
        traj_pos = targ_pos;
    }
    return traj_pos;
};

double Trajectory::sin_j_traj(double current_position, double amplitude, double freq)
{
    traj_t_sin_ ++;
    double  offset = amplitude*sin(2 * M_PI*freq * traj_t_sin_*Ts_);
    return offset + current_position;
}