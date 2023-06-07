// CS4760 Project 5 Bryce Paubel 4/18/23
// OSS resource management and deadlock detection

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
#include <sys/stat.h>
#include <signal.h>
#include <getopt.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <semaphore.h>
#include <fcntl.h>
#include <pthread.h>

// Extra semaphore info found here:
// https://stackoverflow.com/questions/32205396/share-posix-semaphore-among-multiple-processes

#include "message.h"
#include "process.h"
#include "queue.h"
#include "memory_tables.h"

// Debugging constants (output for this debugging information will not be written to a log file since its functions are delegated to other files)
#define DISPLAY_PROCESS_TABLE 0
#define DISPLAY_QUEUES 0
#define VERBOSE 1
#define PRINT_WAITING_INFORMATION 0

// Constants for the system to use
#define TOTAL_PROCESSES_TO_SPAWN 100
#define SPAWN_INTERVAL_MAX_NANO_SECONDS 500000000
#define SPAWN_INTERVAL_MAX_SECONDS 0
#define DISK_TIME 140000000
#define SEMAPHORE_NAME "GERALD_THE_SEMAPHORE"
#define UPDATE_CLOCK_TO_TIME_PERIOD 20000


/* Global variables needed so signal handler and other functions can have access to them */
PCB process_table[MAX_PROCESSES];            // Global process table
unsigned long long int* sh_clock_ptr = NULL; // Global pointer for shared memory
int sh_clock_id = -1;                        // Global integer for shared memory ID
int msg_id = -1;                             // Global integer for message queue ID
FILE* lfp = NULL;                            // Global file pointer for outputs
sem_t *sh_m_sem = NULL;                      // Global named semaphore
int num_of_lines = 0;                        // Global number of lines printed
node* blocked_head = NULL;                   // Global blocked queue head pointer

// Memory for statistics
unsigned long long int num_of_page_faults = 0;
unsigned long long int num_of_page_hits = 0;
unsigned long long int num_of_memory_accesses = 0;
unsigned long long int num_of_memory_requests = 0;
unsigned long long int time_spent_waiting_for_memory[2] = {0, 0};

