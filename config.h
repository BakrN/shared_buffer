/** 
 * \author Abubakr Nada 
 * Last Name: Nada 
 * First Name: Abubakr 
 * Student Number: r0767316   
*/

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdint.h>
#include "lib/dplist.h"
#include <time.h>

typedef uint16_t sensor_id_t;
typedef double sensor_value_t;
typedef time_t sensor_ts_t;
typedef struct
{
    uint16_t id;
    double value;
    long ts;
} sensor_data_t;

typedef struct
{
    char clear_flag;
    void *buffer;
    int reader_thread_id;
    void *logger;
} strgmgr_args;
typedef struct
{
    int pipefd;
    void *fp_sensor_map;
    void *buffer;
    int reader_thread_id;
    void *logger;
} datamgr_args;

typedef struct
{
    int port_number;
    int pipefd;
    void *buffer;
    void *logger;
} conn_args;

#endif /* _CONFIG_H_ */
