//******************************************************************************//
// ecat_func.cpp
// Author: Junyoung Kim, Ph.D.
// Date of comments: May 15, 2023
// Description:
//      Declaration for the etherCAT master-related functions and variables
//******************************************************************************//

#include "ecat_func.hpp"

boolean needlf;
boolean inOP;
uint8 currentgroup = 0;
int ecat_init(char *ifname)     //etherCATinitialization with the NIC ifname (ex-enp0)
{
    int i, oloop, iloop, k, wkc_count;
    needlf = FALSE;

    printf("Initializing EtherCAT Master...\n");

    if( ec_init(ifname) > 0)
    {
        printf("ec_init on %s succeeded.\n", ifname);

        if( ec_config_init(FALSE) > 0 )
        {
            printf("%d slaves found and configured.\n",ec_slavecount);

            if( ec_slavecount < NUMOFSLAVES )
            {
                return 0;
            }

            /*if( strcmp(ec_slave[TWITTER_1].name,TWITTER_1_NAME) )
            {
                char *nametmp;
                nametmp = ec_slave[TWITTER_1].name;
                printf("Name of slave is %s, not TWITTER.\n",nametmp);
                return 0;
            }*/
        }
        else
        {
            return 0;
        }
    }
    else
    {
        return 0;
    }

    return 1;
}

int ecat_PDO_Config(uint16 slave)
{
    int retval_SM2 = 0;
    int retval_SM3 = 0;

    retval_SM2 += ec_SDOwrite(slave, SM2_RXPDO_ASSIGN, 0x00,     TRUE, sizeof(RXPDO_ADDR_GTWI),   &RXPDO_ADDR_GTWI,    EC_TIMEOUTSAFE);
    retval_SM3 += ec_SDOwrite(slave, SM3_TXPDO_ASSIGN, 0x00,     TRUE, sizeof(TXPDO_ADDR_GTWI),   &TXPDO_ADDR_GTWI,    EC_TIMEOUTSAFE);

    printf("PDO config : retval_SM2 = %d, retval_SM3 = %d\n",retval_SM2,retval_SM3);

    return 1;
}




/*uint16 DS402_controlword(int controlCmd, uint16 tmp_controlword)
{
    uint16 cmd = 0b0000000000000000;

    switch (controlCmd)
    {
        case DS402_CTR_CMD_SHUTDOWN:
            cmd = tmp_controlword & (0b1111111101111110);   //set 0 for bits 7 and 0
            cmd = cmd | (0b0000000000000110);               //set 1 for bits 2 and 1
            break;

        case DS402_CTR_CMD_SWITCH_ON:
            cmd = tmp_controlword & (0b1111111101110111);   //set 0 for bits 7 and 3
            cmd = cmd | (0b0000000000000111);               //set 1 for bits 2, 1, and 0
            break;

        case DS402_CTR_CMD_SWITCH_ON_AND_ENABLE:
            cmd = tmp_controlword & (0b1111111101111111);   //set 0 for bit 7
            cmd = cmd | (0b0000000000001111);               //set 1 for bits 3, 2, 1, and 0
            break;

        case DS402_CTR_CMD_DISABLE_VOLTAGE:
            cmd = tmp_controlword & (0b1111111101111101);   //set 0 for bits 7 and 1
            break;

        case DS402_CTR_CMD_QUICKSTOP:
            cmd = tmp_controlword & (0b1111111101111011);   //set 0 for bit 7 and 2
            cmd = cmd | (0b0000000000000010);               //set 1 for bit 1
            break;

        case DS402_CTR_CMD_DISABLE_OPERATION:
            cmd = tmp_controlword & (0b1111111101110111);   //set 0 for bit 7 and 3
            cmd = cmd | (0b0000000000000111);               //set 1 for bits 2, 1, and 0
            break;

        case DS402_CTR_CMD_ENABLE_OPERATION:
            cmd = tmp_controlword & (0b1111111101111111);   //set 0 for bit 7
            cmd = cmd | (0b0000000000001111);               //set 1 for bits 3, 2, 1, and 0
            break;

        case DS402_CTR_CMD_FAULT_RESET:
            cmd = tmp_controlword & (0b1111111111111111);   //set 0 for bit for nothing
            cmd = cmd | (0b0000000010000000);               //set 1 for bit 7 to make rising edge
            break;

        default:
            cmd = 0b0000000000000000;
            break;
    }

    return cmd;
}*/

uint16 DS402_controlword(int controlCmd, uint16 tmp_controlword)
{
    (void) tmp_controlword;
    uint16 cmd = 0b0000000000000000;

    switch (controlCmd)
    {
        case DS402_CTR_CMD_SHUTDOWN:
            cmd = 6;
            break;

    case DS402_CTR_CMD_SWITCH_ON_AND_ENABLE:
            cmd = 14;
            break;

        case DS402_CTR_CMD_DISABLE_OPERATION:
            cmd = 7;        //set 1 for bits 2, 1, and 0
            break;

        case DS402_CTR_CMD_ENABLE_OPERATION:
            cmd = 15;            //set 1 for bits 3, 2, 1, and 0
            break;

        case DS402_CTR_CMD_FAULT_RESET:
            cmd = 128;
            break;

        default:
            cmd = 0b0000000000000000;
            break;
    }

    return cmd;
}






