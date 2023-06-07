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
#include "resource.h"

// Debugging constants (output for this debugging information will not be written to a log file since its functions are delegated to other files)
#define DISPLAY_PROCESS_TABLE 0
#define DISPLAY_QUEUES 0
#define VERBOSE 1

// Constants for the system to use
#define CLOCK_INCREMENT 100000
#define TOTAL_PROCESSES_TO_SPAWN 40
#define SPAWN_INTERVAL_MAX_NANO_SECONDS 50000
#define SPAWN_INTERVAL_MAX_SECONDS 0
#define SEMAPHORE_NAME "GERALD_THE_SEMAPHORE"


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
unsigned long long int times_deadlocked = 0;
unsigned long long int num_times_resource_granted_immediately = 0;
unsigned long long int num_times_resource_not_granted_immediately = 0;
unsigned long long int num_of_processes_terminated_by_deadlock_detection = 0;
unsigned long long int num_of_processes_terminated_normally = 0;
unsigned long long int num_times_deadlock_detection_ran = 0;
unsigned long long int num_times_deadlock_detection_started = 0;
double total_percent_of_processes_terminated_by_deadlock_recovery = 0;

// Memory used to determine when to display resources
unsigned long long int num_times_resource_granted = 0;
unsigned long long int num_times_resource_granted_prev = 0;


