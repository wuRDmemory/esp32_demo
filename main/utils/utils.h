#ifndef __UTILS_UTILS_H__
#define __UTILS_UTILS_H__

#include <stdio.h>
#include <stdbool.h>

/**
 * @brief sync with the nstp
 * @param bool sync result
*/
void sync_with_nstp();

/**
 * @brief Get the current unix timestamp
 * @return double unix timestamp
*/
double get_unix_timestamp();


#endif