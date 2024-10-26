//******************************************************************************//
// ecat_func.h
// Author: Junyoung Kim, Ph.D.
// Date of comments: May 15, 2023
// Description:
//      Declaration for the etherCAT master-related functions and variables
//******************************************************************************//


#ifndef ECAT_FUNC_H
#define ECAT_FUNC_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/timerfd.h>
#include <string.h>
#include <malloc.h>
#include <pthread.h>
#include <fcntl.h>
#include <error.h>
#include <errno.h>
#include <sys/mman.h>
#include <rtdm/ipc.h>
#include <inttypes.h>


#include "ethercat.h"

// Board type
#define SLAVE_GTWI
//#define SLAVE_PTWI

// Number of slaves
#define NUMOFSLAVES	1

// Define slaves number
#define TWITTER_left    1
#define TWITTER_right   2

#define SM2_RXPDO_ASSIGN    0x1C12
#define SM3_TXPDO_ASSIGN    0x1C13

// DS402
#define DS402_CTR_CMD_NA                    -1
#define DS402_CTR_CMD_SHUTDOWN              0
#define DS402_CTR_CMD_SWITCH_ON             1
#define DS402_CTR_CMD_SWITCH_ON_AND_ENABLE  2
#define DS402_CTR_CMD_DISABLE_VOLTAGE       3
#define DS402_CTR_CMD_QUICKSTOP             4
#define DS402_CTR_CMD_DISABLE_OPERATION     5
#define DS402_CTR_CMD_ENABLE_OPERATION      6
#define DS402_CTR_CMD_FAULT_RESET           7

#define DS402_MODE_OP_PROFILED_POSITION     1
#define DS402_MODE_OP_PROFILED_VELOCITY     3
#define DS402_MODE_OP_TORQUE_PROFILED       4
#define DS402_MODE_OP_HOMING                6
#define DS402_MODE_OP_CYCLIC_SYNC_POSITION  8
#define DS402_MODE_OP_CYCLIC_SYNC_VELOCITY  9
#define DS402_MODE_OP_CYCLIC_SYNC_TORQUE    10

#define DS402_STATUS_NOT_READY_TO_SWITCH_ON 0
#define DS402_STATUS_SWITCH_ON_DISABLED     1
#define DS402_STATUS_READY_TO_SWITCH_ON     2
#define DS402_STATUS_SWITCH_ON              3
#define DS402_STATUS_OPERATION_ENABLED      4
#define DS402_STATUS_QUICK_STOP_ENABLED     5
#define DS402_STATUS_FAULT_REACTION_ACTIVE  6
#define DS402_STATUS_FAULT                  7

// Define RXPDO mapping
typedef struct PACKED
{
    int32   RXPDO_TARGET_POSITION_DATA;     //0x607A:0x00
    uint32  RXPDO_DIGITAL_OUTPUTS_DATA;     //0x60FE:0x01
    uint16  RXPDO_CONTROLWORD_DATA;         //0x6040:0x00
    int32   RXPDO_TARGET_POSITION_DATA_0;   //0x607A:0x00
    int32   RXPDO_TARGET_VELOCITY_DATA;     //0x60FF:0x00
    int16   RXPDO_TARGET_TORQUE_DATA;       //0x6071:0x00
    uint16  RXPDO_MAXIMAL_TORQUE;           //0x6072:0x00
    uint16  RXPDO_CONTROLWORD_DATA_0;       //0x6040:0x00
    int8    RXPDO_MODE_OF_OPERATION_DATA;   //0x6060:0x00
    uint8   RXPDO_RESERVED;                 //0x0000:0x00
} output_GTWI_t;

// Define TXPDO mapping
typedef struct PACKED
{
    int32   TXPDO_ACTUAL_POSITION_DATA;                 //0x6064:0x00
    int16   TXPDO_ACTUAL_TORQUE_DATA;                   //0x6077:0x00
    uint16  TXPDO_STATUSWORD_DATA;                      //0x6041:0x00
    int8    TXPDO_MODE_OF_OPERATION_DISPLAY_DATA;       //0x6061:0x00
    uint8   TXPDO_RESERVED;                             //0x0000:0x00
    int32   TXPDO_ACTUAL_POSITION_DATA_0;               //0x6064:0x00
    uint32  TXPDO_DIGITAL_INPUTS_DATA;                  //0x60FD:0x00
    int32   TXPDO_ACTUAL_VELOCITY_DATA;                 //0x606C:0x00
    uint16  TXPDO_STATUSWORD_DATA_0;                    //0x6041:0x00
    //uint32  TXPDO_DC_LINK_CIRCUIT_VOLTAGE_DATA;         //0x6079:0x00
    //int16   TXPDO_ANALOG_INPUT_1_DATA;                  //0x2205:0x01
    int32   TXPDO_AUXILIARY_POSITION_ACTUAL_VALUE_DATA; //0x20A0:0x00
} input_GTWI_t;


extern uint16 RXPDO_ADDR_GTWI[3];
extern uint16 TXPDO_ADDR_GTWI[4];

extern uint16 RXPDO_ADDR_PTWI[2];
extern uint16 TXPDO_ADDR_PTWI[2];

extern char    IOmap[4096];

int ecat_init(char *ifname);
int ecat_PDO_Config(uint16 slave);

uint16 DS402_controlword(int controlCmd, uint16 tmp_controlword);

#endif // ECAT_FUNC_H
