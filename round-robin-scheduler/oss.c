// CS4760 Project 4 Bryce Paubel 3/23/23
// OSS scheduler

// NOTE - Since there are shared resources that are cleaned up in error cases,
// these error cases have been tested in Project 3.

// NOTE - I implemented by message system slightly differently.
// A negative value indicates being blocked instead of termination, and a value less than
// the quantum that is positive indicates termination. Slightly different semantics but same end goal.

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
#include <sys/msg.h>
#include <signal.h>
#include <getopt.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "message.h"
#include "process.h"
#include "queue.h"

// Debugging constants (output for this debugging information will not be written to log file)
#define DISPLAY_PROCESS_TABLE 0
#define DISPLAY_QUEUES 0

// Constants for the system to use
#define TOTAL_PROCESSES_TO_SPAWN 100
#define SPAWN_INTERVAL_MAX_NANO_SECONDS 5000000
#define SPAWN_INTERVAL_MAX_SECONDS 0
#define QUANTUM 1000000

/* Global variables needed so signal handler and other functions can have access to them */
PCB process_table[MAX_PROCESSES];            // Global process table
unsigned long long int* sh_clock_ptr = NULL; // Global pointer for shared memory
int sh_clock_id = -1;                        // Global integer for shared memory ID
int msg_id = -1;                             // Global integer for message queue ID
FILE* lfp = NULL;                            // Global file pointer for outputs

// Memory for statistics
unsigned long long int total_time_spent_ready[2] = {0ull, 0ull};                // Total time spent in the ready queue for all processes
unsigned long long int total_time_spent_blocked[2] = {0ull, 0ull};              // Total time spent in the blocked queue for all processes
unsigned long long int total_times_blocked = 0;                                 // Total times the processes were blocked
unsigned long long int total_times_run = 0;                                     // Total times the processes were dispatched
unsigned long long int total_time_spent_idling[2] = {0ull, 0ull};               // Total time the process was idling, i.e. no process was able to run
unsigned long long int total_time_spent_running_processes[2] = {0ull, 0ull};    // Total time the processor was actually running a process
unsigned long long int total_time_spent_running_processes_pcb[2] = {0ull, 0ull};// Total time the processes was actually running a process according to our process table
unsigned long long int total_time_spent_in_system[2] = {0ull, 0ull};            // Total time the processes spent in the system

// Ready and blocked queue pointers
node* ready_head = NULL;    // Ready queue head pointer
node* blocked_head = NULL;  // Blocked queue head pointer

/*
  clean_msq_queue
  Cleans the message queue

  Input: int
  The message queue id

  Output: void
*/
void clean_msg_queue(int id) {
  msgctl(id, IPC_RMID, NULL);
}

/*
  clean_shared_clock
  Cleans the shared clock memory

  Input: int*, int
  Pointer to the shared memory, id for said memory

  Output: void
*/
void clean_shared_clock(unsigned long long int* ptr, int id) {
	shmdt(ptr);
	shmctl(id, IPC_RMID, NULL);
}

