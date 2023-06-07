// CS4760 2/18/23 Bryce Paubel Project 2(update for Project 4)
// Functions and definitions for working with processes

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <signal.h>
#include <getopt.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "process.h"

// init_process_table
// Initializes a process table with a default value
// 
// Input: PCB*, int
// PCB table to seed values into, value to seed the process table with
// 
// Output: void
void init_process_table(PCB* process_table, int seed_value) {
  for (int i = 0; i < MAX_PROCESSES; i++) {
      process_table[i].occupied = seed_value;
      process_table[i].pid = seed_value;
      process_table[i].start_seconds = seed_value;
      process_table[i].start_nano = seed_value;
      process_table[i].time_spent_blocked[0] = seed_value;
      process_table[i].time_spent_blocked[1] = seed_value;
      process_table[i].time_spent_ready[0] = seed_value;
      process_table[i].time_spent_ready[1] = seed_value;
      process_table[i].time_spent_in_system[0] = seed_value;
      process_table[i].time_spent_in_system[1] = seed_value;
      process_table[i].time_spent_running[0] = seed_value;
      process_table[i].time_spent_running[1] = seed_value;
      process_table[i].state = -1;
      process_table[i].total_times_run = seed_value;
      process_table[i].total_times_blocked = seed_value;
  }
}

// init_process
// Initializes a given process
//
// Input: PCB*, int
// Process control block to be intialized, seed to use to initialize it
//
// Output: void
void init_process(PCB* process, int seed) {
  process->occupied = seed;
  process->pid = seed;
  process->start_seconds = seed;
  process->start_nano = seed;
  process->time_spent_blocked[0] = seed;
  process->time_spent_blocked[1] = seed;
  process->time_spent_ready[0] = seed;
  process->time_spent_ready[1] = seed;
  process->time_spent_in_system[0] = seed;
  process->time_spent_in_system[1] = seed;
  process->time_spent_running[0] = seed;
  process->time_spent_running[1] = seed;
  process->state = -1;
  process->total_times_run = seed;
  process->total_times_blocked = seed;
}

// insert_process
// Inserts a process into the passed PCB table
// 
// Input: PCB*, pid_t, int, int
// PCB table to insert into, process id, start seconds for process, and start nano seconds for process
// 
// Output: int
// C boolean to determine if we could actually insert the process
int insert_process(PCB* process_table, pid_t pid, int start_seconds, int start_nano) {
  int did_we_make_an_insert = 0;
  for (int i = 0; i < MAX_PROCESSES; i++) {
    if (!process_table[i].occupied) {
      process_table[i].occupied = 1;
      process_table[i].pid = pid;
      process_table[i].start_seconds = start_seconds;
      process_table[i].start_nano = start_nano;
      process_table[i].time_spent_blocked[0] = 0;
      process_table[i].time_spent_blocked[1] = 0;
      process_table[i].time_spent_ready[0] = 0;
      process_table[i].time_spent_ready[1] = 0;
      process_table[i].time_spent_in_system[0] = 0;
      process_table[i].time_spent_in_system[1] = 0;
      process_table[i].time_spent_running[0] = 0;
      process_table[i].time_spent_running[1] = 0;
      process_table[i].state = READY;
      process_table[i].total_times_run = 0;
      process_table[i].total_times_blocked = 0;
      did_we_make_an_insert = 1;
      break;
    }
  }
  return did_we_make_an_insert;
}

