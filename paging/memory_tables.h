// CS4760 Project 6 Bryce Paubel 4/26/23
// Declarations for the memory tables used and utility functions

#ifndef MEMORY_TABLES_H
#define MEMORY_TABLES_H

#include "process.h"

#define FRAME_SIZE 1024
#define NUM_OF_FRAMES 256
#define PAGES_PER_PROCESS 32

// Queue 'pointer'
extern int head;

// Datatype for every table entry
typedef struct {
  int pid;
  int dirty_bit;
  int num;
  int claimed;
} table_entry;

// The memory frame table for main memory
table_entry frame_table[NUM_OF_FRAMES];

// The page tables for each process
table_entry page_tables[MAX_PROCESSES][PAGES_PER_PROCESS];

// Translation array from an index to a pid
int index_to_pid[MAX_PROCESSES];

/*
  init_memory_tables
  Initializes the memory tables
  
  Input: int init
  init : The value you would like to use to initialize the data 
  
  Output: void
*/
void init_memory_tables(int init);

/*
  find_open_frame
  Finds the location of a currently open frame
  
  Input: void
  
  Output: int
  Location (index) of a currently open frame
  Returns -1 if there are no currently open frames
*/
int find_open_frame();

/*
  display_frames
  Displays the frames in the system
  
  Input: FILE* lfp
  lfp : The file pointer to write output to as well as stdout
  
  Output: int
  Returns the number of lines written
*/
int display_frames(FILE* lfp);

// To insert: free_frames, free_page_table

/*
  insert_page_into_frame
  Takes a given logical address and stores it in the corresponding frame
  
  Input: int pid, int page_num, int frame_num
  pid       : The process id for the given page
  page_num  : The page number for this process'
  frame_num : The frame to insert this page into
  
  Output: int
  Returns the passed pid if successful, -1 otherwise
*/
int insert_page_into_frame(int pid, int page_num, int frame_num);

/*
  insert_process_into_page_table
  Inserts a given process into the page table, essentially allocated some memory for it
  
  Input: int pid
  pid : The pid of the process that we would like to insert into the page table
  
  Output: int
  The pid passed if successful, -1 if failed
*/
int insert_process_into_page_table(int pid);

/*
  pid_to_index
  Translates a pid into its corresponding index
  
  Intput: int pid
  pid : The pid of the process you would like to search

  Output:
  The corresponding pid that matches this index, or -1 if it is not found
*/
int pid_to_index(int pid);

/*
  get_frame_from_head
  Gets the frame from the head and increments the head
  
  Input: void
  
  Output: int
  The head of memory to replace
*/
int get_frame_from_head();

/*
  swap_frame
  Performs a frame swap
  
  Input: int pid, int page_num, int frame_index
  pid         : the pid of the process that is putting its page into memory
  page_num    : the page number of the page we are putting into memory
  frame_index : the frame we would like to put the page in
  
  Output: int
  Returns the passed pid if successful, -1 if failure
*/
int swap_frame(int pid, int page_num, int frame_index);

/*
  remove_page_table_data
  Removes a processes page table
  
  Input: int pid, int init
  pid  : The process that we would like to remove
  init : The value we would like to replace stuff with
  
  Output: int
  If successesful the pid is returned, otherwise -1 is returned
*/
int remove_page_and_frame_table_data(int pid, int init);

#endif