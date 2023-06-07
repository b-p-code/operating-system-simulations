// CS4760 Project 3 Bryce Paubel 3/14/23
// Definition for message type, r_msg, random message

#ifndef MESSAGE_H
#define MESSAGE_H

typedef struct {
   long mtype;
   int pid;
   int location;
   int reading;
   int ending;
} message;

#endif