// CS4760 3/16/2023 Project 4 Bryce Paubel
// Definitions for queue operations

#include <stdio.h>
#include <stdlib.h>

#include "queue.h"
#include "process.h"

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
int queue_insert(node** head, int pid, unsigned long int blocked_end_seconds, unsigned long int blocked_end_nano_seconds, int index, int page, int read) {
  struct node* tmp = *head;
  
  if (tmp == NULL) {
    *head = malloc(sizeof(node));
    (*head)->next = NULL;
    (*head)->blocked_end_seconds = blocked_end_seconds;
    (*head)->blocked_end_nano_seconds = blocked_end_nano_seconds;
    (*head)->request_index = index;
    (*head)->page_index = page;
    (*head)->pid = pid;
    (*head)->read = read;
    (*head)->s = 0;
    (*head)->ns = 0;
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
  new_node->request_index = index;
  new_node->page_index = page;
  new_node->read = read;
  new_node->s = 0;
  new_node->ns = 0;
  
  tmp->next = new_node;
  
  return 1;
}

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
int queue_insert_using_resource_index(node** head, int pid, unsigned long long int request_index) {
  struct node* tmp = *head;
  
  if (tmp == NULL) {
    *head = malloc(sizeof(node));
    (*head)->next = NULL;
    (*head)->request_index = request_index;
    (*head)->pid = pid;
    return 1;
  }
  
  while (tmp->next != NULL) {
    tmp = tmp->next;
  }
  
  node* new_node = malloc(sizeof(node));
  
  new_node->next = NULL;
  new_node->pid = pid;
  new_node->request_index = request_index;
  
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
  
  Output: node
  A copy of the node we removed (pid is -1 if we haven't found one) 
*/
node queue_remove_blocked(node** head, unsigned long int blocked_end_seconds, unsigned long int blocked_end_nano_seconds) {
  struct node return_node;
  struct node* tmp = *head;
  struct node* prev = NULL;
  
  return_node.pid = -1;
  
  if (tmp == NULL) {
    return return_node;
  } else {
  
    if (tmp->next == NULL && tmp->blocked_end_seconds <= blocked_end_seconds && tmp->blocked_end_nano_seconds <= blocked_end_nano_seconds) {
      return_node.pid = (*head)->pid; 
      return_node.blocked_end_seconds = (*head)->blocked_end_seconds;
      return_node.blocked_end_nano_seconds = (*head)->blocked_end_nano_seconds;
      return_node.request_index = (*head)->request_index;
      return_node.page_index = (*head)->page_index;
      return_node.read = (*head)->read;
      return_node.s = (*head)->s;
      return_node.ns = (*head)->ns;
      free(*head);
      *head = NULL;
      return return_node;
    }
  
    while (tmp != NULL) {
      if (tmp->blocked_end_seconds <= blocked_end_seconds && tmp->blocked_end_nano_seconds <= blocked_end_nano_seconds) {
        if (prev != NULL) {
          prev->next = tmp->next;
        } else {
          *head = tmp->next;
        }
        
        return_node.pid = tmp->pid;
        return_node.blocked_end_seconds = tmp->blocked_end_seconds;
        return_node.blocked_end_nano_seconds = tmp->blocked_end_nano_seconds;
        return_node.request_index = tmp->request_index;
        return_node.page_index = tmp->page_index;
        return_node.read = tmp->read;
        return_node.s = tmp->s;
        return_node.ns = tmp->ns;
        
        free(tmp);
        return return_node;
      }
      prev = tmp;
      tmp = tmp->next;
    }
  }
  
  return return_node;
}

/*
  queue_remove_by_pid
  Removes a node in the queue based on a pid
  
  Input: node** head, int pid_param
  head      : Head of the queue
  pid_param : The pid of the node we would like to remove
  
  Output: node
  A copy of the node we removed (pid is -1 if we haven't found one)
*/
node queue_remove_by_pid(node** head, int pid_param) {
  struct node return_node;
  struct node* tmp = *head;
  struct node* prev = NULL;
  
  return_node.pid = -1;
  
  if (tmp == NULL) {
    return return_node;
  } else {
  
    if (tmp->next == NULL && tmp->pid == pid_param) {
      return_node.pid = (*head)->pid; 
      return_node.blocked_end_seconds = (*head)->blocked_end_seconds;
      return_node.blocked_end_nano_seconds = (*head)->blocked_end_nano_seconds;
      return_node.request_index = (*head)->request_index;
      return_node.read = (*head)->read;
      return_node.s = (*head)->s;
      return_node.ns = (*head)->ns;
      free(*head);
      *head = NULL;
      
      
      return return_node;
    }
  
    while (tmp != NULL) {
      if (tmp->pid == pid_param) {
        if (prev != NULL) {
          prev->next = tmp->next;
        } else {
          *head = tmp->next;
        }
        return_node.pid = tmp->pid;
        return_node.blocked_end_seconds = tmp->blocked_end_seconds;
        return_node.blocked_end_nano_seconds = tmp->blocked_end_nano_seconds;
        return_node.request_index = tmp->request_index;
        return_node.read = tmp->read;
        return_node.s = tmp->s;
        return_node.ns = tmp->ns;
 
        free(tmp);
        return return_node;
      }
      prev = tmp;
      tmp = tmp->next;
    }
  }
  
  return return_node;
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
  increment_time
  increments the time an item spends in the queue
  
  Input: node** head, unsigned long long int ns
  head   : A pointer to the node pointer, done so we can have direct access to this node
  ns     : The nanoseconds to add
  
  Output: void
*/
void increment_time(node** head, unsigned long long int ns) {
  struct node* tmp1 = *head;
  while (tmp1 != NULL) {
    tmp1->ns += ns;
    update_time_if_billion(&tmp1->s, &tmp1->ns);
    tmp1 = tmp1->next;
  }
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
