// CS4760 4/18/23 Bryce Paubel Project 5
// Resource descriptors

#include "resource.h"

// Request matrix
int request_matrix[MAX_PROCESSES][NUMBER_OF_UNIQUE_RESOURCES] = {
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

// Allocated matrix
int allocated_matrix[MAX_PROCESSES][NUMBER_OF_UNIQUE_RESOURCES] = {
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

// Resource vector
int resources[NUMBER_OF_UNIQUE_RESOURCES] = {
  NUMBER_OF_RESOURCES_EACH,
  NUMBER_OF_RESOURCES_EACH,
  NUMBER_OF_RESOURCES_EACH,
  NUMBER_OF_RESOURCES_EACH,
  NUMBER_OF_RESOURCES_EACH,
  NUMBER_OF_RESOURCES_EACH,
  NUMBER_OF_RESOURCES_EACH,
  NUMBER_OF_RESOURCES_EACH,
  NUMBER_OF_RESOURCES_EACH,
  NUMBER_OF_RESOURCES_EACH
};

// Available vector
int available[NUMBER_OF_UNIQUE_RESOURCES] = {
  NUMBER_OF_RESOURCES_EACH,
  NUMBER_OF_RESOURCES_EACH,
  NUMBER_OF_RESOURCES_EACH,
  NUMBER_OF_RESOURCES_EACH,
  NUMBER_OF_RESOURCES_EACH,
  NUMBER_OF_RESOURCES_EACH,
  NUMBER_OF_RESOURCES_EACH,
  NUMBER_OF_RESOURCES_EACH,
  NUMBER_OF_RESOURCES_EACH,
  NUMBER_OF_RESOURCES_EACH
};

// Index to pid array for storing which index has what pid
int index_to_pid[] = {
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1
};

/*
  display_resources
  Displays the current resources in use for the system, the allocations
  
  Input: FILE* lfp, char c
  lfp : Log file pointer to write to
  c   : A special character we may want to add to the front of each line
  
  Output: int
  Returns the number of lines written to output/file
*/
int display_resources(FILE* lfp, char c) {
  int lines_written = 0;
  
  printf("%c", c);
  if (lfp != NULL) {
    fprintf(lfp, "%c", c);
  }
  printf("PID    ");
  if (lfp != NULL) {
    fprintf(lfp, "PID    ");
  }
  
  for (int i = 0; i < NUMBER_OF_UNIQUE_RESOURCES; i++) {
    printf("R%d ", i);
    if (lfp != NULL) {
      fprintf(lfp, "R%d ", i);
    }
  }
  
  printf("\n");
  if (lfp != NULL) {
    fprintf(lfp, "\n");
  }
  lines_written++;
  
  for (int i = 0; i < MAX_PROCESSES; i++) {
    printf("%c", c);
    if (lfp != NULL) {
      fprintf(lfp, "%c", c);
    }
    printf("%-6d ", index_to_pid[i]);
    if (lfp != NULL) {
      fprintf(lfp, "%-6d ", index_to_pid[i]);
    }
    
    for (int j = 0; j < NUMBER_OF_UNIQUE_RESOURCES; j++) {
      printf("%-2d ", allocated_matrix[i][j]);
      if (lfp != NULL) {
        fprintf(lfp, "%-2d ", allocated_matrix[i][j]);
      }
    }
    
    printf("\n");
    if (lfp != NULL) {
      fprintf(lfp, "\n");
    }
    lines_written++;
  }
  
  return lines_written;
}


/*
  display_requests
  Displays the current requests made in the system
  
  Input: FILE* lfp, char c
  lfp : Log file pointer to write to
  c   : A special character we may want to add to the front of each line
  
  Output: int
  Returns the number of lines written to output/file
*/
int display_requests(FILE* lfp, char c) {
  int lines_written = 0;
  printf("%c", c);
  if (lfp != NULL) {
    fprintf(lfp, "%c", c);
  }
  printf("PID    ");
  if (lfp != NULL) {
    fprintf(lfp, "PID    ");
  }
  
  for (int i = 0; i < NUMBER_OF_UNIQUE_RESOURCES; i++) {
    printf("R%d ", i);
    if (lfp != NULL) {
      fprintf(lfp, "R%d ", i);
    }
  }

  printf("\n");
  if (lfp != NULL) {
    fprintf(lfp, "\n");
  }
  lines_written++;
  
  for (int i = 0; i < MAX_PROCESSES; i++) {
     printf("%c", c);
    if (lfp != NULL) {
      fprintf(lfp, "%c", c);
    }
    printf("%-6d ", index_to_pid[i]);
    if (lfp != NULL) {
      fprintf(lfp, "%-6d ", index_to_pid[i]);
    }
    
    for (int j = 0; j < NUMBER_OF_UNIQUE_RESOURCES; j++) {
      printf("%-2d ", request_matrix[i][j]);
      if (lfp != NULL) {
        fprintf(lfp, "%-2d ", request_matrix[i][j]);
      }
    }
    
    printf("\n");
    if (lfp != NULL) {
      fprintf(lfp, "\n");
    }
    lines_written++;
  }
  
  return lines_written;
}

/*
  display_available
  Displays the current available resources in the system
  
  Input: FILE* lfp, char c
  lfp : Log file pointer to write to
  c   : A special character we may want to add to the front of each line
  
  Output: int
  Returns the number of lines written to output/file
*/
int display_available(FILE* lfp, char c) {
  int lines_written = 0;
  printf("%c", c);
  if (lfp != NULL) {
    fprintf(lfp, "%c", c);
  }
  for (int i = 0; i < NUMBER_OF_UNIQUE_RESOURCES; i++) {
    printf("R%d ", i);
    if (lfp != NULL) {
      fprintf(lfp, "R%d ", i);
    }
  } 
  printf("\n");
  if (lfp != NULL) {
    fprintf(lfp, "\n");
  }
  lines_written++;
  
  printf("%c", c);
  if (lfp != NULL) {
    fprintf(lfp, "%c", c);
  }
  for (int i = 0; i < NUMBER_OF_UNIQUE_RESOURCES; i++) {
    printf("%-2d ", available[i]);
    if (lfp != NULL) {
      fprintf(lfp, "%-2d ", available[i]);
    }
  }
  printf("\n");
  if (lfp != NULL) {
    fprintf(lfp, "\n");
  }
  lines_written++;
  
  return lines_written;
}

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
int update_request(int pid, int index, int value_to_update) {
  for (int i = 0; i < MAX_PROCESSES; i++) {
    if (index_to_pid[i] == pid) {
      request_matrix[i][index] = value_to_update;
      return 1;
    }
  }
  
  return 0;
}

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
int update_allocated(int pid, int index, int value_to_update) {
  for (int i = 0; i < MAX_PROCESSES; i++) {
    if (index_to_pid[i] == pid) {
      allocated_matrix[i][index] = value_to_update;
      return 1;
    }
  }
  
  return 0;
}

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
int increment_allocated(int pid, int index, int value_to_update) {
  for (int i = 0; i < MAX_PROCESSES; i++) {
    if (index_to_pid[i] == pid) {
      allocated_matrix[i][index] += value_to_update;
      return 1;
    }
  }
  
  return 0;
}

/*
  increment_available
  Increments the value of a given available vector
  
  Input: int index, int value_to_update
  index           : The resource index that we would like to increment
  value_to_update : The value we would like to increment the resource by
  
  Output: int
  Returns the success of the function
*/
int increment_available(int resource_index, int value) {
  if (resource_index < 0 || resource_index >= NUMBER_OF_UNIQUE_RESOURCES) {
    return -1;
  }
  
  available[resource_index] += value;
  return 1;
}


/*
  check_if_available
  A function used to check if a given resource is available for allocation
  
  Input: int resource_index
  resource_index : Index of the resource that we would like to check if available
  
  Output: int
  Returns the index of the resource if available, -1 if not available
*/
int check_if_available(int resource_index) {
  if (resource_index < 0 || resource_index >= NUMBER_OF_UNIQUE_RESOURCES) {
    return -1;
  }
  
  if (available[resource_index] > 0) {
    return resource_index;
  }
  
  return -1;
}


/*
  pid_to_index
  Translates a given pid into an index
  
  Input: int index
  pid : Process PID that we would like to translate into an index
  
  Output: int
  Returns the success of the function
*/
int pid_to_index(int pid) {
  for (int i = 0; i < MAX_PROCESSES; i++) {
    if (index_to_pid[i] == pid) {
      return i;
    }
  }
  
  return -1;
}