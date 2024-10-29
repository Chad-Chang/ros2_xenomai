#ifndef MCLMODEL_H_
#define MCLMODEL_H_

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
#include <iostream>

#define NUMOFMOTOR 12

namespace MCL
{
struct MCLmodel_
{
    double time = 0.0;
    double time_old = 0.0;
    double motor_num= 0 ;
    double motor_pos= 0 ;
    double motor_vel= 0 ;
    double ctrl_input= 0 ;
};


}


typedef struct {
    float time_stamp;
    int motor_num[NUMOFMOTOR];
    double motor_pos[NUMOFMOTOR];
    double motor_vel[NUMOFMOTOR];
    double ctrl_input[NUMOFMOTOR];
} SharedData;

#endif // ECAT_FUNC_H