// remove_process
// Removes a process from the passed PCB table
// 
// Input: PCB*, pid_t, int
// PCB table to insert into, process id for process to remove, value to replace (re-seed) the process with
// 
// Output: int
// C boolean to determine if we could actually removed the process
int remove_process(PCB* process_table, pid_t pid, int seed_value) {
  int did_we_make_a_removal = 0;
  for (int i = 0; i < MAX_PROCESSES; i++) {
    if (process_table[i].pid == pid) {
      process_table[i].occupied = 0;    // note - not the seed value, since we must keep this false regardless of the user
      process_table[i].pid = seed_value;
      process_table[i].time_spent_blocked[0] = seed_value;
      process_table[i].time_spent_blocked[1] = seed_value;
      process_table[i].time_spent_ready[0] = seed_value;
      process_table[i].time_spent_ready[1] = seed_value;
      process_table[i].time_spent_in_system[0] = seed_value;
      process_table[i].time_spent_in_system[1] = seed_value;
      process_table[i].time_spent_running[0] = seed_value;
      process_table[i].time_spent_running[1] = seed_value;
      process_table[i].state = -1;
      process_table[i].total_times_run = seed_value;
      process_table[i].total_times_blocked = seed_value;
      did_we_make_a_removal = 1;
      break;
    }
  }
  return did_we_make_a_removal;
}

// display_process_table
// Displays the processes in the process table
//
// Input: PCB*
// Process table consisting of process control blocks
//
// Output: void
void display_process_table(PCB* process_table) {
  printf("Process Table:\n");
  printf("Entry    Occupied PID     Run     Blocked  State   Time Ready     Time Blocked    Time in System    Time Spent Running\n");
  for (int i = 0; i < MAX_PROCESSES; i++) {
    printf("%-8d %-8d %-8d %-8d %-8d %-8d %-llu, %-8llu %-llu, %-8llu %-llu, %-8llu %-llu, %-8llu\n", i, process_table[i].occupied, process_table[i].pid, process_table[i].total_times_run, process_table[i].total_times_blocked, process_table[i].state, process_table[i].time_spent_ready[0], process_table[i].time_spent_ready[1], process_table[i].time_spent_blocked[0], process_table[i].time_spent_blocked[1], process_table[i].time_spent_in_system[0], process_table[i].time_spent_in_system[1],process_table[i].time_spent_running[0],process_table[i].time_spent_running[1]);
  }
}

// add_to_blocked_process_time
// updates the time spent blocked in the system for a given process
//
// Input: PCB*, int
// The PID for the process, the time spent in the system to add
//
// Output: void
void add_to_blocked_process_time(PCB* process_table, int time) {
  for (int i = 0; i < MAX_PROCESSES; i++) {
    if (process_table[i].state == BLOCKED) {
      process_table[i].time_spent_blocked[1] += time;
      
      update_time_if_billion(&process_table[i].time_spent_blocked[0], &process_table[i].time_spent_blocked[1]);
    }
  }
  
  return;
}

// add_to_ready_process_time
// updates the time spent in the system for a given process
//
// Input: PCB*, int
// The process table and the time spent in the system to add
//
// Output: void
void add_to_ready_process_time(PCB* process_table, int time) {
  for (int i = 0; i < MAX_PROCESSES; i++) {
    if (process_table[i].state == READY) {
      process_table[i].time_spent_ready[1] += time;
      
      update_time_if_billion(&process_table[i].time_spent_ready[0], &process_table[i].time_spent_ready[1]);
    }
  }
  
  return;
}

// add_to_total_time_in_system
// Adds more to total time in the system
//
// Input: PCB*, int
// The process table and the time spent in the system to add
//
// Output: void
void add_to_total_time_in_system(PCB* process_table, int number_to_add) {
  for (int i = 0; i < MAX_PROCESSES; i++) {
    if (process_table[i].occupied == 1) {
      process_table[i].time_spent_in_system[1] += number_to_add;
      
      update_time_if_billion(&process_table[i].time_spent_in_system[0], &process_table[i].time_spent_in_system[1]);
    }
  }
  
  return;
}

// add_to_total_time_spent_running
// Adds more to time to the time spent running
//
// Input: PCB*, int
// The process table and the time spent in the system to add
//
// Output: void
void add_to_total_time_spent_running(PCB* process_table, int number_to_add, int pid) {
  for (int i = 0; i < MAX_PROCESSES; i++) {
    if (process_table[i].pid == pid) {
      process_table[i].time_spent_running[1] += number_to_add;
      
      update_time_if_billion(&process_table[i].time_spent_running[0], &process_table[i].time_spent_running[1]);
    }
  }
  
  return;
}