/*
  print_statistics
  Prints out statistics to standard output and the file pointer
  
  Input: FILE*, unsigned long long int*, unsigned long long int*, unsigned long long int*, unsigned long long int*, unsigned long long int, unsigned long long int
  File to write the auxiliary output to, the array of (s, ns) for the total time spent blocked, the array of (s, ns) for the total time spent in the ready queue,
  the array of (s, ns) for the total time spent actually running processes,  the array of (s, ns) for the total time spent idling, the total amount of times we had
  to block a process, the total times we had to dispatch a process

*/
void print_statistics(FILE* lfp, unsigned long long int* total_time_spent_blocked, 
  unsigned long long int* total_time_spent_ready, unsigned long long int* total_time_spent_running_processes, unsigned long long int* total_time_spent_running_processes_pcb, unsigned long long int* total_time_spent_idling, unsigned long long int total_times_blocked, unsigned long long int total_times_run) {
  
  // Print the time the system spent running actual processes
  printf("Total time that processes were running according to process table: (%llus, %lluns)\n", total_time_spent_running_processes_pcb[0], total_time_spent_running_processes_pcb[1]);
  printf("Time spent running processes: (%llus, %lluns)\n", total_time_spent_running_processes[0], total_time_spent_running_processes[1]);
  if (lfp != NULL) {
    fprintf(lfp, "Total time that processes were running according to process table: (%llus, %lluns)\n", total_time_spent_running_processes[0], total_time_spent_running_processes[1]);
    fprintf(lfp, "Total times a process was running according to process table: (%llus, %lluns)\n", total_time_spent_running_processes[0], total_time_spent_running_processes[1]);
  }

  // Print number of times blocked
  printf("Time spent blocked: (%llus, %lluns)\n", total_time_spent_blocked[0], total_time_spent_blocked[1]);
  if (lfp != NULL) {
    fprintf(lfp, "Time spent blocked: (%llus, %lluns)\n", total_time_spent_blocked[0], total_time_spent_blocked[1]);
  }
  
  // Print number of times we dispatched a process
  printf("Time spent ready: (%llus, %lluns)\n", total_time_spent_ready[0], total_time_spent_ready[1]);
  if (lfp != NULL) {
    fprintf(lfp, "Time spent ready: (%llus, %lluns)\n", total_time_spent_ready[0], total_time_spent_ready[1]);
  }
  
  // Calculate average wait time (excluding the blocked queue)
  unsigned long long average_wait_time = total_times_run == 0 ? 0 : (total_time_spent_ready[0] * BILLION + total_time_spent_ready[1]) / total_times_run;
  
    // Calculate average wait time (including the blocked queue)
  unsigned long long average_wait_time_including_blocked = total_times_run == 0 ? 0 : (total_time_spent_blocked[0] * BILLION + total_time_spent_blocked[1] + total_time_spent_ready[0] * BILLION + total_time_spent_ready[1]) / total_times_run;
  
  // Calculate average blocked time
  // Processes may never get blocked so we have to check that we don't divide by zero
  unsigned long long int average_time_blocked = total_times_blocked == 0 ? 0 : (total_time_spent_blocked[0] * BILLION + total_time_spent_blocked[1]) / total_times_blocked;
  
  // Calculate CPU utilization
  double process_time_in_sec = total_time_spent_running_processes[0] + (double)total_time_spent_running_processes[1] / BILLION;
  
  double system_time_in_sec = (double)sh_clock_ptr[0] + (double)sh_clock_ptr[1] / BILLION;
  
  double cpu_utilization = process_time_in_sec / system_time_in_sec;
  
  // Print values
  printf("Total times a process was dispatched: %lld\n", total_times_run);
  printf("Total times a process was blocked: %lld\n", total_times_blocked);
  printf("Total time the system spent idling: (%llds, %lldns)\n", total_time_spent_idling[0], total_time_spent_idling[1]);
  printf("Average wait time in the ready queue: %lldns\n", average_wait_time);
  printf("Average wait time in any queue: %lldns\n", average_wait_time_including_blocked);
  printf("Average time in the blocked queue: %lldns\n", average_time_blocked);
  printf("CPU utilization: %f\n", cpu_utilization);
  
  if (lfp != NULL) {
    fprintf(lfp, "Total times a process was dispatched: %lld\n", total_times_run);
    fprintf(lfp, "Total times a process was blocked: %lld\n", total_times_blocked);
    fprintf(lfp, "Total time the system spent idling: (%llds, %lldns)\n", total_time_spent_idling[0], total_time_spent_idling[1]);
    fprintf(lfp, "Average wait time in the ready queue: %lldns\n", average_wait_time);
    fprintf(lfp, "Average wait time in any queue: %lldns\n", average_wait_time);
    fprintf(lfp, "Average time in the blocked queue: %lldns\n", average_time_blocked);
    fprintf(lfp, "CPU utilization: %f\n", cpu_utilization);
  }
  
  return;
}

/*
  update_if_nano_seconds_went_over_billion
  Updates the clock values if the nano seconds in the system clock
  went over a billion
  
  Input: void

  Output: void
*/
void update_if_nano_seconds_went_over_billion() {
  if (sh_clock_ptr != NULL) {
   	if (sh_clock_ptr[1] >= BILLION) {
			sh_clock_ptr[0]++;
			sh_clock_ptr[1] = sh_clock_ptr[1] % BILLION;
 		}
  }
}

