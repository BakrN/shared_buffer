/** 
 * \author Abubakr Nada 
 * Last Name: Nada 
 * First Name: Abubakr 
 * Student Number: r0767316   
*/
#ifndef _SENSOR_DB_H_
#define _SENSOR_DB_H_
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include <sqlite3.h>
#include "sbuffer.h"
#include "logger.h"
// stringify preprocessor directives using 2-level preprocessor magic
// this avoids using directives like -DDB_NAME=\"some_db_name\"
#ifndef DB_LOG_SEQUENCE_NO
#define DB_LOG_SEQUENCE_NO 003
#endif
#define REAL_TO_STRING(s) #s
#define TO_STRING(s) REAL_TO_STRING(s) //force macro-expansion on s before stringify s

#ifndef DB_NAME
#define DB_NAME Sensor.db
#endif

#ifndef TABLE_NAME
#define TABLE_NAME SensorData
#endif

#define DBCONN sqlite3
#define DB_SUCCESS 0
#define DB_FAILUIRE -1

typedef int (*callback_t)(void *, int, char **, char **);

typedef struct
{

  log_msg DB_LOG_MSG;   // logmsg
  DBCONN *db;           // SQLite connection
  int reader_thread_id; // This is the unique thread id assigned to the strgmgr
  uint8_t fail_count;   // This variable keeps count of consecutive failed attempts to the sql DB

} STRGMGR_DATA;

void *strgmgr_init(void *args);
/**
 * Make a connection to the database server
 * Create (open) a database with name DB_NAME having 1 table named TABLE_NAME  
 * \param clear_up_flag if the table existed, clear up the existing data when clear_up_flag is set to 1
 * \return storagemgr for success, NULL if an error occurs
 */
STRGMGR_DATA *strmgr_init_connection(char clear_up_flag);

/**
 * Disconnect from the database server
 * \param conn pointer to the current connection
 */
void disconnect(STRGMGR_DATA *strmgr_data);

/**
 * Write an INSERT query to insert a single sensor measurement
 * \param STRGMGR_DATA pointer to the the storage manager information
 * \param id the sensor id
 * \param value the measurement value
 * \param ts the measurement timestamp
 * \return zero for success, and non-zero if an error occurs
 */

int insert_sensor(STRGMGR_DATA *strmgr_data, sensor_id_t id, sensor_value_t value, sensor_ts_t ts);

/**
 * @brief The passed storage manager will listen to the passed buffer
 * 
 * @param strmgr_data 
 * @param buffer 
 * @return int 
 */

int insert_sensor_from_sbuffer(STRGMGR_DATA *strmgr_data, sbuffer_t *buffer);

/**
  * Write a SELECT query to select all sensor measurements in the table 
  * The callback function is applied to every row in the result
  * \param conn pointer to the current connection
  * \param f function pointer to the callback method that will handle the result set
  * \return zero for success, and non-zero if an error occurs
  */
int find_sensor_all(DBCONN *conn, callback_t f);

/**
 * Write a SELECT query to return all sensor measurements having a temperature of 'value'
 * The callback function is applied to every row in the result
 * \param conn pointer to the current connection
 * \param value the value to be queried
 * \param f function pointer to the callback method that will handle the result set
 * \return zero for success, and non-zero if an error occurs
 */
int find_sensor_by_value(DBCONN *conn, sensor_value_t value, callback_t f);

/**
 * Write a SELECT query to return all sensor measurements of which the temperature exceeds 'value'
 * The callback function is applied to every row in the result
 * \param conn pointer to the current connection
 * \param value the value to be queried
 * \param f function pointer to the callback method that will handle the result set
 * \return zero for success, and non-zero if an error occurs
 */
int find_sensor_exceed_value(DBCONN *conn, sensor_value_t value, callback_t f);

/**
 * Write a SELECT query to return all sensor measurements having a timestamp 'ts'
 * The callback function is applied to every row in the result
 * \param conn pointer to the current connection
 * \param ts the timestamp to be queried
 * \param f function pointer to the callback method that will handle the result set
 * \return zero for success, and non-zero if an error occurs
 */
int find_sensor_by_timestamp(DBCONN *conn, sensor_ts_t ts, callback_t f);

/**
 * Write a SELECT query to return all sensor measurements recorded after timestamp 'ts'
 * The callback function is applied to every row in the result
 * \param conn pointer to the current connection
 * \param ts the timestamp to be queried
 * \param f function pointer to the callback method that will handle the result set
 * \return zero for success, and non-zero if an error occurs
 */
int find_sensor_after_timestamp(DBCONN *conn, sensor_ts_t ts, callback_t f);

#endif /* _SENSOR_DB_H_ */
