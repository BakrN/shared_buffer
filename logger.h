#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <malloc.h> 
typedef struct {
    uint32_t sequence_number; 
    char* timestamp; 
    char* message; 
} log_msg; 

static int log_fd; 


int log_init();// Fork Process 

void log_event(int write_fd, log_msg* event); 
void log_destroy(); 
