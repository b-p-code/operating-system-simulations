// CS4760 3/16/2023 Project 4 Bryce Paubel
// Definitions for queue operations

#include <stdio.h>
#include <stdlib.h>

#include "queue.h"

/*
  queue_insert
  Inserts a PCB into the queue (equivalent to enqueue)
  
  Input: node** head, PCB process
  head   : A pointer to the node pointer, done so we can have direct access to this node
  process: the process to insert
  
  Output: int
  Integer to boolean to indicate failure or success  
*/
int queue_insert(node** head, int pid, unsigned long int blocked_end_seconds, unsigned long int blocked_end_nano_seconds) {
  struct node* tmp = *head;
  
  if (tmp == NULL) {
    *head = malloc(sizeof(node));
    (*head)->next = NULL;
    (*head)->blocked_end_seconds = blocked_end_seconds;
    (*head)->blocked_end_nano_seconds = blocked_end_nano_seconds;
    (*head)->pid = pid;
    return 1;
  }
  
  while (tmp->next != NULL) {
    tmp = tmp->next;
  }
  
  node* new_node = malloc(sizeof(node));
  
  new_node->next = NULL;
  new_node->pid = pid;
  new_node->blocked_end_seconds = blocked_end_seconds;
  new_node->blocked_end_nano_seconds = blocked_end_nano_seconds;
  
  tmp->next = new_node;
  
  return 1;
}

/*
  queue_remove
  Removes a PCB into the queue (equivalent to dequeue)
  
  Input: node** head
  head   : A pointer to the node pointer, done so we can have direct access to this node

  Output: PCB
  The found process (all values are -1 in the structure if the process doesn't exist in the queue) 
*/
int queue_remove(node** head) {
  struct node* tmp = *head;
  int pid = -1;
  
  if (tmp == NULL) {
    return pid;
  } else {
    node* next = tmp->next;
    pid = tmp->pid;
    free(tmp);
    *head = next;
    return pid;
  }
  
  return pid;
}

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
int queue_remove_blocked(node** head, unsigned long int blocked_end_seconds, unsigned long int blocked_end_nano_seconds) {
  struct node* tmp = *head;
  struct node* prev = NULL;
  int pid = -1;
  
  if (tmp == NULL) {
    return pid;
  } else {
  
    if (tmp->next == NULL && tmp->blocked_end_seconds <= blocked_end_seconds && tmp->blocked_end_nano_seconds <= blocked_end_nano_seconds) {
      pid = (*head)->pid;
      free(*head);
      *head = NULL;
      return pid;
    }
  
    while (tmp != NULL) {
      if (tmp->blocked_end_seconds <= blocked_end_seconds && tmp->blocked_end_nano_seconds <= blocked_end_nano_seconds) {
        if (prev != NULL) {
          prev->next = tmp->next;
        } else {
          *head = tmp->next;
        }
        
        pid = tmp->pid;
        free(tmp);
        return pid;
      }
      prev = tmp;
      tmp = tmp->next;
    }
  }
  
  return pid;
}

/*
  dealloc_queue
  deallocates the items in the queue
  
  Input: node** head
  head   : A pointer to the node pointer, done so we can have direct access to this node
  
  Output: int
  Integer to boolean to indicate failure or success  
*/
int dealloc_queue(node** head) {
  struct node* tmp1 = *head;
  while (tmp1 != NULL) {
    printf("Dealloced node: %d\n", tmp1->pid);
    struct node* tmp2 = tmp1->next;
    free(tmp1);
    tmp1 = tmp2;
  }
  *head = NULL;
  return 1;
}

/*
  display_queue
  Displays the processes in the queue
  
  Input: node** head
  head   : A pointer to the node pointer, done so we can have direct access to this node
  
  Output: void
*/
void display_queue(node** head) {
 struct node* tmp = *head;
 printf("%p\n", tmp);
 
 while (tmp != NULL) {
   printf("%d %llu %llu\n", tmp->pid, tmp->blocked_end_seconds, tmp->blocked_end_nano_seconds);
   tmp = tmp->next;
 }
}
