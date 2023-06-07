// CS4760 Bryce Paubel 3/16/23
// Custom queue declaration implemented as a linked list

#ifndef QUEUE_H
#define QUEUE_H

#include "process.h"
#include "resource.h"

// Linked list node for our queue 
typedef struct node {
  int pid;
  unsigned long long int blocked_end_seconds;
  unsigned long long int blocked_end_nano_seconds;
  unsigned long long int request_index;
  unsigned long long int page_index;
  unsigned long long int s;
  unsigned long long int ns;
  int read;
  struct node* next;
} node;

/*
  queue_insert
  Inserts a PCB into the queue (equivalent to enqueue)
  
  Input: node** head, int pid, unsigned long int blocked_end_seconds, unsigned long int blocked_end_nano_seconds, int index, int page, int read
  head                    : A pointer to the node pointer, done so we can have direct access to this node
  pid                     : process id to insert
  blocked_end_seconds     : the second at which a process will be unblocked
  blocked_end_nano_seconds: the nano second at which a process will be unblocked
  index                   : the frame the process wants
  page                    : the page the process wants to put into memory
  read                    : integer boolean indicating if the memory request is a read or write
  
  Output: int
  Integer to boolean to indicate failure or success  
*/
int queue_insert(node** head, int pid, unsigned long int blocked_end_seconds, unsigned long int blocked_end_nano_seconds, int index, int page, int read);

/*
  queue_insert_using_resource_index
  Inserts a pid into the queue but with the resource index instead of blocked time (equivalent to enqueue)
  
  Input: node** head, int pid, unsigned long long int resource_index
  head           : A pointer to the node pointer, done so we can have direct access to this node
  pid            : The process id to insert
  resource_index : The resource of the index that we are waiting on
  
  Output: int
  Integer to boolean to indicate failure or success  
*/
int queue_insert_using_resource_index(node** head, int pid, unsigned long long int request_index);

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
  
  Output: node
  A copy of the node we removed (pid is -1 if we haven't found one) 
*/
node queue_remove_blocked(node** head, unsigned long int blocked_end_seconds, unsigned long int blocked_end_nano_seconds);

/*
  queue_remove_by_pid
  Removes a node in the queue based on a pid
  
  Input: node** head, int pid_param
  head      : Head of the queue
  pid_param : The pid of the node we would like to remove
  
  Output: node
  A copy of the node we removed (pid is -1 if we haven't found one)
*/
node queue_remove_by_pid(node** head, int pid_param);

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
  increment_time
  increments the time an item spends in the queue
  
  Input: node** head, unsigned long long int ns
  head   : A pointer to the node pointer, done so we can have direct access to this node
  ns     : The nanoseconds to add
  
  Output: void
*/
void increment_time(node** head, unsigned long long int ns);

/*
  display_queue
  Displays the processes in the queue
  
  Input: node** head
  head   : A pointer to the node pointer, done so we can have direct access to this node
  
  Output: void
*/
void display_queue(node** head);

#endif