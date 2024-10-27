
#include <math.h>
#include <stdio.h>
#include "controller.hpp"


// constructor 
Controller::Controller(){}

double Controller :: tustin_derivative(double input, double input_old, double output_old, double cutoff_freq)
{
    double time_const = 1 / (2 * pi * cutoff_freq);
    double output = 0;

    output = (2 * (input - input_old) - (0.0001 - 2 * time_const) * output_old) / (0.0001 + 2 * time_const);

    return output;
}

// PID operator
double Controller ::j_posPID(double target, double current_ang)
{   
    double t_const= 1/(2*pi*d_cutoff_); // time constant
    j_err[0] = target-current_ang;
    j_P_term_ = j_Kp_*j_err[0];
    j_D_term_[0] = (j_Kd_*2*(j_err[0]-j_err[1])-(dt_-2*t_const)*j_D_term_[1])/(2*t_const+dt_);
    j_I_term_[0] = j_I_term_[1] + j_Ki_*dt_/2*(j_err[0] + j_err[1]);
    j_I_term_[0] = j_constraint(j_I_term_[0],-10000,10000);
    // printf("PID = %f, %f, %f\n", P_term_, I_term_[0] , D_term_[0]);
    j_err[1] = j_err[0];
    return j_P_term_+ j_I_term_[0] + j_D_term_[0];
}

void Controller::init_ctrl()
{
    double j_I_term_[2] = {0};
    double j_D_term_[2] = {0};
    double j_P_term_ = 0;
    double j_imax_;
    double j_err[2] = {0};
}