/*
  print_statistics
  Takes in system information and prints out statistics
  
  Input: FILE* lfp, unsigned long long int times_deadlocked, unsigned long long int num_times_resource_granted_immediately, unsigned long long int num_times_resource_not_granted_immediately, unsigned long long int num_of_processes_terminated_by_deadlock_detection, unsigned long long int num_of_processes_terminated_normally, unsigned long long int num_times_deadlock_detection_ran, double total_percent_of_processes_terminated_by_deadlock_recovery
  
  lfp                                                        : Log file pointer
  times_deadlocked                                           : Number of times the system was actually in a deadlock
  num_times_resource_granted_immediately                     : Number of times the system was able to immediately give a process a requested resource
  num_times_resource_not_granted_immediately                 : Number of times the system was not able to immediately give a process a requested resource
  num_of_processes_terminated_by_deadlock_detection          : Number of processes that we terminated to remove a deadlock
  num_of_processes_terminated_normally                       : Number of processes that terminated normally
  num_times_deadlock_detection_ran                           : Number of times we ran deadlock detection (includes the times we run deadlock detection after removing processes)
  total_percent_of_processes_terminated_by_deadlock_recovery : The total sum of the percentages of processes that we had to terminate for each deadlock
  
  Output: void
*/
void print_statistics(FILE* lfp, unsigned long long int times_deadlocked, unsigned long long int num_times_resource_granted_immediately, unsigned long long int num_times_resource_not_granted_immediately, unsigned long long int num_of_processes_terminated_by_deadlock_detection, unsigned long long int num_of_processes_terminated_normally, unsigned long long int num_times_deadlock_detection_ran, double total_percent_of_processes_terminated_by_deadlock_recovery) {
  // Header to notify users what we are doing
  printf("System statistics:\n");
  if (lfp != NULL) {
    fprintf(lfp, "System statistics:\n");
  }
  
  // How many deadlocks happened
  printf("Number of times deadlocked: %llu\n", times_deadlocked);
  if (lfp != NULL) {
    fprintf(lfp, "Number of times deadlocked: %llu\n", times_deadlocked);
  }
  
  // How many times we were able to grant requests
  printf("Number of times a resource was granted immediately: %llu\n", num_times_resource_granted_immediately);
  if (lfp != NULL) {
    fprintf(lfp, "Number of times a resource was granted immediately: %llu\n", num_times_resource_granted_immediately);
  }
  
  // How many times we had to block processes
  printf("Number of times a resource was not granted immediately: %llu\n", num_times_resource_not_granted_immediately);
  if (lfp != NULL) {
    fprintf(lfp, "Number of times a resource was not granted immediately: %llu\n", num_times_resource_not_granted_immediately);
  }
  
  // How many processes we had to terminate in deadlock detection
  printf("Number of times a process was terminated by deadlock detection: %llu\n", num_of_processes_terminated_by_deadlock_detection);
  if (lfp != NULL) {
    fprintf(lfp, "Number of times a process was terminated by deadlock detection: %llu\n", num_of_processes_terminated_by_deadlock_detection);
  }
  
  // How many processes terminated normally
  printf("Number of times a process terminated normally: %llu\n", num_of_processes_terminated_normally);
  if (lfp != NULL) {
    fprintf(lfp, "Number of times a process terminated normally: %llu\n", num_of_processes_terminated_normally);
  }
  
  // How many times we ran the deadlock detection algorithm (includes the times we run it after terminating a process in deadlock recovery)
  printf("Number of times deadlock detection ran: %llu\n", num_times_deadlock_detection_ran);
  if (lfp != NULL) {
    fprintf(lfp, "Number of times deadlock detection ran: %llu\n", num_times_deadlock_detection_ran);
  }
  
  // Average percent of processes we had to terminate (divides the sum of the percentages for each deadlock over the times deadlocked)
  double average = times_deadlocked == 0 ? 0 : total_percent_of_processes_terminated_by_deadlock_recovery / times_deadlocked;
  printf("Average percentage of processes terminated: %f\n", average);
  if (lfp != NULL) {
    fprintf(lfp, "Average percentage of processes terminated: %f\n", average);
  }
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
  printf("%s", output);
  if (lfp != NULL) {
    fprintf(lfp, "%s", output);
  }
  
    print_statistics(lfp, times_deadlocked, num_times_resource_granted_immediately, num_times_resource_not_granted_immediately, num_of_processes_terminated_by_deadlock_detection, num_of_processes_terminated_normally, num_times_deadlock_detection_ran, total_percent_of_processes_terminated_by_deadlock_recovery);
  
  // Clean up resources
  // OF NOTE - this may cause an error message where a child
  // does not receive a message. This is okay, since we are ending
  // the processes, and they never will receive a message. This occurs
  // since a process may be forked before having its pid in the process table
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
  value.it_interval.tv_sec = 5;
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

/*
  deadlock_detection_list
  Get a list of the deadlocked processes
  (note that it checks all processes, not just blocked ones, since by default nonblocked processes will not have any requests and will be marked)
  
  Input: int allocated[MAX_PROCESSES][NUMBER_OF_UNIQUE_RESOURCES], int request[MAX_PROCESSES][NUMBER_OF_UNIQUE_RESOURCES], int available[NUMBER_OF_UNIQUE_RESOURCES], int index_to_pid[MAX_PROCESSES], int* size
  allocated    : The allocation matrix
  request      : The request matrix
  available    : The available resources
  index_to_pid : The index to pid translation array
  size         : Storage for the size of the returned list
  
  Output: int*
  Array of pids that correspond to deadlocked processes
*/
int* deadlock_detection_list(int allocated[MAX_PROCESSES][NUMBER_OF_UNIQUE_RESOURCES], int request[MAX_PROCESSES][NUMBER_OF_UNIQUE_RESOURCES], int available[NUMBER_OF_UNIQUE_RESOURCES], int index_to_pid[MAX_PROCESSES], int* size) {
  int allocated_cp[MAX_PROCESSES][NUMBER_OF_UNIQUE_RESOURCES];
  int request_cp[MAX_PROCESSES][NUMBER_OF_UNIQUE_RESOURCES];
  int available_cp[NUMBER_OF_UNIQUE_RESOURCES];
  
  // List of marked processes, we start with them all unmarked (i.e. they are assumed to be deadlocked)
  int marked[MAX_PROCESSES];
  for (int i = 0; i < MAX_PROCESSES; i++) {
    marked[i] = 1;
  }
  
  // Mark all the empty indices where there is no pid (i.e. there is no process, cannot participate in deadlocked)
  for (int i = 0; i < MAX_PROCESSES; i++) {
    if (index_to_pid[i] < 0) {
      marked[i] = 0;
    }
  }
  
  // Copy the data into the local variables so we can modify their values
  for (int i = 0; i < MAX_PROCESSES; i++){
    for (int j = 0; j < NUMBER_OF_UNIQUE_RESOURCES; j++) {
      allocated_cp[i][j] = allocated[i][j];
      request_cp[i][j] = request[i][j];
    }
  }
  for (int i = 0; i < NUMBER_OF_UNIQUE_RESOURCES; i++) {
    available_cp[i] = available[i];
  }
  
  // Go through each process and see if its requests can be fulfilled
  // If they can, then simulate the process ending
  int marked_something = 0;
  for (int i = 0; i < MAX_PROCESSES; i++) {
    for (int j = 0; j < MAX_PROCESSES; j++) {
      // Start by assuming this process's request can be fulfilled
      // by setting is_less_than to 1
      int is_less_than = 1;
    
      for (int k = 0; k < NUMBER_OF_UNIQUE_RESOURCES; k++) {
        // Skip marked processes or nonexistent processes
        if (marked[j] == 0 || index_to_pid[j] < 0) {
          continue;
        }
      
        // If we can't grant a request for a process, then change
        // is_less_than to 0 to indicate this
        if (request_cp[j][k] > available_cp[k]) {
          is_less_than = 0;
        }
      }
      
      
      // If we can simulate this process ending, then free its resources and mark it
      if (is_less_than && marked[j] != 0) {
        /*
        printf("\n\nWe can end process at index: %d\n\n", j);
        if (lfp != NULL) {
          fprintf(lfp,"\n\nWe can end process at index: %d\n\n", j);
        }
        
        printf("\n\nMarked status: %d\n\n", marked[j]);
        if (lfp != NULL) {
          fprintf(lfp, "\n\nMarked status: %d\n\n", marked[j]);
        }
        */
        
        marked_something++;
        marked[j] = 0;
        for (int i = 0; i < NUMBER_OF_UNIQUE_RESOURCES; i++) {
          available_cp[i] += allocated_cp[j][i];
        }
      }
    }
  }
  
  // Count all the unmarked processes at the end
  int unmarked_process_count = 0;
  for (int i = 0; i < MAX_PROCESSES; i++) {
    if (marked[i] != 0) {
      unmarked_process_count++;
    }
  }
    
  /*
  printf("\n\nUNMARKED PROCESSES:%d\n\n", unmarked_process_count);
  if (lfp != NULL) {
    fprintf(lfp, "\n\nUNMARKED PROCESSES:%d\n\n", unmarked_process_count);
  }
  */
  
  // Allocate an array and find and store the deadlocked processes (i.e. those that were not marked)
  int* deadlocked_processes = malloc(sizeof(int) * unmarked_process_count);
  int deadlocked_count = 0;
  for (int i = 0; i < MAX_PROCESSES && deadlocked_count < unmarked_process_count; i++) {
    if (marked[i] != 0) {
      deadlocked_processes[deadlocked_count] = index_to_pid[i];
      deadlocked_count++;
    }
  }
  
  // Set the size
  *size = unmarked_process_count;
  
  return deadlocked_processes;
}

/*
  end_process_and_free_resources
  Ends a process in the system and frees its resources
  
  Input: PCB* process_table, node** blocked_queue, int pid, int allocated[MAX_PROCESSES][NUMBER_OF_UNIQUE_RESOURCES], int request[MAX_PROCESSES][NUMBER_OF_UNIQUE_RESOURCES], int available[NUMBER_OF_UNIQUE_RESOURCES]
  process_table : The process table holding process information
  blocked_queue : The blocked queue for processes waiting to have their requests fulfilled
  pid           : The process id for the process that we would like to end
  allocated     : Resource allocation matrix
  request       : Request matrix
  available     : The available resources
  
  Output: int
  Returns 1 for success, -1 for failure
*/
int end_process_and_free_resources(PCB* process_table, node** blocked_queue, int pid, int allocated[MAX_PROCESSES][NUMBER_OF_UNIQUE_RESOURCES], int request[MAX_PROCESSES][NUMBER_OF_UNIQUE_RESOURCES], int available[NUMBER_OF_UNIQUE_RESOURCES]) {
  
  // Remove the process from the blocked queue if it is in the blocked queue
  // Since we can't gaurantee it will be in the blocked queue, it generates no error
  queue_remove_by_pid(blocked_queue, pid);
  
  // Get the index of the process we are trying to end
  // and flag the index as currently empty
  int index = pid_to_index(pid);
  index_to_pid[index] = -1;
  if (index < 0) {
    return -1;
  }
  
  // Clear the request matrix
  for (int i = 0; i < NUMBER_OF_UNIQUE_RESOURCES; i++) {
    request[index][i] = 0;
  }
  
  // Clear the allocated matrix and release the resources
  for (int i = 0; i < NUMBER_OF_UNIQUE_RESOURCES; i++) {
    available[i] += allocated[index][i];
    allocated[index][i] = 0;
  }
  
  // Remove the process from the PCB Table
  int remove_success = remove_process(process_table, pid, 0);
  if (remove_success < 0) {
    return -1;
  }
  
  // End the child process
  kill(pid, SIGKILL);
  
  // Make sure we wait on the child to die
  wait(NULL);
  
  return 1;
}

/*
  queue_unblock_if_resource_is_available
  Goes through the blocked queue and removes and grants requests for a single process if possible
  The reason that this function does not take the resources as parameters is because it uses functions
  that require the resources to be global.
  
  Note - this uses the request_index as the resource index for the process
  this is done because I am reusing data structures from older projects
  and did not want to modify my code too much as to avoid any bugs
  
  Input: node** head
  head : Blocked queue head pointer
  
  Output: int
  Pid of the process to be unblocked and request granted
*/
int queue_unblock_if_resource_is_available(node** head) {
  struct node* tmp = *head;
  struct node* prev = NULL;
  int pid = -1;
  
  if (tmp == NULL) {
    return pid;
  } else {
  
    if (tmp->next == NULL && available[tmp->request_index] >= 1) {
      pid = (*head)->pid;  
      
      // Grant a resource request - no need to populate the request fulfillment
      // because that information isn't necessary for the child to acknowledge it
      // Allocate the resource and update the request
      message grant_request_message;
      grant_request_message.mtype = pid;
      
      update_request(pid, tmp->request_index, 0);
      increment_allocated(pid, tmp->request_index, 1);
      increment_available(tmp->request_index, -1);
      
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
      
      num_times_resource_granted++;
      
      if (VERBOSE) {
        sem_wait(sh_m_sem);
        printf("OSS: Process %d granting R%llu after blocking, System clock: %llu %llu\n", pid, tmp->request_index, sh_clock_ptr[0], sh_clock_ptr[1]);
        if (lfp != NULL) {
          fprintf(lfp,"OSS: Process %d granting R%llu after blocking, System clock: %llu %llu\n", pid, tmp->request_index, sh_clock_ptr[0], sh_clock_ptr[1]);
        }
        sem_post(sh_m_sem);
        num_of_lines++;
      }
      
      free(*head);
      *head = NULL;
      
      return pid;
    }
  
    while (tmp != NULL) {
      if (available[tmp->request_index] >= 1) {
        if (prev != NULL) {
          prev->next = tmp->next;
        } else {
          *head = tmp->next;
        }
        pid = tmp->pid;
        
        // Grant a resource request - no need to populate the request fulfillment
        // because that information isn't necessary for the child to acknowledge it
        // Allocate the resource and update the request
        message grant_request_message;
        grant_request_message.mtype = pid;
        
        update_request(pid, tmp->request_index, 0);
        increment_allocated(pid, tmp->request_index, 1);
        increment_available(tmp->request_index, -1);
        
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
        
        num_times_resource_granted++;
        
        if (VERBOSE) {
          sem_wait(sh_m_sem);
          printf("OSS: Process %d granting R%llu after blocking, System clock: %llu %llu\n", pid, tmp->request_index, sh_clock_ptr[0], sh_clock_ptr[1]);
          if (lfp != NULL) {
            fprintf(lfp, "OSS: Process %d granting R%llu after blocking, System clock: %llu %llu\n", pid, tmp->request_index, sh_clock_ptr[0], sh_clock_ptr[1]);
          }
          sem_post(sh_m_sem);
          num_of_lines++;
        }
 
        free(tmp);
        return pid;
      }
      prev = tmp;
      tmp = tmp->next;
    }
  }
  
  return pid;
}

/******************* MAIN *******************/
int main(int argc, char** argv) {
  // Create the semaphore
  // Extra semaphore info found here:
  // https://stackoverflow.com/questions/32205396/share-posix-semaphore-among-multiple-processes
  sh_m_sem = sem_open(SEMAPHORE_NAME, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, 1);
  if (sh_m_sem == SEM_FAILED) {
    error("FAILED TO CREATE THE SHARED SEMAPHORE", 0, lfp);
  }

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
  
    // Check to see if we can grant any requests to any blocked processes
    while (queue_unblock_if_resource_is_available(&blocked_head) != -1);
  
    // Ending condition of 100,000 lines of output
    if (num_of_lines >= 100000) {
      printf("\nMore than 100,000 lines in output, ending system simulation\n");
      if (lfp != NULL) {
        fprintf(lfp, "\nMore than 100,000 lines in output, ending system simulation\n");
      }
      
      kill(getpid(), SIGPROF);
    }
    
    // Ending condition of five seconds (another precaution in case the process gets blocked and takes longer than five seconds)
    if (time(0) - init_time >= 5) {
      printf("\nFive seconds have passed, ending system simulation\n");
      if (lfp != NULL) {
        fprintf(lfp, "\nFive seconds have passed, ending system simulation\n");
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

        int insert_sucess = insert_process(process_table, cpid, sh_clock_ptr[0], sh_clock_ptr[1]);
        
        // insert the pid translation
        for (int i = 0; i < MAX_PROCESSES; i++) {
          if (index_to_pid[i] == -1) {
            index_to_pid[i] = cpid;
            break;
          }
        }
        
        if (!insert_sucess) {
          dealloc_queue(&blocked_head);
          kill_all_processes(process_table);
          kill(cpid, SIGKILL);
          clean_msg_queue(msg_id);
          clean_shared_clock(sh_clock_ptr, sh_clock_id);
          sem_close(sh_m_sem);
          sem_unlink(SEMAPHORE_NAME);
          
          char* err_msg = "UNABLE TO INSERT ANOTHER PROCESS INTO THE PROCESS TABLE";
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
    received_message.have_sent = 0;
    int is_there_message = msgrcv(msg_id, &received_message, sizeof(message), getpid(), IPC_NOWAIT);
    
    // If there is a message, then process it
    if (is_there_message != -1) {
      // Child has indicated that it is ending
      if (received_message.ending == 1) {
        if (VERBOSE) {
          printf("OSS: Process %d ending\n", received_message.pid);
          if (lfp != NULL) {
            fprintf(lfp, "OSS: Process %d ending\n", received_message.pid);
          }
          num_of_lines++;
        }
      
        // Free the resources and update statistic and runtime information
        end_process_and_free_resources(process_table, &blocked_head, received_message.pid, allocated_matrix, request_matrix, available);
        num_of_processes_terminated_normally++;
        total_processes_finished++;
        executing_processes--;
        
      // Child has indicated that it does not want to free a resource, which means it wants to request one
      } else if (received_message.free_resource == 0) {
        if (VERBOSE) {
          sem_wait(sh_m_sem);     
          printf("OSS: Process %d requesting R%d, System clock: %llu %llu\n", received_message.pid, received_message.resource_index, sh_clock_ptr[0], sh_clock_ptr[1]);
          if (lfp != NULL) {
            fprintf(lfp, "OSS: Process %d requesting R%d, System clock: %llu %llu\n", received_message.pid, received_message.resource_index, sh_clock_ptr[0], sh_clock_ptr[1]);
          }
          sem_post(sh_m_sem);
          num_of_lines++;
        }
      
        // Generate a resource request message
        message grant_request_message;
        grant_request_message.mtype = received_message.pid;
        
        // Update the requests
        update_request(received_message.pid, received_message.resource_index, 1);
        
        // Check if we can actually give a resource to said process
        if (check_if_available(received_message.resource_index) == -1) {
          if (VERBOSE) {
            sem_wait(sh_m_sem);
            printf("OSS: Process %d's request of R%d was denied, inserting into the blocked queue, System clock: %llu %llu\n", received_message.pid, received_message.resource_index, sh_clock_ptr[0], sh_clock_ptr[1]);
            if (lfp != NULL) {
              fprintf(lfp, "OSS: Process %d's request of R%d was denied, inserting into the blocked queue, System clock: %llu %llu\n", received_message.pid, received_message.resource_index, sh_clock_ptr[0], sh_clock_ptr[1]);
            }
            sem_post(sh_m_sem);
            num_of_lines++;
          }
          
          // We cannot grant this resource request, so update information and block the process
          num_times_resource_not_granted_immediately++;
          queue_insert_using_resource_index(&blocked_head, received_message.pid, received_message.resource_index);
          
          // Update clock in critical section
          sem_wait(sh_m_sem);
          sh_clock_ptr[1] += CLOCK_INCREMENT;
          update_if_nano_seconds_went_over_billion();
          
          // Display the resources
          if (num_times_resource_granted % 20 == 0 && num_times_resource_granted != num_times_resource_granted_prev && VERBOSE) {
            num_times_resource_granted_prev = num_times_resource_granted;
            printf("\tAvailable\n");
            if (lfp != NULL) {
              fprintf(lfp, "\tAvailable\n");
            }
            num_of_lines++;
            num_of_lines += display_available(lfp, '\t');
            printf("\tAllocations:\n");
            if (lfp != NULL) {
              fprintf(lfp, "\tAllocations:\n");
            }
            num_of_lines++;
            num_of_lines += display_resources(lfp, '\t');
            printf("\tRequests:\n");
            if (lfp != NULL) {
              fprintf(lfp, "\tRequests:\n");
            }
            num_of_lines++;
            num_of_lines += display_requests(lfp, '\t');
          }
          
          // Run deadlock detection
          if (sh_clock_ptr[0] - prev_second > 0) {
            // Perform deadlock detection
            prev_second = sh_clock_ptr[0];
            int size = 0;
            int* deadlock_list = deadlock_detection_list(allocated_matrix, request_matrix, available, index_to_pid, &size);
            double num_initial_deadlocked_processes = size;
            double num_of_processes_terminated = 0;
            num_times_deadlock_detection_ran++;
            num_times_deadlock_detection_started++;
            
            if (VERBOSE) {
              printf("OSS: Running deadlock detection at time: %llu, %llu\n", sh_clock_ptr[0], sh_clock_ptr[1]);
              if (lfp != NULL) {
                fprintf(lfp, "OSS: Running deadlock detection at time: %llu, %llu\n", sh_clock_ptr[0], sh_clock_ptr[1]);
              }
              
              printf("\tAvailable\n");
              if (lfp != NULL) {
                fprintf(lfp, "\tAvailable\n");
              }
              num_of_lines++;
              num_of_lines += display_available(lfp, '\t');
              printf("\tAllocations:\n");
              if (lfp != NULL) {
                fprintf(lfp, "\tAllocations:\n");
              }
              num_of_lines++;
              num_of_lines += display_resources(lfp, '\t');
              printf("\tRequests:\n");
              if (lfp != NULL) {
                fprintf(lfp, "\tRequests:\n");
              }
              num_of_lines++;
              num_of_lines += display_requests(lfp, '\t');
            }
                        
            // If there are deadlocked processes, report it and take action
            if (size > 0) {
              times_deadlocked++;
            
              printf("\tOSS: Detected deadlocked processes: ");
              if (lfp != NULL) {
                fprintf(lfp, "\tOSS: Detected deadlocked processes: ");
              }
              num_of_lines++;

              printf("\tCurrent state of system:\n");
              if (lfp != NULL) {
                fprintf(lfp, "\tCurrent state of system:\n");
              }
              
              for (int i = 0; i < size; i++) {
                printf("%d ", deadlock_list[i]);
                if (lfp != NULL) {
                  fprintf(lfp, "%d ", deadlock_list[i]);
                }
              }
              
              printf("\n");
              if (lfp != NULL) {
                fprintf(lfp, "\n");
              }
              num_of_lines++;
              
              printf("\tAvailable\n");
              if (lfp != NULL) {
                fprintf(lfp, "\tAvailable\n");
              }
              num_of_lines++;
              num_of_lines += display_available(lfp, '\t');
              printf("\tAllocations:\n");
              if (lfp != NULL) {
                fprintf(lfp, "\tAllocations:\n");
              }
              num_of_lines++;
              num_of_lines += display_resources(lfp, '\t');
              printf("\tRequests:\n");
              if (lfp != NULL) {
                fprintf(lfp, "\tRequests:\n");
              }
              num_of_lines++;
              num_of_lines += display_requests(lfp, '\t');
              
              // Continue removing processes until we are no longer deadlocked
              while (size > 0) {
                printf("\tKilling process: %d\n", deadlock_list[0]);
                if (lfp != NULL) {
                  fprintf(lfp, "\tKilling process: %d\n", deadlock_list[0]);
                }
                num_of_lines++;
                
                // Kill process and update information
                end_process_and_free_resources(process_table, &blocked_head, deadlock_list[0], allocated_matrix, request_matrix, available);
                num_of_processes_terminated_by_deadlock_detection++;
                num_of_processes_terminated++;
                total_processes_finished++;
                executing_processes--;
                
                printf("\tAvailable\n");
                if (lfp != NULL) {
                  fprintf(lfp, "\tAvailable\n");
                }
                num_of_lines++;
                num_of_lines += display_available(lfp, '\t');
                printf("\tAllocations:\n");
                if (lfp != NULL) {
                  fprintf(lfp, "\tAllocations:\n");
                }
                num_of_lines++;
                num_of_lines += display_resources(lfp, '\t');
                printf("\tRequests:\n");
                if (lfp != NULL) {
                  fprintf(lfp, "\tRequests:\n");
                }
                num_of_lines++;
                num_of_lines += display_requests(lfp, '\t');
                
                free(deadlock_list);
                deadlock_list = deadlock_detection_list(allocated_matrix, request_matrix, available, index_to_pid, &size);
                num_times_deadlock_detection_ran++;
              }
              
              printf("OSS: Deadlock removed from the system\n");
              if (lfp != NULL) {
                fprintf(lfp, "OSS: Deadlock removed from the system\n");
              }
              num_of_lines++;
              
              total_percent_of_processes_terminated_by_deadlock_recovery += num_of_processes_terminated / num_initial_deadlocked_processes;
              
              free(deadlock_list);
            }
          }
          sem_post(sh_m_sem);
          
          
          // Skip the iteration, we can't grant this request
          continue;
        }
        
        /* WE CAN GRANT A REQUEST IF WE ARE PAST THIS */
        
        // Allocate the resource and update the request
        update_request(received_message.pid, received_message.resource_index, 0);
        increment_allocated(received_message.pid, received_message.resource_index, 1);
        increment_available(received_message.resource_index, -1);
        
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
        
        if (VERBOSE) {
          sem_wait(sh_m_sem);
          printf("OSS: Process %d granting R%d immediately, System clock: %llu %llu\n", received_message.pid, received_message.resource_index, sh_clock_ptr[0], sh_clock_ptr[1]);
          if (lfp != NULL) {
            fprintf(lfp,"OSS: Process %d granting R%d immediately, System clock: %llu %llu\n", received_message.pid, received_message.resource_index, sh_clock_ptr[0], sh_clock_ptr[1] );
          }
          sem_post(sh_m_sem);
          num_of_lines++;
        }
        
        // Update statistics information
        num_times_resource_granted_immediately++;
        num_times_resource_granted++;
        
      // If we aren't ending or requesting a resource, then we are releasing a resources
      } else {
        if (VERBOSE) {
          sem_wait(sh_m_sem);
          printf("OSS: Process %d releasing R%d, System clock: %llu %llu\n", received_message.pid, received_message.resource_index, sh_clock_ptr[0], sh_clock_ptr[1]);
          if (lfp != NULL) {
            fprintf(lfp, "OSS: Process %d releasing R%d, System clock: %llu %llu\n", received_message.pid, received_message.resource_index, sh_clock_ptr[0], sh_clock_ptr[1]);
          }
          sem_post(sh_m_sem);
          num_of_lines++;
        }
      
        // Grant a resource release
        message grant_request_message;
        grant_request_message.mtype = received_message.pid;
        
        increment_allocated(received_message.pid, received_message.resource_index, -1);
        increment_available(received_message.resource_index, 1);
        
        // Send the resource release message
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
    }
    
    /* THIS IS DONE TWICE SINCE IF WE BLOCK A PROCESS WE SKIP THE ITERATION WITH CONTINUE */
    
    // Update clock in critical section
    sem_wait(sh_m_sem);
    sh_clock_ptr[1] += CLOCK_INCREMENT;
    update_if_nano_seconds_went_over_billion();

    // Display the resources
    if (num_times_resource_granted % 20 == 0 && num_times_resource_granted != num_times_resource_granted_prev && VERBOSE) {
      num_times_resource_granted_prev = num_times_resource_granted;
      
      printf("\tAvailable\n");
      if (lfp != NULL) {
        fprintf(lfp, "\tAvailable\n");
      }
      num_of_lines++;
      num_of_lines += display_available(lfp, '\t');
      printf("\tAllocations:\n");
      if (lfp != NULL) {
        fprintf(lfp, "\tAllocations:\n");
      }
      num_of_lines++;
      num_of_lines += display_resources(lfp, '\t');
      printf("\tRequests:\n");
      if (lfp != NULL) {
        fprintf(lfp, "\tRequests:\n");
      }
      num_of_lines++;
      num_of_lines += display_requests(lfp, '\t');
    }
    
    // Run deadlock detection
    if (sh_clock_ptr[0] - prev_second > 0) {
      // Perform deadlock detection
      prev_second = sh_clock_ptr[0];
      int size = 0;
      int* deadlock_list = deadlock_detection_list(allocated_matrix, request_matrix, available, index_to_pid, &size);
      double num_initial_deadlocked_processes = size;
      double num_of_processes_terminated = 0;
      num_times_deadlock_detection_ran++;
      num_times_deadlock_detection_started++;
      
      if (VERBOSE) {
        printf("OSS: Running deadlock detection at time: %llu, %llu\n", sh_clock_ptr[0], sh_clock_ptr[1]);
        if (lfp != NULL) {
          fprintf(lfp, "OSS: Running deadlock detection at time: %llu, %llu\n", sh_clock_ptr[0], sh_clock_ptr[1]);
        }
        
        printf("\tAvailable\n");
        if (lfp != NULL) {
          fprintf(lfp, "\tAvailable\n");
        }
        num_of_lines++;
        num_of_lines += display_available(lfp, '\t');
        printf("\tAllocations:\n");
        if (lfp != NULL) {
          fprintf(lfp, "\tAllocations:\n");
        }
        num_of_lines++;
        num_of_lines += display_resources(lfp, '\t');
        printf("\tRequests:\n");
        if (lfp != NULL) {
          fprintf(lfp, "\tRequests:\n");
        }
        num_of_lines++;
        num_of_lines += display_requests(lfp, '\t');
      }
      
      // If there are deadlocked processes, report it and take action
      if (size > 0) {
        times_deadlocked++;
      
        printf("\tOSS: Detected deadlocked processes: ");
        if (lfp != NULL) {
          fprintf(lfp, "\tOSS: Detected deadlocked processes: ");
        }
        num_of_lines++;
        
        for (int i = 0; i < size; i++) {
          printf("%d ", deadlock_list[i]);
          if (lfp != NULL) {
            fprintf(lfp, "%d ", deadlock_list[i]);
          }
        }
        
        printf("\n");
        if (lfp != NULL) {
          fprintf(lfp, "\n");
        }
        num_of_lines++;

        printf("\tCurrent state of system:\n");
        if (lfp != NULL) {
          fprintf(lfp, "\tCurrent state of system:\n");
        }
        num_of_lines++;
        printf("\tAvailable\n");
        if (lfp != NULL) {
          fprintf(lfp, "\tAvailable\n");
        }
        num_of_lines++;
        num_of_lines += display_available(lfp, '\t');
        printf("\tAllocations:\n");
        if (lfp != NULL) {
          fprintf(lfp, "\tAllocations:\n");
        }
        num_of_lines++;
        num_of_lines += display_resources(lfp, '\t');
        printf("\tRequests:\n");
        if (lfp != NULL) {
          fprintf(lfp, "\tRequests:\n");
        }
        num_of_lines++;
        num_of_lines += display_requests(lfp, '\t');
        
        // Continue removing processes until we are no longer deadlocked
        while (size > 0) {
          printf("\tKilling process: %d\n", deadlock_list[0]);
          if (lfp != NULL) {
            fprintf(lfp, "\tKilling process: %d\n", deadlock_list[0]);
          }
          num_of_lines++;
          
          // End a process and update the information surrounding it
          end_process_and_free_resources(process_table, &blocked_head, deadlock_list[0], allocated_matrix, request_matrix, available);
          num_of_processes_terminated_by_deadlock_detection++;
          num_of_processes_terminated++;
          total_processes_finished++;
          executing_processes--;
          
          printf("\tAvailable:\n");
          if (lfp != NULL) {
            fprintf(lfp, "\tAvailable:\n");
          }
          num_of_lines++;
          num_of_lines += display_available(lfp, '\t');
          printf("\tAllocations:\n");
          if (lfp != NULL) {
            fprintf(lfp, "\tAllocations:\n");
          }
          num_of_lines++;
          num_of_lines += display_resources(lfp, '\t');
          printf("\tRequests:\n");
          if (lfp != NULL) {
            fprintf(lfp, "\tRequests:\n");
          }
          num_of_lines++;
          num_of_lines += display_requests(lfp, '\t');
          
          free(deadlock_list);
          deadlock_list = deadlock_detection_list(allocated_matrix, request_matrix, available, index_to_pid, &size);
          num_times_deadlock_detection_ran++;
        }
        
        printf("OSS: Deadlock removed from the system\n");
        if (lfp != NULL) {
          fprintf(lfp, "OSS: Deadlock removed from the system\n");
        }
        num_of_lines++;
        
        total_percent_of_processes_terminated_by_deadlock_recovery += num_of_processes_terminated / num_initial_deadlocked_processes;
        
        free(deadlock_list);
      }
    }
    sem_post(sh_m_sem);
  }
  
  // Print out the ending message
  printf("\n%d PROCESSES HAVE COMPLETED IN THE SYSTEM\n\n", TOTAL_PROCESSES_TO_SPAWN);
  if (lfp != NULL) {
    fprintf(lfp, "\n%d PROCESSES HAVE COMPLETED IN THE SYSTEM\n\n", TOTAL_PROCESSES_TO_SPAWN);
  }
  
  print_statistics(lfp, times_deadlocked, num_times_resource_granted_immediately, num_times_resource_not_granted_immediately, num_of_processes_terminated_by_deadlock_detection, num_of_processes_terminated_normally, num_times_deadlock_detection_ran, total_percent_of_processes_terminated_by_deadlock_recovery);
  
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