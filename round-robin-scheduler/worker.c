// CS4760 Project 4 Bryce Paubel 3/24/23
// Simple worker program determining if it should end, block itself, or take up a full quantum

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>

#include "message.h"

#define LOWER_BOUND 2
#define UPPER_BOUND 98

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
  // Some arbitrary seed value
  srand(getpid());
	// Argument check
	if (argc != 1) {
		error("IMPROPER AMOUNT OF ARGUMENTS", 1);
	}

	// Get shared memory clock
	const int sh_clock_key = ftok("oss.c", 0);
  // Get message queue id
  const int m_key = ftok("oss.c", 0);
  
	int sh_clock_id = shmget(sh_clock_key, sizeof(int) * 2, 0777);
	int* sh_clock_ptr = shmat(sh_clock_id, 0, 0);
 
 // Error check if we can't attach to the clock
	if (sh_clock_ptr <= 0) {
		error("UNABLE TO ATTACH TO SHARED MEMORY", 0);
	}

  // Connect to the queue
  int m_id = msgget(m_key, 0777);
  if (m_id < 0) {
    error("UNABLE TO CONNECT TO THE MESSAGE QUEUE", 0);
  }

  int end = 0;
  while (end == 0) {
    // Get time sent by the parent
    message message_received;
    if (msgrcv(m_id, &message_received, sizeof(message), getpid(), 0) < 0) {
      error("FAILED TO GET MESSAGE FROM PARENT", 0);
    }
    
    message message_to_send;
    
    int rand_val = (int)(rand() % 100);
    if (rand_val < LOWER_BOUND) {
      // Determine how much time quantum was used before being blocked
      int time_quantum_used = -(((int)(rand() % 99 + 1)) / 100.0) * message_received.nano_seconds;
      message_to_send.nano_seconds = time_quantum_used;
    } else if (rand_val >= LOWER_BOUND && rand_val < UPPER_BOUND) {
      // Most of the time we will use all the quantum
      message_to_send.nano_seconds = message_received.nano_seconds;
    } else {
      // Determine how much time quantum was used before terminating
      int time_quantum_used = -(((int)(rand() % 99 + 1)) / 100.0) * message_received.nano_seconds;
      message_to_send.nano_seconds = time_quantum_used;
      end = 1;
    }
    
    message_to_send.mtype = getppid();

    if (msgsnd(m_id, &message_to_send, sizeof(message) - sizeof(long), 0)) {
      char* err_msg = "UNABLE TO SEND MESSAGE";
      error(err_msg, 0);
    }
  }

	return EXIT_SUCCESS;
}
