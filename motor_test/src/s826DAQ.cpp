#include "s826DAQ.hpp"

int SetDacOutput(uint board, uint chan, uint range, double volts)
{
  uint setpoint;
  switch (range) {  // conversion is based on dac output range:
    case S826_DAC_SPAN_0_5:   setpoint = (uint)(volts * 0xFFFF /  5);          break; // 0 to +5V
    case S826_DAC_SPAN_0_10:  setpoint = (uint)(volts * 0xFFFF / 10);          break; // 0 to +10V
    case S826_DAC_SPAN_5_5:   setpoint = (uint)(volts * 0xFFFF / 10) + 0x8000; break; // -5V to +5V
    case S826_DAC_SPAN_10_10: setpoint = (uint)(volts * 0xFFFF / 20) + 0x8000; break; // -10V to +10V
    default:                  return S826_ERR_VALUE;                                  // invalid range
  }
  return S826_DacDataWrite(board, chan, setpoint, 0);  // program DAC output and return error code
}

// static int Aout_Write(uint board, uint channel, double val)
// {
//     (void) board; (void) channel; (void) val;
//     int err = 0;

//     return err;
// }

// static int Ain_Read(uint board, uint channel, double &val)
// {
//     (void) board; (void) channel; (void) val;
//     int err = 0;

//     return err;
// }

// static int QE_Read(uint board, uint channel, uint32_t val)
// {
//     (void) board; (void) channel; (void) val;
//     int err = 0;

//     return err;
// }

// static int Doutbit_Write(uint board, uint channelbit, bool val)
// {
//     (void) board; (void) channel; (void) val;
//     int err = 0;

//     return err;
// }

// static int Doutbyte_Write(uint board, uint channelbyte, bool val)
// {
//     (void) board; (void) channel; (void) val;
//     int err = 0;

//     return err;
// }

// static int Din_Read(uint board, uint channelbit, uint32_t val[2])
// {
//     (void) board; (void) channel; (void) val;
//     int err = 0;

//     return err;
// }