// increment_number_of_times_run
// Increments the number of times run for a given process in the process table
//
// Input: PCB*, int
// Process table, pid of the system to increment
//
// Output: int
// Succcess/failure integer boolean
int increment_number_of_times_run(PCB* process_table, int pid) {
  for (int i = 0; i < MAX_PROCESSES; i++) {
    if (process_table[i].pid == pid) {
      process_table[i].total_times_run++;
      return 1;
    }
  }
  
  return 0;
}

// increment_number_of_times_blocked
// Increments the number of times blocked for a given process in the process table
//
// Input: PCB*, int
// Process table, pid of the system to increment
//
// Output: int
// Succcess/failure integer boolean
int increment_number_of_times_blocked(PCB* process_table, int pid) {
  for (int i = 0; i < MAX_PROCESSES; i++) {
    if (process_table[i].pid == pid) {
      process_table[i].total_times_blocked++;
      return 1;
    }
  }
  
  return 0;
}

// get_process_copy
// Returns a copy of a process control block in the process table
//
// Input: PCB*, int
// Process table, pid of the process to copy
//
// Output: PCB
// The copied process control block
PCB get_process_copy(PCB* process_table, int pid) {
  PCB temp_pcb;
  
  init_process(&temp_pcb, -1);
  
  for (int i = 0; i < MAX_PROCESSES; i++) {
    if (process_table[i].pid == pid) {
      temp_pcb.occupied = process_table[i].occupied;
      temp_pcb.pid = process_table[i].pid;
      temp_pcb.start_seconds = process_table[i].start_seconds;
      temp_pcb.start_nano = process_table[i].start_nano;
      temp_pcb.state = process_table[i].state;
      temp_pcb.time_spent_ready[0] = process_table[i].time_spent_ready[0];
      temp_pcb.time_spent_ready[1] = process_table[i].time_spent_ready[1];
      temp_pcb.time_spent_blocked[0] = process_table[i].time_spent_blocked[0];
      temp_pcb.time_spent_blocked[1] = process_table[i].time_spent_blocked[1];
      temp_pcb.time_spent_in_system[0] = process_table[i].time_spent_in_system[0];
      temp_pcb.time_spent_in_system[1] = process_table[i].time_spent_in_system[1];
      temp_pcb.time_spent_running[0] = process_table[i].time_spent_running[0];
      temp_pcb.time_spent_running[1] = process_table[i].time_spent_running[1];
      temp_pcb.total_times_run = process_table[i].total_times_run;
      temp_pcb.total_times_blocked = process_table[i].total_times_blocked;
      return temp_pcb;
    }
  }
  
  return temp_pcb;
}


// update_time_if_billion
// Updates two integers to their appropriate values if nano seconds has gone over a billion
//
// Input: unsigned long long int*, unsigned long long int*
// Seconds pointer, nanoseconds pointer (pointers so we can manipulate the values)
//
// Ouput: void
void update_time_if_billion(unsigned long long int* seconds, unsigned long long int* nano) {
  if (*nano >= BILLION) {
    (*seconds)++;
    *nano = (*nano) % BILLION;
  }
}

// set_state
// Sets the state of a given process in the process control block
//
// Input: PCB*, int, int
// The process table, the process id, and the process state to set
//
// Output: int
// Integer boolean determining success
int set_state(PCB* process_table, int pid, int state) {
  if (state > 3 || state < 0) {
    return 0;
  }
  
  for (int i = 0; i < MAX_PROCESSES; i++) {
    if (process_table[i].pid == pid) {
      process_table[i].state = state;
      return 1;
    }
  }
  
  return 0;
}

// kill_all_processes
// Kills all processes in a given process table
//
// Input: PCB*
// A process control table
//
// Output: void
void kill_all_processes(PCB* process_table) {
  for (int i = 0; i < MAX_PROCESSES; i++) {
    if (process_table[i].occupied) {
      kill(process_table[i].pid, SIGKILL);
    } 
  }
}