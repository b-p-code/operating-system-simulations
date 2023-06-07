// CS4760 Bryce Paubel 3/16/23
// Custom queue declaration implemented as a linked list

#ifndef QUEUE_H
#define QUEUE_H

#include "process.h"

// Linked list node for our queue 
typedef struct node {
  int pid;
  unsigned long long int blocked_end_seconds;
  unsigned long long int blocked_end_nano_seconds;
  struct node* next;
} node;

/*
  queue_insert
  Inserts a PCB into the queue (equivalent to enqueue)
  
  Input: node** head, int pid, unsigned long int blocked_end_seconds, unsigned long int blocked_end_nano_seconds
  head                    : A pointer to the node pointer, done so we can have direct access to this node
  pid                     : process id to insert
  blocked_end_seconds     : the second at which a process will be unblocked
  blocked_end_nano_seconds: the nano second at which a process will be unblocked
  
  Output: int
  Integer to boolean to indicate failure or success  
*/
int queue_insert(node** head, int pid, unsigned long int blocked_end_seconds, unsigned long int blocked_end_nano_seconds);

/*
  queue_remove
  Removes a PCB into the queue (equivalent to dequeue)
  
  Input: node** head
  head   : A pointer to the node pointer, done so we can have direct access to this node
  
  Output: int
  The found process pid (-1 returned if not found) 
*/
int queue_remove(node** head);

/*
  queue_remove_blocked
  Removes a PCB into the queue (equivalent to dequeue) based on whether any processes could be unblocked
  
  Input: node** head, unsigned long int blocked_end_seconds, unsigned long int blocked_end_nano_seconds
  head                    : A pointer to the node pointer, done so we can have direct access to this node
  blocked_end_seconds     : the second at which a process will be unblocked
  blocked_end_nano_seconds: the nano second at which a process will be unblocked
  
  Output: PCB
  The found process (all values are -1 in the structure if the process doesn't exist in the queue) 
*/
int queue_remove_blocked(node** head, unsigned long int blocked_end_seconds, unsigned long int blocked_end_nano_seconds);

/*
  dealloc_queue
  deallocates the items in the queue
  
  Input: node** head
  head   : A pointer to the node pointer, done so we can have direct access to this node
  
  Output: int
  Integer to boolean to indicate failure or success  
*/
int dealloc_queue(node** head);

/*
  display_queue
  Displays the processes in the queue
  
  Input: node** head
  head   : A pointer to the node pointer, done so we can have direct access to this node
  
  Output: void
*/
void display_queue(node** head);

#endif