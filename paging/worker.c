// CS4760 Project 6 Bryce Paubel 5/1/23
// Simple worker program to make memory requests

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
#define PERCENTAGE_CONSTANT_TO_READ 85
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
  unsigned long long int num_of_requests = 0;
  unsigned long long int num_of_requests_rand_offset = rand() % 200 - 100;
  while (end == 0) {
    // Get clock values
    sem_wait(sh_m_sem);
    unsigned long long int temp_second = sh_clock_ptr[0];
    unsigned long long int temp_nano_second = sh_clock_ptr[1];
    sem_post(sh_m_sem);
    
    // If we have gone past the potential end interval, determine if we should end
    if (num_of_requests >= 1000 + num_of_requests_rand_offset) {
      num_of_requests_rand_offset = rand() % 200 - 100;
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
    
    if (((random_sec <= temp_second && random_nano_sec <= temp_nano_second) || (random_sec < temp_second)) && temp_second >= 1) {
      // Send a resource request to the parent
      // This is the case in which we want to get a resource
      message message_to_send;
      message_to_send.mtype = getppid();
      message_to_send.pid = getpid();
      message_to_send.location = (rand() % 32) * 1024 + rand() % 1024;
      message_to_send.reading = rand() % MAX_RAND_NUMBER <= PERCENTAGE_CONSTANT_TO_READ ? 1 : 0;
      message_to_send.ending = 0;
      if (msgsnd(m_id, &message_to_send, sizeof(message) - sizeof(long), 0)) {
        char* err_msg = "UNABLE TO SEND MESSAGE";
        error(err_msg, 0);
      }
      
      num_of_requests++;
      
      // Wait for OSS to grant this resource request
      message message_received;
      if (msgrcv(m_id, &message_received, sizeof(message), getpid(), 0) < 0) {
        error("FAILED TO GET MESSAGE FROM PARENT", 0);
      }
      
      random_sec = temp_second;
      random_nano_sec = temp_nano_second + rand() % MAXIMUM_NANOSECONDS_TO_MAKE_REQUEST;
      update_time_if_billion(&random_sec, &random_nano_sec);
    }
  }
  
	return EXIT_SUCCESS;
}
