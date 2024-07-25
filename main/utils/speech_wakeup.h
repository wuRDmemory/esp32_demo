#pragma once

#include "freertos/FreeRTOS.h"
/*
Basic, somewhat quick and dirty, interface for an audio source. The main code uses this to grab samples 
from whatever input source it instantiates. Here's how to use it:

- Create a src_cfg_t variable
- Set item_size to the amount of samples per queue read.
- Set queue to a queue with an item size of (item_size * sizeof(int16_t))
- Create a task for the worker function of the input method of your choice (e.g. wavsrcTask). Pass the
  src_cfg_t variable address as the argument.
- The worker task should now start filling the queue. Receive from it and do with the samples as 
  you please.

Note that at the moment all source interfaces are expected to return signed 16-bit samples at an 16KHz 
sample rate.
*/

#ifdef __cplusplus
extern "C"
{
#endif


  /**
   * speech wakeup initialization
  */
  void speech_wakeup_initialise();

#ifdef __cplusplus
}
#endif