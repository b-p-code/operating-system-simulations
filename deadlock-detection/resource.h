// CS4760 4/18/23 Bryce Paubel Project 5
// Resource descriptors

#ifndef RESOURCE_H
#define RESOURCE_H

#include "process.h"

#define NUMBER_OF_UNIQUE_RESOURCES 10
#define NUMBER_OF_RESOURCES_EACH 20

// Request matrix
int request_matrix[MAX_PROCESSES][NUMBER_OF_UNIQUE_RESOURCES];
// Allocation matrix
int allocated_matrix[MAX_PROCESSES][NUMBER_OF_UNIQUE_RESOURCES];

// Resource vector
int resources[NUMBER_OF_UNIQUE_RESOURCES];
// Available vector
int available[NUMBER_OF_UNIQUE_RESOURCES];

// Index to pid array for storing which index has what pid
int index_to_pid[MAX_PROCESSES];

/*
  display_resources
  Displays the current resources in use for the system, the allocations
  
  Input: FILE* lfp, char c
  lfp : Log file pointer to write to
  c   : A special character we may want to add to the front of each line
  
  Output: int
  Returns the number of lines written to output/file
*/
int display_resources(FILE*, char);

/*
  display_requests
  Displays the current requests made in the system
  
  Input: FILE* lfp, char c
  lfp : Log file pointer to write to
  c   : A special character we may want to add to the front of each line
  
  Output: int
  Returns the number of lines written to output/file
*/
int display_requests(FILE*, char);

/*
  display_available
  Displays the current available resources in the system
  
  Input: FILE* lfp, char c
  lfp : Log file pointer to write to
  c   : A special character we may want to add to the front of each line
  
  Output: int
  Returns the number of lines written to output/file
*/
int display_available(FILE*, char);


/*
  update_request
  Updates the value of a given request
  
  Input: int pid, int index, int value_to_update
  pid             : Process ID that we would like to update
  index           : The resource index that we would like to update
  value_to_update : The value we would like to update that resource to
  
  Output: int
  Returns the success of the function
*/
int update_request(int, int, int);

/*
  update_allocated
  Updates the value of a given allocation
  
  Input: int pid, int index, int value_to_update
  pid             : Process ID that we would like to update
  index           : The resource index that we would like to update
  value_to_update : The value we would like to update that resource to
  
  Output: int
  Returns the success of the function
*/
int update_allocated(int, int, int);

/*
  increment_allocated
  Increments the value of a given allocation
  
  Input: int pid, int index, int value_to_update
  pid             : Process ID that we would like to update
  index           : The resource index that we would like to increment
  value_to_update : The value we would like to increment the resource by
  
  Output: int
  Returns the success of the function
*/
int increment_allocated(int, int, int);

/*
  increment_available
  Increments the value of a given available vector
  
  Input: int index, int value_to_update
  index           : The resource index that we would like to increment
  value_to_update : The value we would like to increment the resource by
  
  Output: int
  Returns the success of the function
*/
int increment_available(int, int);

/*
  check_if_available
  A function used to check if a given resource is available for allocation
  
  Input: int resource_index
  resource_index : Index of the resource that we would like to check if available
  
  Output: int
  Returns the index of the resource if available, -1 if not available
*/
int check_if_available(int);

/*
  pid_to_index
  Translates a given pid into an index
  
  Input: int index
  pid : Process PID that we would like to translate into an index
  
  Output: int
  Returns the success of the function
*/
int pid_to_index(int);

#endif