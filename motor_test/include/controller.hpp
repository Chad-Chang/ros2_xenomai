#ifndef CONTROLLER_H_
#define CONTROLLER_H_

#include <math.h>
#include <stdio.h>
#define pi 3.141592

class Controller
{
    public:
        Controller();
        void init_ctrl();
        // 이거 없어도 되긴한데 이전 코드 복붙해서 걍 씀.
        double tustin_derivative(double input, double input_old, double output_old, double cutoff_freq);

        // gain setting
        void j_set_gain(double Kp, double Ki, double Kd, double d_cutoff, double dt)
            {j_Kp_ = Kp; j_Ki_ = Ki; j_Kd_ = Kd; d_cutoff_ = d_cutoff ; dt_ = dt;}
        
        // constrain
        double j_constraint(double v, double v_min, double v_max){return (v> v_max)?v_max : (v<v_min)?v_min:v;}
        
        // PID operator
        double j_posPID(double target, double current_ang);
        
        // old 값들 update
        void j_setDelayData() {j_D_term_[1] = j_D_term_[0]; j_I_term_[1] = j_I_term_[0];}
        double j_get_posPgain() {return j_Kp_;}
        double j_get_posIgain() {return j_Ki_;}
        double j_get_posDgain() {return j_Kd_;}

    private:
        double d_cutoff_= 0;
        double j_Kp_, j_Kd_, j_Ki_; 
        double j_I_term_[2] = {0};
        double j_D_term_[2] = {0};
        double j_P_term_ = 0;
        double j_imax_;
        double j_err[2] = {0};
        double dt_ = 0.001;
};

#endif // ECAT_FUNC_H