// Note that there is a second time check since this will stop if the process gets blocked
/* FUNCTIONS FROM PAGE 318 OF UNIX SYSTEMS PROGRAMMING */

static void my_handler(int s) {
  // Alert the user as to the reason for termination
  char* output = "\nSignal sent, terminating\n\n";
  printf("%s", output);
  if (lfp != NULL) {
    fprintf(lfp, "%s", output);
  }
  
  // Clean up resources
  // OF NOTE - this may cause an error message where a child
  // does not receive a message. This is okay, since we are ending
  // the processes, and they never will receive a message. This occurs
  // since a process may be forked before having its pid in the process table
  kill_all_processes(process_table);
  if (msg_id != -1) {
    clean_msg_queue(msg_id);
  }
  
  // Extract any information left from current processes that never completed
  // that were left in the ready queue
  int ready_pid = -1;
  while ((ready_pid = queue_remove(&ready_head)) > -1) {
    PCB finished_process = get_process_copy(process_table, ready_pid);
    
    total_times_run += finished_process.total_times_run;
    total_times_blocked += finished_process.total_times_blocked;
  
    total_time_spent_blocked[0] += finished_process.time_spent_blocked[0];
    total_time_spent_blocked[1] += finished_process.time_spent_blocked[1];
    update_time_if_billion(&total_time_spent_blocked[0], &total_time_spent_blocked[1]);
  
    total_time_spent_ready[0] += finished_process.time_spent_ready[0];
    total_time_spent_ready[1] += finished_process.time_spent_ready[1];
    update_time_if_billion(&total_time_spent_ready[0], &total_time_spent_ready[1]);
    
    total_time_spent_running_processes_pcb[0] += finished_process.time_spent_running[0];
    total_time_spent_running_processes_pcb[1] += finished_process.time_spent_running[1];
    update_time_if_billion(&total_time_spent_running_processes_pcb[0], &total_time_spent_running_processes_pcb[1]);
    
    remove_process(process_table, ready_pid, 0);
  }
  
  // Extract any information left from current processes that never completed
  // that were left in the blocked queue
  int blocked_pid = -1;
  while ((blocked_pid = queue_remove(&blocked_head)) > -1) {
    PCB finished_process = get_process_copy(process_table, blocked_pid);
    
    total_times_run += finished_process.total_times_run;
    total_times_blocked += finished_process.total_times_blocked;
  
    total_time_spent_blocked[0] += finished_process.time_spent_blocked[0];
    total_time_spent_blocked[1] += finished_process.time_spent_blocked[1];
    update_time_if_billion(&total_time_spent_blocked[0], &total_time_spent_blocked[1]);
  
    total_time_spent_ready[0] += finished_process.time_spent_ready[0];
    total_time_spent_ready[1] += finished_process.time_spent_ready[1];
    update_time_if_billion(&total_time_spent_ready[0], &total_time_spent_ready[1]);
    
    total_time_spent_running_processes_pcb[0] += finished_process.time_spent_running[0];
    total_time_spent_running_processes_pcb[1] += finished_process.time_spent_running[1];
    update_time_if_billion(&total_time_spent_running_processes_pcb[0], &total_time_spent_running_processes_pcb[1]);
    
    remove_process(process_table, blocked_pid, 0);
  }
  
  // Print out statistics for the processes that have finished
  print_statistics(lfp, total_time_spent_blocked, total_time_spent_ready, total_time_spent_running_processes, total_time_spent_running_processes_pcb, total_time_spent_idling, total_times_blocked, total_times_run);
  
  // Clean the shared memory since we do not need it anymore
  if (sh_clock_ptr != NULL) {
    clean_shared_clock(sh_clock_ptr, sh_clock_id);
  }
  
  // Exit
  exit(EXIT_SUCCESS);
}

static int setup_interrupt() {
  struct sigaction act;
  act.sa_handler = my_handler;
  act.sa_flags = 0;
  
  int bool1 = sigemptyset(&act.sa_mask);
  int bool2 = sigaction(SIGPROF, &act, NULL);
  int bool3 = sigaction(SIGINT, &act, NULL);
  return bool1 || bool2 || bool3;
}

static int setup_it_timer() {
  struct itimerval value;
  value.it_interval.tv_sec = 3;
  value.it_interval.tv_usec = 0;
  value.it_value = value.it_interval;
  return setitimer(ITIMER_PROF, &value, NULL);
}

