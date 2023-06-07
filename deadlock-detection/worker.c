// CS4760 Project 5 Bryce Paubel 4/18/23
// Simple worker program determining if it should end, block itself, or take up a full quantum

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

#include "message.h"
#include "process.h"
#include "resource.h"

#define MAX_RAND_NUMBER 100
#define PERCENTAGE_CONSTANT_TO_TERMINATE 98
#define PERCENTAGE_CONSTANT_TO_REQUEST 98
#define MAXIMUM_NANOSECONDS_TO_END_PROCESS 250000000
#define MAXIMUM_NANOSECONDS_TO_MAKE_REQUEST 5000
#define SEMAPHORE_NAME "GERALD_THE_SEMAPHORE"

// help
// Prints out a help message
//
// Input: int
// Boolean for determining if the help is for an error message or just for help
// 
// Output: void
void help(int error) {
	if (error) {
		fprintf(stderr, "\n\tUSAGE: ./worker number_of_iterations\n\n");
	} else {
		printf("\n\tUSAGE: ./workern number_of_iterations\n\n");
	}
}

// error
// Prints out an error message and exits the program
// 
// Input: char*, int
// Error message to print out and boolean determining if a help message should be printed as well
//
// Output: void
void error(char* error_message, int print_help) {
	fprintf(stderr, "\nERROR: %s\n", error_message);

	if (print_help) {
		help(1);
	}

	exit(EXIT_FAILURE);
}

