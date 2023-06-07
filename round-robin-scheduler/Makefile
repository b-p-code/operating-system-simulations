# CS4760 Project 3 Makefile Bryce Paubel 3/1/23
# Based heavily on a Makefile I wrote in CS4280 which was based on a Makefile I wrote based on examples in CS2750
# Also based on the makefiles from project1/project2

OSS_SRC = oss.c process.c process.h message.h queue.c queue.h
WORKER_SRC = worker.c message.h

OSS_OBJ = oss.o process.o queue.o
WORKER_OBJ = worker.o

OSS_TARGET = oss
WORKER_TARGET = worker

COMPILER = gcc
FLAGS = -std=c99 -Wall -g

# all as used in the makefile shown on Canvas
all: $(OSS_TARGET) $(WORKER_TARGET)

$(OSS_TARGET): $(OSS_OBJ)
	$(COMPILER) $(FLAGS) $^ -o $@

# Create equivalent object files for each OSS source
%.o: %.c $(OSS_SRC)
	$(COMPILER) $(FLAGS) -c $< -o $@

$(WORKER_TARGET): $(WORKER_OBJ)
	$(COMPILER) $(FLAGS) $^ -o $@

# Create equivalent object files for each WORKER source
%.o: %.c $(WORKER_SRC)
	$(COMPILER) $(FLAGS) -c $< -o $@

.PHONY clean:
	rm $(OSS_OBJ) $(WORKER_OBJ) $(OSS_TARGET) $(WORKER_TARGET)