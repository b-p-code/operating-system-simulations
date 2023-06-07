#!/bin/bash

# CS4760 Bryce Paubel
# Simple test script to run oss for multiple cases

# NOTE - this is taken exactly from project 1/2/3/4 (although modified), since they have the same arguments
# NOTE - After running these tests, ipcs and ps were used to check if there was any shared memory or processes left behind

# This project should be run visually, confirming the outputs of these files visually
echo
echo "***** RUNNING OSS *****"
echo

./oss

./oss -f test_log_file_with_random_name.txt

# Using a quick word count/find
# Found here: https://www.tecmint.com/count-word-occurrences-in-linux-text-file/
# These values should match the number of times scheduled/blocked
grep -o -i "OSS: Deadlock removed" test_log_file_with_random_name.txt | wc -l
grep -o -i "OSS: Detected" test_log_file_with_random_name.txt | wc -l
grep -o -i "immediately," test_log_file_with_random_name.txt | wc -l
grep -o -i "denied" test_log_file_with_random_name.txt | wc -l
grep -o -i "Killing" test_log_file_with_random_name.txt | wc -l
grep -o -i "ending" test_log_file_with_random_name.txt | wc -l

rm test_log_file_with_random_name.txt

echo
echo "***** RUNNING TEST QUEUE *****"
echo
gcc queue_test.c queue.c -o test_queue_executable

./test_queue_executable

rm test_queue_executable