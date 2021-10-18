/**
 * \author Abubakr Ehab Samir Nada
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdint.h>
#include <time.h>


typedef struct {
    uint16_t id;
    double value;
    time_t ts;
} sensor_data_t;
typedef enum {
    DATA_ENTRY, STORE_ENTRY
} ENTRY_TYPE; 

#endif /* _CONFIG_H_ */
