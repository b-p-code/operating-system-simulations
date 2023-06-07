// Project 4 Bryce Paubel CS4760 4/1/23
// Quick program to test queue implementations
// Compile: qcc queue_test.c queue.c -o test

#include <stdio.h>

#include "queue.h"

int main() {
  node* queue_head;
  
  queue_insert(&queue_head, -1, 1, 1);
  display_queue(&queue_head);
  
  queue_insert(&queue_head, -2, 2, 2);
  display_queue(&queue_head);
  
  queue_insert(&queue_head, -3, 3, 3);
  display_queue(&queue_head);
  
  queue_remove(&queue_head);
  display_queue(&queue_head);
  
  queue_insert(&queue_head, -4, 4, 4);
  display_queue(&queue_head);
  
  queue_remove(&queue_head);
  display_queue(&queue_head);
  
  queue_remove(&queue_head);
  display_queue(&queue_head);
  
  queue_remove(&queue_head);
  display_queue(&queue_head);
  
  queue_remove(&queue_head);
  display_queue(&queue_head);
  
  queue_remove(&queue_head);
  display_queue(&queue_head);
  
  dealloc_queue(&queue_head);
  
  return 0;
}