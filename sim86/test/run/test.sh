#!/bin/bash

for ASM_PATH in "$@"
do
  TXT_PATH="${ASM_PATH%.*}.txt"

  nasm "$ASM_PATH" -o "test_run.out"

  ./dev/sim86 "trace" "test_run.out" > "test_run_trace.txt"
  if [ $? -ne 0 ]
  then
    echo "Error while running $ASM_PATH"

    rm "test_run_trace.txt"
    rm "test_run.out"

    exit 1
  fi

  cmp -s "$TXT_PATH" "test_run_trace.txt"
  if [ $? -ne 0 ]
  then
    echo "File $ASM_PATH run trace doesn't match $TXT_PATH"

    rm "test_run_trace.txt"
    rm "test_run.out"

    exit 1
  fi
done

rm "test_run_trace.txt"
rm "test_run.out"

echo "All files ran correctly"
