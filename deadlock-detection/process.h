// CS4760 2/18/23 Bryce Paubel Project 2
// Declarations for process information and process struct definition

#ifndef PROCESS_H
#define PROCESS_H

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

// Maximum number of processes that can be spawned
#define MAX_PROCESSES 18

#define BILLION 1000000000

// Struct for process control blocks
typedef struct {
  int occupied;
  pid_t pid;
  int start_seconds;
  int start_nano;
  int state;
  unsigned long long int time_spent_ready[2];
  unsigned long long int time_spent_blocked[2];
  int total_times_run;
  int total_times_blocked;
  
} PCB;

// Enumerated states
enum {BLOCKED, READY, RUNNING};

// init_process_table
// Initializes a process table with a default value
// 
// Input: PCB*, int
// PCB table to seed values into, value to seed the process table with
// 
// Output: void
void init_process_table(PCB* process_table, int seed_value);


// init_process
// Initializes a given process
//
// Input: PCB*, int
// Process control block to be intialized, seed to use to initialize it
//
// Output: void
void init_process(PCB* process, int seed);

// insert_process
// Inserts a process into the passed PCB table
// 
// Input: PCB*, pid_t, int, int
// PCB table to insert into, process id, start seconds for process, and start nano seconds for process
// 
// Output: int
// C boolean to determine if we could actually insert the process
int insert_process(PCB* process_table, pid_t pid, int start_seconds, int start_nano);

// remove_process
// Removes a process from the passed PCB table
// 
// Input: PCB*, pid_t, int
// PCB table to insert into, process id for process to remove, value to replace (re-seed) the process with
// 
// Output: int
// C boolean to determine if we could actually removed the process
int remove_process(PCB* process_table, pid_t pid, int seed_value);

// update_time_if_billion
// Updates two integers to their appropriate values if nano seconds has gone over a billion
//
// Input: unsigned long long int*, unsigned long long int*
// Seconds pointer, nanoseconds pointer (pointers so we can manipulate the values)
//
// Ouput: void
void update_time_if_billion(unsigned long long int* seconds, unsigned long long int* nano);

// display_pt_and_time
// Displays the processes in the process table and the current system clock
//
// Input: FILE*, PCB*, pid_t, int, int
// File for output, process table consisting of process control blocks, the process id calling the function, system time in seconds, system time in nano seconds
//
// Output: void
void display_pt_and_time(FILE* filename, PCB* process_table, pid_t pid, int seconds, int nano_seconds);

// add_to_blocked_process_time
// updates the time spent blocked in the system for a given process
//
// Input: PCB*, int
// The PID for the process, the time spent in the system to add
//
// Output: void
void add_to_blocked_process_time(PCB* process_table, int number_to_add);

// add_to_ready_process_time
// updates the time spent in the system for a given process
//
// Input: PCB*, int
// The process table and the time spent in the system to add
//
// Output: void
void add_to_ready_process_time(PCB* process_table, int number_to_add);

// set_state
// Sets the state of a given process in the process control block
//
// Input: PCB*, int, int
// The process table, the process id, and the process state to set
//
// Output: int
// Integer boolean determining success
int set_state(PCB* process_table, int pid, int state);

PCB get_process_copy(PCB* process_table, int pid);

int increment_number_of_times_run(PCB* process_table, int pid);

void display_process_table(PCB* process_table);

int increment_number_of_times_blocked(PCB* process_table, int pid);

// kill_all_processes
// Kills all processes in a given process table
//
// Input: PCB*
// Process control table array
//
// Output: void
void kill_all_processes(PCB* process_table);
#endif