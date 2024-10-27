#ifndef MCLMODEL_H_
#define MCLMODEL_H_

#include <stdio.h>
#include <vector>
#include <string.h>
#include <stdlib.h>

namespace MCL
{
struct MCLmodel_
{
    double time = 0.0;
    double motor_num;
    double motor_pos;
    double motor_vel;
    double ctrl_input;
};
}

#endif // ECAT_FUNC_H