/*
  print_statistics
  Input: FILE* lfp, unsigned long long int num_of_page_faults, unsigned long long int num_of_page_hits, unsigned long long int num_of_memory_accesses, unsigned long long int num_of_memory_requests, unsigned long long int time_spent_waiting_for_memory[2]
  lfp                              : log file pointer
  num_of_page_faults               : how many times we had a page fault
  num_of_page_hits                 : how many times we had a page hit
  num_of_memory_accesses           : how many times we were able to actually access memory
  num_of_memory_requests           : how many requests we actually made to memory
  time_spent_waiting_for_memory[2] : an array of second/nanosecond of the total time spent waiting for memory requests
  
  Output: void
*/
void print_statistics(FILE* lfp, unsigned long long int num_of_page_faults, unsigned long long int num_of_page_hits, unsigned long long int num_of_memory_accesses, unsigned long long int num_of_memory_requests, unsigned long long int time_spent_waiting_for_memory[2]) {
  printf("IMPORTANT STATISTICS:\n");
  
  // Page fault information
  printf("Number of page faults: %llu\n", num_of_page_faults);
  if (lfp != NULL) {
    fprintf(lfp, "Number of page faults: %llu\n", num_of_page_faults);
  }
  
  // Page hit information
  printf("Number of page hits: %llu\n", num_of_page_hits);
  if (lfp != NULL) {
    fprintf(lfp, "Number of page hits: %llu\n", num_of_page_hits);
  }
  
  // Memory access information
  printf("Number of memory accesses (times we completed memory requests): %llu\n", num_of_memory_accesses);
  if (lfp != NULL) {
    fprintf(lfp, "Number of memory accesses (times we completed memory requests): %llu\n", num_of_memory_accesses);
  }
  
  // Number of memory requests
    printf("Number of memory requests (may have not finished): %llu\n", num_of_memory_requests);
  if (lfp != NULL) {
    fprintf(lfp, "Number of memory requests (may have not finished): %llu\n", num_of_memory_requests);
  }
  
  
  // Memory access per second
  double accesses_per_second = 0;
  if (time_spent_waiting_for_memory[0] + time_spent_waiting_for_memory[1] != 0) {
    accesses_per_second = (double)(num_of_memory_accesses) / (sh_clock_ptr[0] + (double)sh_clock_ptr[1] / BILLION);
  }
  printf("Memory accesses per second: %f\n", accesses_per_second);
  if (lfp != NULL) {
    fprintf(lfp, "Memory accesses per second: %f\n", accesses_per_second);
  }
  
  // Page fault percentage per memory request
  double page_fault_percentage = (num_of_memory_requests == 0 ? 0 : (double) (num_of_page_faults) / num_of_memory_requests);
  printf("Number of page faults per memory request: %f\n", page_fault_percentage);
  if (lfp != NULL) {
    fprintf(lfp, "Number of page faults per memory request: %f\n", page_fault_percentage);
  }
  
  // Average the time waited for actual memory access (this is done since there are requests we may not have serviced)
  double seconds = (time_spent_waiting_for_memory[0] + (double)time_spent_waiting_for_memory[1] / BILLION);
  double average_time = num_of_memory_accesses == 0? 0 : seconds / num_of_memory_accesses;
  printf("Average access time: %f seconds\n", average_time);
  if (lfp != NULL) {
    fprintf(lfp, "Average access time: %f seconds\n", average_time);
  }
  
  return;
}


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
  printf(output);
  if (lfp != NULL) {
    fprintf(lfp, output);
  }
  
  print_statistics(lfp, num_of_page_faults, num_of_page_hits, num_of_memory_accesses, num_of_memory_requests, time_spent_waiting_for_memory);
  
  // Clean up resources
  // OF NOTE - this may cause an error message where a child
  // does not receive a message. This is okay, since we are ending
  // the processes, and they never will receive a message. This occurs
  // since a process may be forked before having its pid in the process tableS
  kill_all_processes(process_table);
  if (msg_id != -1) {
    clean_msg_queue(msg_id);
  }
  
  // Clean the shared memory since we do not need it anymore
  if (sh_clock_ptr != NULL) {
    clean_shared_clock(sh_clock_ptr, sh_clock_id);
  }
  
  sem_close(sh_m_sem);
  sem_unlink(SEMAPHORE_NAME);
  
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
  value.it_interval.tv_sec = 2;
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
  // Initialize the memory tables
  init_memory_tables(-1);

  // Initial time when the program started
  time_t init_time = time(0);

  // Setting up the signal handlers
  if (setup_interrupt() == -1) {
    sem_close(sh_m_sem);
    sem_unlink(SEMAPHORE_NAME);
    error("UNABLE TO SET UP INTERRUPT HANDLER", 0, lfp);
  }
  if (setup_it_timer() == -1) {
    sem_close(sh_m_sem);
    sem_unlink(SEMAPHORE_NAME);
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
        sem_close(sh_m_sem);
        sem_unlink(SEMAPHORE_NAME);
        error("UKNOWN OPTION OR ARGUMENT MISSING", 1, lfp);
        break;
    }
  }
  
  // Open the logfile
  lfp = NULL;
  if (isLogFile) {
    lfp = fopen(filename, "w");
    if (lfp == NULL) {
      sem_close(sh_m_sem);
      sem_unlink(SEMAPHORE_NAME);
      error("UNABLE TO OPEN THE LOGFILE", 0, lfp);
    }
  }
  
    // Create the semaphore
  // Extra semaphore info found here:
  // https://stackoverflow.com/questions/32205396/share-posix-semaphore-among-multiple-processes
  sh_m_sem = sem_open(SEMAPHORE_NAME, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, 1);
  if (sh_m_sem == SEM_FAILED) {
    error("FAILED TO CREATE THE SHARED SEMAPHORE", 0, lfp);
  }
  
  // Allocate a message queue
  msg_id = msgget(m_queue_key, IPC_CREAT | 0777);
  if (msg_id < 0) {
    sem_close(sh_m_sem);
    sem_unlink(SEMAPHORE_NAME);
    char* err_msg = "FAILED TO ALLOCATE A MESSAGE QUEUE";
    error(err_msg, 0, lfp);
  }
  
  // Allocate the shared memory
  sh_clock_id = shmget(sh_clock_key, sizeof(unsigned long long int) * 2, IPC_CREAT | 0777);
  if (sh_clock_id <= 0) {
    sem_close(sh_m_sem);
    char* err_msg = "FAILED TO ALLOCATE SHARED MEMORY";
    sem_unlink(SEMAPHORE_NAME);
    clean_msg_queue(msg_id);
    error(err_msg, 0, lfp);
  }
  
  // Get the shared memory pointer
  sh_clock_ptr = shmat(sh_clock_id, 0, 0);
  if (sh_clock_ptr <= 0) {
    sem_close(sh_m_sem);
    sem_unlink(SEMAPHORE_NAME);
    char* err_msg = "UNABLE TO ATTACH TO SHARED MEMORY";
    clean_msg_queue(msg_id);
    error(err_msg, 0, lfp);
  }
  
  // Initialize the clock values
  sh_clock_ptr[0] = 0ull;
  sh_clock_ptr[1] = 0ull;
  int prev_second = sh_clock_ptr[0];
  unsigned long long int prev_nano_second = sh_clock_ptr[1];
  
  // Set information for processes
  int total_processes_finished = 0;
  int total_processes = 0;
  int executing_processes = 0;
  
  // Set the initial spawn interval and times
  unsigned long long int rand_spawn_sec = SPAWN_INTERVAL_MAX_SECONDS == 0 ? 0 : rand() % SPAWN_INTERVAL_MAX_SECONDS;
  unsigned long long int rand_spawn_nano = SPAWN_INTERVAL_MAX_NANO_SECONDS == 0 ? 0 : rand() % SPAWN_INTERVAL_MAX_NANO_SECONDS;
  unsigned long long int spawn_time_sec = rand_spawn_sec;
  unsigned long long int spawn_time_nano = rand_spawn_nano;
  
  /* MAIN LOOP */
  while (total_processes_finished < TOTAL_PROCESSES_TO_SPAWN) {
  
    /* Every 10000 nanoseconds we increment the clock to the head of the requests if we have any */
    if (sh_clock_ptr[1] - prev_nano_second >= UPDATE_CLOCK_TO_TIME_PERIOD) {
      if (blocked_head != NULL) {
        while (((blocked_head->blocked_end_seconds >= sh_clock_ptr[0] && blocked_head->blocked_end_nano_seconds >= sh_clock_ptr[1]) || blocked_head->blocked_end_seconds > sh_clock_ptr[0])) {
          int effective_deadlock_increment = 10000;
          
          add_to_memory_access_time(process_table, effective_deadlock_increment, -1, 1);
          increment_time(&blocked_head, effective_deadlock_increment);
          sh_clock_ptr[1] += effective_deadlock_increment;
          update_if_nano_seconds_went_over_billion();
        }
      }
      prev_nano_second = sh_clock_ptr[1];
    }
  
    // Loop and wake up any processes that have been blocked
    node blocked = queue_remove_blocked(&blocked_head, sh_clock_ptr[0], sh_clock_ptr[1]);
    while(blocked.pid >= 0) {
      if (VERBOSE) {
        printf("OSS: Process %d's requested page %llu is now in memory, being put in frame %llu and unblocking. System time: %llu, %llu\n", blocked.pid, blocked.page_index, blocked.request_index, sh_clock_ptr[0], sh_clock_ptr[1]);
        if (lfp != NULL) {
          fprintf(lfp, "OSS: Process %d's requested page %llu is now in memory, being put in frame %llu and unblocking. System time: %llu, %llu\n", blocked.pid, blocked.page_index, blocked.request_index, sh_clock_ptr[0], sh_clock_ptr[1]);
        }
        num_of_lines++;
      }

      PCB t = get_process_copy(process_table, blocked.pid);
      
      if (PRINT_WAITING_INFORMATION) {
        printf("\nTotal memory reqests for %d:%d\n", blocked.pid, t.total_memory_requests);
        printf("\nTime spent waiting in total: %llu, %llu\n", t.time_spent_memory_access[0], t.time_spent_memory_access[1]);
        printf("\nTime spent waiting for this request: %llu, %llu\n", blocked.s, blocked.ns);
        if (lfp != NULL) {
          fprintf(lfp, "\nTotal memory reqests for %d:%d\n", blocked.pid, t.total_memory_requests);
          fprintf(lfp, "\nTime spent waiting in total: %llu, %llu\n", t.time_spent_memory_access[0], t.time_spent_memory_access[1]);
          fprintf(lfp, "\nTime spent waiting for this request: %llu, %llu\n", blocked.s, blocked.ns);
        }
      }
      
      // Increment the number of memory accesses and the total time spent waiting
      num_of_memory_accesses++;
      time_spent_waiting_for_memory[0] += blocked.s;
      time_spent_waiting_for_memory[1] += blocked.ns;
      update_time_if_billion(&time_spent_waiting_for_memory[0], &time_spent_waiting_for_memory[1]);
      
      // Generate a resource request message
      // so that the process can continue
      message grant_request_message;
      grant_request_message.mtype = blocked.pid;
      
      // Send the request granting message
      if (msgsnd(msg_id, &grant_request_message, sizeof(message) - sizeof(long), 0) < 0) {
        dealloc_queue(&blocked_head);
        kill_all_processes(process_table);
        clean_shared_clock(sh_clock_ptr, sh_clock_id);
        clean_msg_queue(msg_id);
        sem_close(sh_m_sem);
        sem_unlink(SEMAPHORE_NAME);
        
        char* err_msg = "UNABLE TO SEND MESSAGE";
        error(err_msg, 0, lfp);
      }
      
      // Get a copy of the dirty bit
      int dirty = frame_table[blocked.request_index].dirty_bit;
      
      set_state(process_table, blocked.pid, READY);
      page_tables[pid_to_index(blocked.pid)][blocked.page_index].num = blocked.request_index;
      swap_frame(blocked.pid, blocked.page_index, blocked.request_index);
      
      int time_taken_to_read_from_swapped_in_memory = 0;
      
      // Read and no dirty bit is fastest
      if (blocked.read == 1 && dirty != 1) {
        time_taken_to_read_from_swapped_in_memory = 100;
      // Read and dirty bit is a little slower
      } else if (blocked.read == 1) {
        time_taken_to_read_from_swapped_in_memory = 1000;
      // Write and not dirty bit is slow
      } else if (blocked.read == 0 && dirty != 1) {
        frame_table[blocked.request_index].dirty_bit = 1;
        time_taken_to_read_from_swapped_in_memory = 1000;
      // Write with dirty bit is slowest
      } else {
        frame_table[blocked.request_index].dirty_bit = 1;
        time_taken_to_read_from_swapped_in_memory = 2000;
      }
      
      // Update clock in critical section
      sem_wait(sh_m_sem);
      sh_clock_ptr[1] += time_taken_to_read_from_swapped_in_memory;
      add_to_memory_access_time(process_table, time_taken_to_read_from_swapped_in_memory, -1, 1);
      increment_time(&blocked_head, time_taken_to_read_from_swapped_in_memory);
      update_if_nano_seconds_went_over_billion();
      sem_post(sh_m_sem);
      
      // Check if there are any more blocked processes
      blocked = queue_remove_blocked(&blocked_head, sh_clock_ptr[0], sh_clock_ptr[1]);
    }
  
    if (sh_clock_ptr[0] - prev_second >= 1) {
      num_of_lines += display_frames(lfp);
      prev_second = sh_clock_ptr[0];
    }
  
    // Ending condition of 100,000 lines of output
    if (num_of_lines >= 100000) {
      printf("\nMore than 100,000 lines in output, ending system simulation\n");
      if (lfp != NULL) {
        fprintf(lfp, "\nMore than 100,000 lines in output, ending system simulation\n");
      }
      
      kill(getpid(), SIGPROF);
    }
    
    // Ending condition of five seconds (another precaution in case the process gets blocked and takes longer than five seconds)
    if (time(0) - init_time >= 2) {
      printf("\nTwo seconds have passed, ending system simulation\n");
      if (lfp != NULL) {
        fprintf(lfp, "\nTwo seconds have passed, ending system simulation\n");
      }
      kill(getpid(), SIGPROF);
    } 
    
    /* PROCESS SPAWNING IN BETWEEN PROCESSES */
    // Spawn a process if we have passed our spawn time (all while abiding by process constraints)
    sem_wait(sh_m_sem);
    unsigned long long int temp_clock_sec = sh_clock_ptr[0];
    unsigned long long int temp_clock_nano_sec = sh_clock_ptr[1];
    sem_post(sh_m_sem);
    if (((temp_clock_sec >= spawn_time_sec && temp_clock_nano_sec >= spawn_time_nano) || temp_clock_sec > spawn_time_sec) && executing_processes < MAX_PROCESSES && total_processes < TOTAL_PROCESSES_TO_SPAWN) {
      
      // Set a new spawn interval
      rand_spawn_sec = SPAWN_INTERVAL_MAX_SECONDS == 0 ? 0 : rand() % SPAWN_INTERVAL_MAX_SECONDS;
      rand_spawn_nano = SPAWN_INTERVAL_MAX_NANO_SECONDS == 0 ? 0 : rand() % SPAWN_INTERVAL_MAX_NANO_SECONDS;

      spawn_time_sec = rand_spawn_sec + sh_clock_ptr[0];
      spawn_time_nano = rand_spawn_nano + sh_clock_ptr[1];
      
      update_time_if_billion(&spawn_time_sec, &spawn_time_nano);
      
      int cpid = fork();
      
      // If we are a child, exec
      if (cpid == 0) {
        char* command = "./user_proc";
        char* arguments[] = {command, NULL};
        execvp(command, arguments);
        
        error("FAILED TO EXEC IN THE CHILD", 0, NULL);
        
      // If we are a parent, update the PCB
      } else if (cpid > 0) {
        // Output to stdout and file
        if (VERBOSE) {
          sem_wait(sh_m_sem);
          printf("OSS: Spawned a new process, PID: %d, System time: %llu %llu\n", cpid, sh_clock_ptr[0], sh_clock_ptr[1]);
          if (lfp != NULL) {
            fprintf(lfp, "OSS: Spawned a new process, PID: %d, System time: %llu %llu\n", cpid, sh_clock_ptr[0], sh_clock_ptr[1]);
          }
          sem_post(sh_m_sem);
          num_of_lines++;
          
          sem_wait(sh_m_sem);
          printf("OSS: Will spawn new process when it has control at some time after: %llu, %llu if there is space. If there is not space, it will be spawned as as soon as there is space. System time: %llu, %llu\n", spawn_time_sec, spawn_time_nano, sh_clock_ptr[0], sh_clock_ptr[1]);
          if (lfp != NULL) {
            fprintf(lfp, "OSS: Will spawn new process when it has control at some time after: %llu, %llu if there is space. If there is not space, it will be spawned as as soon as there is space. System time: %llu, %llu\n", spawn_time_sec, spawn_time_nano, sh_clock_ptr[0], sh_clock_ptr[1]);
          }  
          sem_post(sh_m_sem);   
          num_of_lines++;
        }

        int insert_sucess_pcb = insert_process(process_table, cpid, sh_clock_ptr[0], sh_clock_ptr[1]);
        int insert_sucess_mem_table = insert_process_into_page_table(cpid);
        
        // insert the pid translation
        for (int i = 0; i < MAX_PROCESSES; i++) {
          if (index_to_pid[i] == -1) {
            index_to_pid[i] = cpid;
            break;
          }
        }
        
        if (!insert_sucess_pcb || !insert_sucess_mem_table) {
          dealloc_queue(&blocked_head);
          kill_all_processes(process_table);
          kill(cpid, SIGKILL);
          clean_msg_queue(msg_id);
          clean_shared_clock(sh_clock_ptr, sh_clock_id);
          sem_close(sh_m_sem);
          sem_unlink(SEMAPHORE_NAME);
          
          char* err_msg = "UNABLE TO INSERT ANOTHER PROCESS INTO THE PROCESS TABLE OR MEMORY TABLE";
          error(err_msg, 0, lfp);
        }
      
      // Error case, fork failed
      } else {
        dealloc_queue(&blocked_head);
        kill_all_processes(process_table);
    		clean_shared_clock(sh_clock_ptr, sh_clock_id);
        clean_msg_queue(msg_id);
        sem_close(sh_m_sem);
        sem_unlink(SEMAPHORE_NAME);
        
        char* err_msg = "FAILED TO FORK";
    		error(err_msg, 0, lfp);
      }
      
      // Update process information
      total_processes++;
      executing_processes++;
    }
    
    // Debugging outputs (NOT WRITTEN TO LOG FILE)
    if (DISPLAY_PROCESS_TABLE) {
      display_process_table(process_table);
    }
    if (DISPLAY_QUEUES) {
      display_queue(&blocked_head);
    }
    
    
    /* SEE IF WE HAVE RECEIVED A MESSAGE FROM ANY CHILD PROCESSES */    
    message received_message;
    int is_there_message = msgrcv(msg_id, &received_message, sizeof(message), getpid(), IPC_NOWAIT);
    
    // If there is a message, then process it
    if (is_there_message != -1) {
      // Child has indicated that it is ending
      if (received_message.ending == 1) {
      
        // Calculate effective access time
        PCB t = get_process_copy(process_table, received_message.pid);
        double seconds = t.time_spent_memory_access[0] + t.time_spent_memory_access[1] / BILLION;
        double average = 0;
        if (seconds > 0.0) {
          average = seconds / t.total_memory_requests;
        }
      
        // Notify user that process is ending and display the effective access time
        if (VERBOSE) {
          printf("OSS: Process %d ending, effective access time: %f seconds\n", received_message.pid, average);
          if (lfp != NULL) {
            fprintf(lfp, "OSS: Process %d ending, efective access time: %f seconds\n", received_message.pid, average);
          }
          num_of_lines++;
        }
        
        // Remove the process system data
        remove_process(process_table, received_message.pid, -1);
        remove_page_and_frame_table_data(received_message.pid, -1);
      
        wait(NULL);
      
        total_processes_finished++;
        executing_processes--;
        
      // Child has indicated that it does not want to end and wants to make a request
      } else {
        // Increment the number of requests that we have made
        num_of_memory_requests++;
        
        if (VERBOSE) {
          if (received_message.reading == 1) {
            sem_wait(sh_m_sem);
            printf("OSS: Process %d requesting read on location: %d, System clock: %llu %llu\n", received_message.pid, received_message.location, sh_clock_ptr[0], sh_clock_ptr[1]);
            if (lfp != NULL) {
              fprintf(lfp, "OSS: Process %d requesting read on location: %d, System clock: %llu %llu\n", received_message.pid, received_message.location, sh_clock_ptr[0], sh_clock_ptr[1]);
            }
            sem_post(sh_m_sem);
            num_of_lines++;
          } else {
            sem_wait(sh_m_sem);
            printf("OSS: Process %d requesting write on location: %d, System clock: %llu %llu\n", received_message.pid, received_message.location, sh_clock_ptr[0], sh_clock_ptr[1]);
            if (lfp != NULL) {
              fprintf(lfp, "OSS: Process %d requesting write on location: %d, System clock: %llu %llu\n", received_message.pid, received_message.location, sh_clock_ptr[0], sh_clock_ptr[1]);
            }
            sem_post(sh_m_sem);
            num_of_lines++;
          }
        }
        
        /* WE CAN GRANT A REQUEST IF WE ARE PAST THIS */
        
        /* Check if we have an open frame is in the memory tables */
        
        // Get the memory location
        int page_index = received_message.location / FRAME_SIZE;
        int offset = received_message.location % FRAME_SIZE;
        
        // If a page has a number of -1, it means it is not in main memory so it must be put into main memory
        if (page_tables[pid_to_index(received_message.pid)][page_index].num == -1) {
          // Find an open frame
          int open_frame = find_open_frame();
          
          // Increment the number of memory page faults
          num_of_page_faults++;
          
          // If there is an open frame available
          if (open_frame >= 0) {
          
            // 'claim' the open frame (so no other process tries to claim it as an open frame as well, since it is still technically open)
            frame_table[open_frame].claimed = 1;
            
            if (VERBOSE) {
              printf("OSS: page fault for process %d, open frame found at index %d, making memory request and blocking\n", received_message.pid, open_frame);
              if (lfp != NULL) {
                fprintf(lfp, "OSS: page fault for process %d, open frame found at index %d, making memory request and blocking\n", received_message.pid, open_frame);
              }
              num_of_lines++;
            }
            
            unsigned long long int end_time_sec = sh_clock_ptr[0];
            unsigned long long int end_time_nano_sec = sh_clock_ptr[1] + DISK_TIME;
            update_time_if_billion(&end_time_sec, &end_time_nano_sec);
            
            // Place the process in the blocked queue and set its state in the process table
            queue_insert(&blocked_head, received_message.pid, end_time_sec, end_time_nano_sec, open_frame, page_index, received_message.reading);
            set_state(process_table, received_message.pid, BLOCKED);
            increment_total_memory_requests(process_table, received_message.pid);
            
            // Update clock in critical section and update data structures
            sem_wait(sh_m_sem);
            int time_to_intially_handle_request = 1000;
            sh_clock_ptr[1] += time_to_intially_handle_request;
            add_to_memory_access_time(process_table, time_to_intially_handle_request, -1, 1);
            increment_time(&blocked_head, time_to_intially_handle_request);
            update_if_nano_seconds_went_over_billion();
            sem_post(sh_m_sem);
            
            // Skip this iteration
            continue;
            
          // If there isn't an open frame, then use the head  
          } else {
            if (VERBOSE) {
              printf("OSS: page fault for process %d, setting page to replace and updating head to %d\n", received_message.pid, head);
              if (lfp != NULL) {
                fprintf(lfp, "OSS: page fault for process %d, setting page to replace and updating head to %d\n", received_message.pid, head);
              }
              num_of_lines++;
            }
    
            
            // Otherwise make a request to swap out a page and block the process
            int frame_to_replace = get_frame_from_head();
            
            unsigned long long int end_time_sec = sh_clock_ptr[0];
            unsigned long long int end_time_nano_sec = sh_clock_ptr[1] + DISK_TIME;
            update_time_if_billion(&end_time_sec, &end_time_nano_sec);
            
            // Place the process in the blocked queue and set its state in the process table
            queue_insert(&blocked_head, received_message.pid, end_time_sec, end_time_nano_sec, frame_to_replace, page_index, received_message.reading);
            set_state(process_table, received_message.pid, BLOCKED);
            increment_total_memory_requests(process_table, received_message.pid);
            
            // Update clock in critical section
            sem_wait(sh_m_sem);
            int time_to_intially_handle_request = 1000;
            sh_clock_ptr[1] += time_to_intially_handle_request;
            add_to_memory_access_time(process_table, time_to_intially_handle_request, -1, 1);
            increment_time(&blocked_head, time_to_intially_handle_request);
            update_if_nano_seconds_went_over_billion();
            sem_post(sh_m_sem);
            
            // Skip this iteration
            continue;
          }
          
        // Our requested page is in memory
        } else {
          increment_total_memory_requests(process_table, received_message.pid);
          if (VERBOSE) {
            printf("OSS: Process %d's request for page %d is in memory at frame %d\n",received_message.pid, page_index, page_tables[pid_to_index(received_message.pid)][page_index].num);
            if (lfp != NULL) {
              fprintf(lfp, "OSS: Process %d's request for page %d is in memory at frame %d\n",received_message.pid, page_index, page_tables[pid_to_index(received_message.pid)][page_index].num);
            }
            num_of_lines++;
          }
          
          // How long we take to handle this request will depend on if it is a read/write
          int time_to_make_request_from_memory = 0;
          if (received_message.reading == 1) {
            time_to_make_request_from_memory = 100;
          } else {
            time_to_make_request_from_memory = 1000;
            page_tables[pid_to_index(received_message.pid)][page_index].dirty_bit = 1;
          }
          
          
          // Increment statistics values
          num_of_page_hits++;
          num_of_memory_accesses++;
          time_spent_waiting_for_memory[1] += time_to_make_request_from_memory;
          update_time_if_billion(&time_spent_waiting_for_memory[0], &time_spent_waiting_for_memory[1]);
          
          // Increment the clock
          sem_wait(sh_m_sem);
          sh_clock_ptr[1] += time_to_make_request_from_memory;
          add_to_memory_access_time(process_table, time_to_make_request_from_memory, -1, 1);
          add_to_memory_access_time(process_table, time_to_make_request_from_memory, received_message.pid, 0);
          increment_time(&blocked_head, time_to_make_request_from_memory);
          update_if_nano_seconds_went_over_billion();
          sem_post(sh_m_sem);
        }
        
        // Generate a resource request message
        message grant_request_message;
        grant_request_message.mtype = received_message.pid;
        
        // Send the request granting message
        if (msgsnd(msg_id, &grant_request_message, sizeof(message) - sizeof(long), 0) < 0) {
          dealloc_queue(&blocked_head);
          kill_all_processes(process_table);
          clean_shared_clock(sh_clock_ptr, sh_clock_id);
          clean_msg_queue(msg_id);
          sem_close(sh_m_sem);
          sem_unlink(SEMAPHORE_NAME);
          
          char* err_msg = "UNABLE TO SEND MESSAGE";
          error(err_msg, 0, lfp);
        }
      }
    } else {
        // We have nothing to service, increment clock
        sem_wait(sh_m_sem);
        int time_idling = 10000;
        sh_clock_ptr[1] += time_idling;
        add_to_memory_access_time(process_table, time_idling, -1, 1);
        increment_time(&blocked_head, time_idling);
        update_if_nano_seconds_went_over_billion();
        sem_post(sh_m_sem);
      }
  }
  
  // Print out the ending message
  printf("\n%d PROCESSES HAVE COMPLETED IN THE SYSTEM\n\n", TOTAL_PROCESSES_TO_SPAWN);
  if (lfp != NULL) {
    fprintf(lfp, "\n%d PROCESSES HAVE COMPLETED IN THE SYSTEM\n\n", TOTAL_PROCESSES_TO_SPAWN);
  }
  
  print_statistics(lfp, num_of_page_faults, num_of_page_hits, num_of_memory_accesses, num_of_memory_requests, time_spent_waiting_for_memory);
  
  // Clean up resources
  kill_all_processes(process_table);
  dealloc_queue(&blocked_head);
 	clean_shared_clock(sh_clock_ptr, sh_clock_id);
  clean_msg_queue(msg_id);
  sem_unlink(SEMAPHORE_NAME);
  sem_close(sh_m_sem);
  if (lfp != NULL) {
    fclose(lfp);
  }
  
 	return EXIT_SUCCESS;
}