/* END UNIX SYSTEMS PROGRAMMING FUNCTIONS */


// Heavily based on a help function I wrote in CS2750
/*
  help
  displays a help message to the parameter provided

  Input: FILE*
  Where to send output

  Output: void
*/
void help(FILE* f) {
	fprintf(f, "\n\tUSAGE: [-h] [-f logfile]\n");		
  fprintf(f, "\t\t-f    file: Name for logfile\n\n");
}

/*
  error
  displays a given error messages and terminates the program

  Input: char*, int, FILE*
  A message for the error, boolean for if we also need help message, logfile to output to

  Output: void
*/
void error(char* error_message, int display_help, FILE* f) {

  if (f != NULL) {
 	  fprintf(f, "\nERROR: %s\n", error_message);
  }
  
  fprintf(stdout, "\nERROR: %s\n", error_message);
	
	if (display_help) {
    help(stdout);
    
    if (f != NULL) {
		  help(f);
    }
	}
	
  // Close the given output file and exit
  if (f != NULL) {
    fclose(f);
  }
 
	exit(EXIT_FAILURE);
}

/******************* MAIN *******************/
int main(int argc, char** argv) {
  // Number of lines written to log file (error messages are not counted since they immediately end the program)
  int num_of_lines = 0;

  // Initial time when the program started
  time_t init_time = time(0);

  
  // Setting up the signal handlers
  if (setup_interrupt() == -1) {
    error("UNABLE TO SET UP INTERRUPT HANDLER", 0, lfp);
  }
  if (setup_it_timer() == -1) {
    error("UNABLE TO SET UP TIMER", 0, lfp);
  }
  
  // Seed the random values
  srand(time(0));
  
  // Get the keys for shared memory and message queues
  const int sh_clock_key = ftok("oss.c", 0);
  const int m_queue_key = ftok("oss.c", 0);
  
  init_process_table(process_table, 0);
  
  // File information
  int isLogFile = 0;
  char* filename = "";
  
  // Parse the input
  char opt;
  while ((opt = getopt(argc, argv, "hf:")) != -1) {
    switch (opt) {
      case 'h':
        help(stdout);
        exit(EXIT_SUCCESS);
      case 'f':
        isLogFile = 1;
        filename = optarg;
        break;
      default:
        error("UKNOWN OPTION OR ARGUMENT MISSING", 1, lfp);
        break;
    }
  }
  
  // Open the logfile
  lfp = NULL;
  if (isLogFile) {
    lfp = fopen(filename, "w");
    if (lfp == NULL) {
      error("UNABLE TO OPEN THE LOGFILE", 0, lfp);
    }
  }
  
  // Allocate a message queue
  msg_id = msgget(m_queue_key, IPC_CREAT | 0777);
  if (msg_id < 0) {
    char* err_msg = "FAILED TO ALLOCATE A MESSAGE QUEUE";
    error(err_msg, 0, lfp);
  }
  
  // Allocate the shared memory
  sh_clock_id = shmget(sh_clock_key, sizeof(unsigned long long int) * 2, IPC_CREAT | 0777);
  if (sh_clock_id <= 0) {
    char* err_msg = "FAILED TO ALLOCATE SHARED MEMORY";
    clean_msg_queue(msg_id);
    error(err_msg, 0, lfp);
  }
  
  // Get the shared memory pointer
  sh_clock_ptr = shmat(sh_clock_id, 0, 0);
  if (sh_clock_ptr <= 0) {
    char* err_msg = "UNABLE TO ATTACH TO SHARED MEMORY";
    clean_msg_queue(msg_id);
    error(err_msg, 0, lfp);
  }
  
  // Initialize the clock values
  sh_clock_ptr[0] = 0ull;
  sh_clock_ptr[1] = 0ull;
  
  // Set information for processes
  int total_processes_finished = 0;
  int total_processes = 0;
  int executing_processes = 0;
  
  // Set the initial spawn interval and times
  unsigned long long int rand_spawn_sec = rand() % (SPAWN_INTERVAL_MAX_SECONDS + 1);
  unsigned long long int rand_spawn_nano = rand() % (SPAWN_INTERVAL_MAX_NANO_SECONDS + 1);
  unsigned long long int spawn_time_sec = rand_spawn_sec;
  unsigned long long int spawn_time_nano = rand_spawn_nano;
  
  /* MAIN LOOP */
  while (total_processes_finished < TOTAL_PROCESSES_TO_SPAWN) {
    // This is not a perfect solution
    // However, since we have blocking waits our timer
    // is not reliable, so we rely on the time check done here
    // since this is separate from the current process which isn't blocked
    if (time(0) - init_time >= 3) {
      printf("\nThree seconds have passed, ending process\n");
      if (lfp != NULL) {
        fprintf(lfp, "\nThree seconds have passed, ending system simulation\n");
      }
      kill(getpid(), SIGPROF);
    } 
    
    if (num_of_lines >= 10000) {
      printf("\nMore than 10,000 lines in output, ending system simulation\n");
      if (lfp != NULL) {
        fprintf(lfp, "\nMore than 10,000 lines in output, ending system simulation\n");
      }
      kill(getpid(), SIGPROF);
    }
    
    /* PROCESS SPAWNING IN BETWEEN PROCESSES */
    // Spawn a process if we have passed our spawn time (all while abiding by process constraints)
    if (sh_clock_ptr[0] >= spawn_time_sec && sh_clock_ptr[1] >= spawn_time_nano && executing_processes < MAX_PROCESSES && total_processes < TOTAL_PROCESSES_TO_SPAWN) {
      
      // Set a new spawn interval
      rand_spawn_sec = rand() % (SPAWN_INTERVAL_MAX_SECONDS + 1);
      rand_spawn_nano = rand() % (SPAWN_INTERVAL_MAX_NANO_SECONDS + 1);

      spawn_time_sec = rand_spawn_sec + sh_clock_ptr[0];
      spawn_time_nano = rand_spawn_nano + sh_clock_ptr[1];

      update_time_if_billion(&spawn_time_sec, &spawn_time_nano);
      
      int cpid = fork();
      
      // If we are a child, exec
      if (cpid == 0) {
        char* command = "./worker";
        char* arguments[] = {command, NULL};
        execvp(command, arguments);
        
        error("FAILED TO EXEC IN THE CHILD", 0, NULL);
        
      // If we are a parent, update the PCB and insert the process into the ready queue
      } else if (cpid > 0) {
        num_of_lines++;
        // Output to stdout and file
        printf("OSS: Spawned a new process and inserted it into the ready queue, PID: %d, System time: %llu %llu\n", cpid, sh_clock_ptr[0], sh_clock_ptr[1]);
        if (lfp != NULL) {
          fprintf(lfp, "OSS: Spawned a new process and inserted it into the ready queue, PID: %d, System time: %llu %llu\n", cpid, sh_clock_ptr[0], sh_clock_ptr[1]);
        }
        
        num_of_lines++;
        printf("OSS: Will spawn new process when it has control at some time after: %llu, %llu if there is space. If there is not space, it will be spawned as as soon as there is space. System time: %llu, %llu\n", spawn_time_sec, spawn_time_nano, sh_clock_ptr[0], sh_clock_ptr[1]);
        if (lfp != NULL) {
          fprintf(lfp, "OSS: Will spawn new process when it has control at some time after: %llu, %llu if there is space. If there is not space, it will be spawned as as soon as there is space. System time: %llu, %llu\n", spawn_time_sec, spawn_time_nano, sh_clock_ptr[0], sh_clock_ptr[1]);
        }
      
        /* SPAWN A PROCESS, TAKES 10000ns */
        // Increment the clock, overhead for the long-term scheduler to insert a process
        // Update the process control blocks with this information
        int time_taken_to_insert_process = 10000;
        add_to_ready_process_time(process_table, time_taken_to_insert_process);
        add_to_blocked_process_time(process_table, time_taken_to_insert_process);
        sh_clock_ptr[1] += time_taken_to_insert_process;
        update_if_nano_seconds_went_over_billion();
        
        int insert_sucess = insert_process(process_table, cpid, sh_clock_ptr[0], sh_clock_ptr[1]);
        add_to_total_time_in_system(process_table, time_taken_to_insert_process);
        set_state(process_table, cpid, READY);
        queue_insert(&ready_head, cpid, 0, 0);
        
        if (!insert_sucess) {
          dealloc_queue(&ready_head);
          kill_all_processes(process_table);
          kill(cpid, SIGKILL);
          clean_msg_queue(msg_id);
          clean_shared_clock(sh_clock_ptr, sh_clock_id);
          
          char* err_msg = "UNABLE TO INSERT ANOTHER PROCESS INTO THE PROCESS TABLE";
          error(err_msg, 0, lfp);
        }
      
      // Error case, fork failed
      } else {
        dealloc_queue(&ready_head);
        kill_all_processes(process_table);
    		clean_shared_clock(sh_clock_ptr, sh_clock_id);
        clean_msg_queue(msg_id);
        char* err_msg = "FAILED TO FORK";
    		error(err_msg, 0, lfp);
      }
      
      // Update process information
      total_processes++;
      executing_processes++;
    }
    
    /* WAKE UP ANY UNBLOCKED PROCESSES, ASSUME THIS TAKES 1000ns TOPS */
    int blocked_pid;
    while (0 < (blocked_pid = queue_remove_blocked(&blocked_head, sh_clock_ptr[0], sh_clock_ptr[1]))) {
      num_of_lines++;
      printf("OSS: Waking up %d from the blocked queue and putting it into the ready queue, System time: %llu %llu\n", blocked_pid, sh_clock_ptr[0], sh_clock_ptr[1]);
      if (lfp != NULL) {
        fprintf(lfp, "OSS: Waking up %d from the blocked queue and putting it into the ready queue, System time: %llu %llu\n", blocked_pid, sh_clock_ptr[0], sh_clock_ptr[1]);
      }
      set_state(process_table, blocked_pid, READY);
      queue_insert(&ready_head, blocked_pid, 0, 0);
      
      // Update the time taken to wake up a blocked process and put it in the ready queue
      int time_taken_to_wake_up_blocked_processes = 1000;
      add_to_blocked_process_time(process_table, time_taken_to_wake_up_blocked_processes);
      add_to_ready_process_time(process_table, time_taken_to_wake_up_blocked_processes);
      add_to_total_time_in_system(process_table, time_taken_to_wake_up_blocked_processes);
      sh_clock_ptr[1] += time_taken_to_wake_up_blocked_processes;
      update_if_nano_seconds_went_over_billion();
    }
    
    /* SCHEDULE A PROCESS FROM THE FRONT OF THE READY QUEUE, TAKES 1000ns */
    // Update the time taken to schedule this process
    int time_taken_to_schedule_a_process = 100;
    add_to_blocked_process_time(process_table, time_taken_to_schedule_a_process);
    add_to_ready_process_time(process_table, time_taken_to_schedule_a_process);
    add_to_total_time_in_system(process_table, time_taken_to_schedule_a_process);
    sh_clock_ptr[1] += time_taken_to_schedule_a_process;
    update_if_nano_seconds_went_over_billion();
    
    int pid_to_schedule = queue_remove(&ready_head);
    set_state(process_table, pid_to_schedule, RUNNING);
    increment_number_of_times_run(process_table, pid_to_schedule);
    
    /* IF THERE IS NOTHING TO SCHEDULE, DO NOTHING, TAKES 10000ns */
    if (pid_to_schedule < 0){
      
      // Update the clock idle time
      unsigned long long int time_taken_while_idling = 10000;
      total_time_spent_idling[1] += time_taken_while_idling;
      update_time_if_billion(&total_time_spent_idling[0], &total_time_spent_idling[1]);
      
      add_to_blocked_process_time(process_table, time_taken_while_idling);
      add_to_ready_process_time(process_table, time_taken_while_idling);
      add_to_total_time_in_system(process_table, time_taken_while_idling);
      sh_clock_ptr[1] += time_taken_while_idling;
      update_if_nano_seconds_went_over_billion();
      
      // If we've finished all our processes, end
      if (total_processes_finished >= TOTAL_PROCESSES_TO_SPAWN) {
        break;
      }
      
      num_of_lines++;
      printf("OSS: No processes to schedule in the ready queue, scheduling the scheduler %d for %lluns, System time: %llu %llu\n", getppid(), time_taken_while_idling, sh_clock_ptr[0], sh_clock_ptr[1]);
      if (lfp != NULL) {
        fprintf(lfp, "OSS: No processes to schedule in the ready queue, scheduling the scheduler %d for %lluns, System time: %llu %llu\n", getppid(), time_taken_while_idling, sh_clock_ptr[0], sh_clock_ptr[1]);
      }
      
      // Still display debugging info
      if (DISPLAY_PROCESS_TABLE) {
        display_process_table(process_table);
      }
      if (DISPLAY_QUEUES) {
        display_queue(&ready_head);
        display_queue(&blocked_head);
      }
      
      // Skip the rest of the iteration since idle
      continue;
    } else {    
      num_of_lines++;
      printf("OSS: Scheduling %d, System time: %llu %llu\n", pid_to_schedule, sh_clock_ptr[0], sh_clock_ptr[1]);
      if (lfp != NULL) {
        fprintf(lfp, "OSS: Scheduling %d, System time: %llu %llu\n", pid_to_schedule, sh_clock_ptr[0], sh_clock_ptr[1]);
      }
    }
    
    // Debugging outputs (NOT WRITTEN TO LOG FILE)
    if (DISPLAY_PROCESS_TABLE) {
      display_process_table(process_table);
    }
    if (DISPLAY_QUEUES) {
      display_queue(&ready_head);
      display_queue(&blocked_head);
    }
        
    /* MSGSND MSGRCV SIMULATING PROCESS SWAPPING, TAKES 100000ns */
    message dispatch_message;
    dispatch_message.mtype = pid_to_schedule;
    dispatch_message.nano_seconds = QUANTUM;
    
    if (msgsnd(msg_id, &dispatch_message, sizeof(message) - sizeof(long), 0) < 0) {
      dealloc_queue(&ready_head);
      kill_all_processes(process_table);
      clean_shared_clock(sh_clock_ptr, sh_clock_id);
      clean_msg_queue(msg_id);
      
      char* err_msg = "UNABLE TO SEND MESSAGE";
      error(err_msg, 0, lfp);
    }
    
    message received_message;
    if (msgrcv(msg_id, &received_message, sizeof(message), getpid(), 0) < 0) {
      dealloc_queue(&ready_head);
      kill_all_processes(process_table);
      clean_shared_clock(sh_clock_ptr, sh_clock_id);
      clean_msg_queue(msg_id);
      
      error("FAILED TO GET MESSAGE BACK FROM DISPATCHED PROCESS", 0, lfp);
    }
    
    // Time it took to swap out processes
    int time_taken_to_swap_out_processes = 100000;
    add_to_blocked_process_time(process_table, time_taken_to_swap_out_processes);
    add_to_ready_process_time(process_table, time_taken_to_swap_out_processes);
    add_to_total_time_in_system(process_table, time_taken_to_swap_out_processes);
    sh_clock_ptr[1] += time_taken_to_swap_out_processes;
    update_if_nano_seconds_went_over_billion();
    
    // Time quantum used by the process
    unsigned long long int time_taken_by_process = abs(received_message.nano_seconds);
    total_time_spent_running_processes[1] += time_taken_by_process;
    update_time_if_billion(&total_time_spent_running_processes[0], & total_time_spent_running_processes[1]);
    
    add_to_blocked_process_time(process_table, time_taken_by_process);
    add_to_ready_process_time(process_table, time_taken_by_process);
    add_to_total_time_in_system(process_table, time_taken_by_process);
    add_to_total_time_spent_running(process_table, time_taken_by_process, pid_to_schedule);
    sh_clock_ptr[1] += time_taken_by_process;
    update_if_nano_seconds_went_over_billion();
    
    /* ACTION TAKEN DEPENDS ON INFORMATION RECEIVED FROM CHILD */
    if (received_message.nano_seconds > 0 && received_message.nano_seconds < QUANTUM) {
      // Need to include a wait here to make sure
      // we don't fork over our limit, we may fork while
      // the process is still in the system
      num_of_lines++;
      printf("OSS: Process %d ended, Quantum used: %d, System time: %llu %llu\n", pid_to_schedule, received_message.nano_seconds, sh_clock_ptr[0], sh_clock_ptr[1]);
      if (lfp != NULL) {
         fprintf(lfp, "OSS: Process %d ended, Quantum used: %d, System time: %llu %llu\n", pid_to_schedule, received_message.nano_seconds, sh_clock_ptr[0], sh_clock_ptr[1]);
      }
      
      wait(NULL);
      total_processes_finished++;
      executing_processes--;
      
      PCB finished_process = get_process_copy(process_table, pid_to_schedule);
      
      total_times_run += finished_process.total_times_run;
      total_times_blocked += finished_process.total_times_blocked;

      total_time_spent_blocked[0] += finished_process.time_spent_blocked[0];
      total_time_spent_blocked[1] += finished_process.time_spent_blocked[1];
      update_time_if_billion(&total_time_spent_blocked[0], &total_time_spent_blocked[1]);

      total_time_spent_ready[0] += finished_process.time_spent_ready[0];
      total_time_spent_ready[1] += finished_process.time_spent_ready[1];
      update_time_if_billion(&total_time_spent_ready[0], &total_time_spent_ready[1]);
            
      total_time_spent_running_processes_pcb[0] += finished_process.time_spent_running[0];
      total_time_spent_running_processes_pcb[1] += finished_process.time_spent_running[1];
      update_time_if_billion(&total_time_spent_running_processes_pcb[0], &total_time_spent_running_processes_pcb[1]);
      
      remove_process(process_table, pid_to_schedule, 0);
      
    } else if (received_message.nano_seconds < 0) { 
      int temp_sec = sh_clock_ptr[0];
      int temp_nano = sh_clock_ptr[1];
      
      
      temp_sec = (int)(rand() % 5 + 1) + temp_sec;
      temp_nano = (int) (rand() % 1000 + 1) + temp_nano;
      
  		if (temp_nano >= BILLION) {
  			temp_sec++;
  			temp_nano = temp_nano % BILLION;
 		  }
     
       
      num_of_lines++; 
      printf("OSS: Blocking process %d until: %d %d, Quantum used: %d, System time: %llu %llu\n", pid_to_schedule, temp_sec, temp_nano, abs(received_message.nano_seconds), sh_clock_ptr[0], sh_clock_ptr[1]);
      if (lfp != NULL) {
        fprintf(lfp, "OSS: Blocking process %d until: %d %d, Quantum used: %d, System time: %llu %llu\n", pid_to_schedule, temp_sec, temp_nano, abs(received_message.nano_seconds), sh_clock_ptr[0], sh_clock_ptr[1]);
      }
      
      set_state(process_table, pid_to_schedule, BLOCKED);
      increment_number_of_times_blocked(process_table, pid_to_schedule);
      
      queue_insert(&blocked_head, pid_to_schedule, temp_sec, temp_nano);
    } else {
      num_of_lines++;
      printf("OSS: Process %d used %dns of its quantum, System time: %llu, %llu\n", pid_to_schedule, received_message.nano_seconds, sh_clock_ptr[0], sh_clock_ptr[1]);
      if (lfp != NULL) {
        fprintf(lfp, "OSS: Process %d used %dns of its quantum, System time: %llu, %llu\n", pid_to_schedule, received_message.nano_seconds, sh_clock_ptr[0], sh_clock_ptr[1]);
      }
      
      set_state(process_table, pid_to_schedule, READY);
      queue_insert(&ready_head, pid_to_schedule, 0, 0);
    }
  }
  
  // Print out the ending message
  printf("\n%d PROCESSES HAVE COMPLETED IN THE SYSTEM\n\n", TOTAL_PROCESSES_TO_SPAWN);
  if (lfp != NULL) {
    fprintf(lfp, "\n%d PROCESSES HAVE COMPLETED IN THE SYSTEM\n\n", TOTAL_PROCESSES_TO_SPAWN);
  }
  
  // Print out the system statistics
  print_statistics(lfp, total_time_spent_blocked, total_time_spent_ready, total_time_spent_running_processes, total_time_spent_running_processes_pcb, total_time_spent_idling, total_times_blocked, total_times_run);
  
  // Clean up resources
  kill_all_processes(process_table);
  dealloc_queue(&ready_head);
 	clean_shared_clock(sh_clock_ptr, sh_clock_id);
  clean_msg_queue(msg_id);
  if (lfp != NULL) {
    fclose(lfp);
  }
 	return EXIT_SUCCESS;
}