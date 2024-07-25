#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define I2S_NUM 1
#define IIS_SCLK 16
#define IIS_LCLK 7
#define IIS_DSIN 6
#define IIS_DOUT 15

/**
 * record i2s initialise
 * @param void
 * @return void
*/
void record_i2s_initialise(void);

/**
 * play i2s initialise
 * @param void
 * @return void
*/
void play_i2s_initialise(void);

/**
 * disable all i2s
 * @param void
 * @return void
*/
void disable_all_i2s(void);

#ifdef __cplusplus
}
#endif
