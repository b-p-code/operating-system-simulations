#!/bin/bash

# CS4760 Bryce Paubel
# Simple test script to run oss for multiple cases

# NOTE - After running these tests, ipcs and ps were used to check if there was any shared memory or processes left behind

# This project should be run visually, confirming the outputs of these files visually
echo
echo "***** RUNNING OSS *****"
echo

./oss

./oss -f test_log_file_with_random_name.txt

# Using a quick word count/find
# Found here: https://www.tecmint.com/count-word-occurrences-in-linux-text-file/
# These values should match the number of times scheduled/blocked, and the scheduler
# value should be 10000 times the number displayed here
grep -o -i "OSS: Scheduling" test_log_file_with_random_name.txt | wc -l
grep -o -i "OSS: Blocking" test_log_file_with_random_name.txt | wc -l
grep -o -i "Scheduling the scheduler" test_log_file_with_random_name.txt | wc -l

rm test_log_file_with_random_name.txt

echo
echo "***** RUNNING TEST QUEUE *****"
echo
gcc queue_test.c queue.c -o test_queue_executable

./test_queue_executable

rm test_queue_executable