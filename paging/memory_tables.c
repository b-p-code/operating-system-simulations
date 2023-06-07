// CS4760 Project 6 Bryce Paubel 4/26/23
// Definitions for the memory tables used and utility functions

#include "memory_tables.h"

// Queue pointer for the frames
int head = 0;

/*
  init_memory_tables
  Initializes the memory tables
  
  Input: int init
  init : The value you would like to use to initialize the data 
  
  Output: void
*/
void init_memory_tables(int init) {
  for (int i = 0; i < MAX_PROCESSES; i++) {
    for (int j = 0; j < PAGES_PER_PROCESS; j++) {
      table_entry te;
      te.dirty_bit = init;
      te.pid = init;
      te.num = init;
      te.claimed = init;
      page_tables[i][j] = te;
    }
  }
  
  for (int i = 0; i < NUM_OF_FRAMES; i++) {
      table_entry te;
      te.dirty_bit = init;
      te.pid = init;
      te.num = init;
      te.claimed = init;
      frame_table[i] = te; 
  }
  
  for (int i = 0; i < MAX_PROCESSES; i++) {
    index_to_pid[i] = init;
  }
}

/*
  find_open_frame
  Finds the location of a currently open frame
  
  Input: void
  
  Output: int
  Location (index) of a currently open frame
  Returns -1 if there are no currently open frames
*/
int find_open_frame() {

  // Search for an empty frame
  // NOTE - in order to avoid processes from continously using the same open frame
  // before it gets swapped in, we don't actually fill the frame but we do 'claim'
  // it. That way, the program will not continously claim the same open frame, even though that open frame is still 'open'
  for (int i = 0; i < NUM_OF_FRAMES; i++) {
    if (frame_table[i].num < 0 && frame_table[i].claimed != 1) {
      return i;
    }
  }
  
  // Otherwise, return -1 since we couldn't find an open frame
  return -1;
}

/*
  display_frames
  Displays the frames in the system
  
  Input: FILE* lfp
  lfp : The file pointer to write output to as well as stdout
  
  Output: int
  Returns the number of lines written
*/
int display_frames(FILE* lfp) {
  int lines = 0;
  
  printf("Frame #   PID       Page #    Dirty Bit Head\n");
  if (lfp != NULL) {
    fprintf(lfp, "Frame #   PID       Page #    Dirty Bit Head\n");
  }
  lines++;
  
  
  for (int i = 0; i < NUM_OF_FRAMES; i++) {
      printf("%-10d%-10d%-10d%-10d", i, frame_table[i].pid, frame_table[i].num, frame_table[i].dirty_bit);
      if (lfp != NULL) {
        fprintf(lfp, "%-10d%-10d%-10d%-10d", i, frame_table[i].pid, frame_table[i].num, frame_table[i].dirty_bit);
      }
      
      if (i == head) {
        printf("*         \n");
        if (lfp != NULL) {
          fprintf(lfp, "*         \n");
        }
      } else {
        printf("\n");
        if (lfp != NULL) {
          fprintf(lfp, "\n");
        }
      }
      
      lines++;
  }
  
  return lines;
}

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
int insert_page_into_frame(int pid, int page_num, int frame_num) {
  int index = pid_to_index(pid);
  if (index < 0 || page_num < 0 || frame_num < 0) {
    return -1;
  }
  
  frame_table[frame_num].pid = pid;
  frame_table[frame_num].num = page_num;
  
  return pid;
}

/*
  update_page_table
  Takes a given logical address and stores it in the corresponding frame
  
  Input: int pid, int page_num, int frame_num
  pid       : The process id for the given page
  page_num  : The page number for this process'
  frame_num : The frame to insert this page into
  
  Output: int
  Returns the passed pid if successful, -1 otherwise
*/
int update_page_table(int pid, int page_num, int frame_num) {
  int index = pid_to_index(pid);
  if (index < 0 || page_num < 0 || frame_num < 0) {
    return -1;
  }
  
  page_tables[index][page_num].pid = pid;
  page_tables[index][page_num].num = frame_num;
  
  return pid;
}

/*
  insert_process_into_page_table
  Inserts a given process into the page table, essentially delegates some space to it
  
  Input: int pid
  pid : The pid of the process that we would like to insert into the page table
  
  Output: int
  The pid passed if successful, -1 if failed
*/
int insert_process_into_page_table(int pid) {
  int index = -1;
  for (int i = 0; i < MAX_PROCESSES; i++) {
    if (page_tables[i][0].pid < 0) {
      index = i;
      break;
    }
  }
  
  if (index < 0) {
    return -1;
  }
  
  index_to_pid[index] = pid;
  
  for (int i = 0; i < PAGES_PER_PROCESS; i++) {
    page_tables[index][i].pid = pid;
  }
  
  return pid;
}

/*
  pid_to_index
  Translates a pid into its corresponding index
  
  Intput: int pid
  pid : The pid of the process you would like to search

  Output:
  The pid passed, or -1 if it is not found
*/
int pid_to_index(int pid) {
  for (int i = 0; i < MAX_PROCESSES; i++) {
    if (index_to_pid[i] == pid) {
      return i;
    }
  }
  
  return -1;
}

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
int swap_frame(int pid, int page_num, int frame_index) {
  if (frame_index > NUM_OF_FRAMES - 1 || frame_index < 0) {
    return -1;
  }
  
  if (page_num > PAGES_PER_PROCESS - 1 || page_num < 0) {
    return -1;
  }
  
  // Find the pid of the frame and update the frame
  int prev_pid = frame_table[frame_index].pid;
  int prev_num = frame_table[frame_index].num;
  frame_table[frame_index].pid = pid;
  frame_table[frame_index].num = page_num;
  frame_table[frame_index].dirty_bit = 0;
  
  
  // Update the page table of the previous process
  if (prev_pid < 0 || prev_num < 0) {
    return -1; 
  }
  int process_index = pid_to_index(prev_pid);
  if (process_index < 0) {
    return -1;
  }
  
  // Update the page table to indicate that it is no longer in memory
  page_tables[process_index][prev_num].num = -1;

  return pid;
}

/*
  get_frame_from_head
  Gets the frame from the head and increments the head
  
  Input: void
  
  Output: int
  The head of memory to replace
*/
int get_frame_from_head() {
  int temp = head;
  head++;
  head = head % NUM_OF_FRAMES;
  return temp;
}

/*
  remove_page_table_data
  Removes a processes page table
  
  Input: int pid, int init
  pid  : The process that we would like to remove
  init : The value we would like to replace stuff with
  
  Output: int
  If successesful the pid is returned, otherwise -1 is returned
*/
int remove_page_and_frame_table_data(int pid, int init) {
  int index = pid_to_index(pid);
  if (index < 0) {
    return -1;
  }
  
  for (int i = 0; i < PAGES_PER_PROCESS; i++) {
    table_entry te;
    te.dirty_bit = init;
    te.pid = init;
    te.num = init;
    te.claimed = init;
    page_tables[index][i] = te;
  }
  
  for (int i = 0; i < NUM_OF_FRAMES; i++) {
    if (frame_table[i].pid == pid) {
      table_entry te;
      te.dirty_bit = init;
      te.pid = init;
      te.num = init;
      te.claimed = init;
      frame_table[i] = te;
    }
  }
  
  return pid;
}