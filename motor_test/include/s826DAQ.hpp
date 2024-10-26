#ifndef S826DAQ_H
#define S826DAQ_H

#include "826api.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Helpful macros for DIOs
#define DIO(C)                  ((uint64)1 << (C))                          // convert dio channel number to uint64 bit mask
#define DIOMASK(N)              {(uint)(N) & 0xFFFFFF, (uint)((N) >> 24)}   // convert uint64 bit mask to uint[2] array
#define DIOSTATE(STATES,CHAN)   ((STATES[CHAN / 24] >> (CHAN % 24)) & 1)    // extract dio channel's boolean state from uint[2] array

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ERROR HANDLING
// These examples employ very simple error handling: if an error is detected, the example functions will immediately return an error code.
// This behavior may not be suitable for some real-world applications but it makes the code easier to read and understand. In a real
// application, it's likely that additional actions would need to be performed. The examples use the following X826 macro to handle API
// function errors; it calls an API function and stores the returned value in errcode, then returns immediately if an error was detected.

#define X826(FUNC)   if ((errcode = FUNC) != S826_ERR_OK) { printf("\nERROR: %d\n", errcode); return errcode;}

int SetDacOutput(uint board, uint chan, uint range, double volts);

// static int Aout_Write(uint board, uint channel, double val);

// static int Ain_Read(uint board, uint channel, double &val);

// static int QE_Read(uint board, uint channel, uint32_t val);

// static int Doutbit_Write(uint board, uint channelbit, bool val);

// static int Doutbyte_Write(uint board, uint channelbyte, bool val);

// static int Din_Read(uint board, uint channelbit, uint32_t val[2]);

#endif // S826DAQ_H