int main(int argc, char** argv) {

  sem_t *sh_m_sem = sem_open(SEMAPHORE_NAME, O_RDWR);
  if (sh_m_sem == SEM_FAILED) {
      error("FAILED TO CONNECT TO SEMAPHORE", 0);
      exit(EXIT_FAILURE);
  }

  // Child worker keeps track of the resources given to it
  int resources[NUMBER_OF_UNIQUE_RESOURCES] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  
  // Some arbitrary seed value
  srand(getpid() + (int)rand() + 10);
	// Argument check
	if (argc != 1) {
		error("IMPROPER AMOUNT OF ARGUMENTS", 1);
	}

	// Get shared memory clock
	const int sh_clock_key = ftok("oss.c", 0);
  // Get message queue id
  const int m_key = ftok("oss.c", 0);
  
	int sh_clock_id = shmget(sh_clock_key, sizeof(unsigned long long int) * 2, 0777);
	unsigned long long int* sh_clock_ptr = shmat(sh_clock_id, 0, 0);
 
 // Error check if we can't attach to the clockipcs
	if (sh_clock_ptr <= 0) {
		error("UNABLE TO ATTACH TO SHARED MEMORY", 0);
	}

  // Connect to the queue
  int m_id = msgget(m_key, 0777);
  if (m_id < 0) {
    error("UNABLE TO CONNECT TO THE MESSAGE QUEUE", 0);
  }

  // Intialize random interval values
  sem_wait(sh_m_sem);
  unsigned long long int random_sec = sh_clock_ptr[0];
  unsigned long long int random_nano_sec = sh_clock_ptr[1] + rand() % MAXIMUM_NANOSECONDS_TO_MAKE_REQUEST;
  update_time_if_billion(&random_sec, &random_nano_sec);
  
  unsigned long long int random_end_sec = sh_clock_ptr[0];
  unsigned long long int random_end_nano_sec = sh_clock_ptr[1] + rand() % MAXIMUM_NANOSECONDS_TO_END_PROCESS;
  update_time_if_billion(&random_end_sec, &random_end_nano_sec);
  sem_post(sh_m_sem);
  
  // Loop until we should end
  int end = 0;
  while (end == 0) {
    // Get clock values
    sem_wait(sh_m_sem);
    unsigned long long int temp_second = sh_clock_ptr[0];
    unsigned long long int temp_nano_second = sh_clock_ptr[1];
    sem_post(sh_m_sem);
    
    // If we have gone past the potential end interval, determine if we should end
    if (((random_end_sec <= temp_second && random_end_nano_sec <= temp_nano_second) || (random_end_sec < temp_second)) && temp_second >= 1) {
      int random_num = rand() % MAX_RAND_NUMBER;
      
      // End
      if (random_num >= PERCENTAGE_CONSTANT_TO_TERMINATE) {
          // Alert the parent that we are ending
          message message_to_send;
          message_to_send.mtype = getppid();
          message_to_send.pid = getpid();
          message_to_send.ending = 1;
          if (msgsnd(m_id, &message_to_send, sizeof(message) - sizeof(long), 0)) {
            char* err_msg = "UNABLE TO SEND MESSAGE";
            error(err_msg, 0);
          }
          
        return EXIT_SUCCESS; 
      
      // If we don't end, generate a new interval
      } else {
         random_end_sec = temp_second;
         random_end_nano_sec = temp_nano_second + rand() % MAXIMUM_NANOSECONDS_TO_END_PROCESS;
         update_time_if_billion(&random_end_sec, &random_end_nano_sec);
      }
    }
    
    // Determine if we are past our release/request interval
    if ((random_sec <= temp_second && random_nano_sec <= temp_nano_second) || (random_sec < temp_second)) {
      int random_num = rand() % MAX_RAND_NUMBER;
      
      // Request a resource
      if (random_num <= PERCENTAGE_CONSTANT_TO_REQUEST) {
        // First, generate a random number from 0-9
        int resource_request_index = (int)rand() % NUMBER_OF_UNIQUE_RESOURCES;
        
        // Error case check in case we have all resources, in which case we just loop again
        int all_requested = 1;
        for (int i = 0; i < NUMBER_OF_UNIQUE_RESOURCES; i++) {
          if (resources[i] != 20) {
            all_requested = 0;
            break;
          }
        }
        
        // If we haven't requested all of the resources, generate a resource request
        // Otherwise, we won't make any requests
        if (!all_requested) {
          // If we have requested all of our resource at our random resource index, continue looking for another index
          if (resources[resource_request_index] >= 20) {
            for (int i = 0; i < NUMBER_OF_UNIQUE_RESOURCES; i++) {
              if (resources[i] < 20) {
                resource_request_index = i;
                break;
              }
            }
          }
        
          // Send a resource request to the parent
          // This is the case in which we want to get a resource
          message message_to_send;
          message_to_send.mtype = getppid();
          message_to_send.pid = getpid();
          message_to_send.have_sent = 1;
          message_to_send.resource_index = resource_request_index;
          message_to_send.free_resource = 0;
          message_to_send.ending = 0;
          if (msgsnd(m_id, &message_to_send, sizeof(message) - sizeof(long), 0)) {
            char* err_msg = "UNABLE TO SEND MESSAGE";
            error(err_msg, 0);
          }
          
          // Wait for OSS to grant this resource request
          message message_received;
          if (msgrcv(m_id, &message_received, sizeof(message), getpid(), 0) < 0) {
            error("FAILED TO GET MESSAGE FROM PARENT", 0);
          }
          
          // Keep track of how many resources we have
          resources[resource_request_index]++;
        }
        
      // Releasing a resource
      } else {
        // First, generate a random number from 0-9
        int resource_request_index = (int)rand() % NUMBER_OF_UNIQUE_RESOURCES;
        
        // Error case check in case we have no resources, in which case we just loop again
        int none_requested = 1;
        for (int i = 0; i < NUMBER_OF_UNIQUE_RESOURCES; i++) {
          if (resources[i] != 0) {
            none_requested = 0;
            break;
          }
        }
        
        // If we have resources, then release one
        if (!none_requested) {
          // Since we know at least one of the indices cannot be zero, keep randomly looping until we get one of those indices
          while (resources[resource_request_index] == 0) {
            resource_request_index = (int)rand() % NUMBER_OF_UNIQUE_RESOURCES;
          }
        
          // Send a resource release to parent
          message message_to_send;
          message_to_send.mtype = getppid();
          message_to_send.pid = getpid();
          message_to_send.have_sent = 1;
          message_to_send.resource_index = resource_request_index;
          message_to_send.free_resource = 1;
          message_to_send.ending = 0;
          if (msgsnd(m_id, &message_to_send, sizeof(message) - sizeof(long), 0)) {
            char* err_msg = "UNABLE TO SEND MESSAGE";
            error(err_msg, 0);
          }
          
          // Wait for OSS to grant this resource request
          message message_received;
          if (msgrcv(m_id, &message_received, sizeof(message), getpid(), 0) < 0) {
            error("FAILED TO GET MESSAGE FROM PARENT", 0);
          }
          
          // Keep track of our resources
          resources[resource_request_index]--;
        }
      }
      
      // Update the random request interval
      sem_wait(sh_m_sem);       
      random_sec = sh_clock_ptr[0];
      random_nano_sec = sh_clock_ptr[1] + rand() % MAXIMUM_NANOSECONDS_TO_MAKE_REQUEST;
      update_time_if_billion(&random_sec, &random_nano_sec);
      sem_post(sh_m_sem);
    }
  }
  
  sem_close(sh_m_sem);
	return EXIT_SUCCESS;
}
