#!/bin/bash

SUCCESS=true

for ASM_PATH in "$@"
do
  TXT_PATH="${ASM_PATH%.*}.txt"

  nasm "$ASM_PATH" -o "test_run.out"

  ./dev/sim86 "trace" "test_run.out" > "test_run_trace.txt"
  if [ $? -ne 0 ]
  then
    echo "Error while running $ASM_PATH"
    SUCCESS=false
  fi

  cmp -s "$TXT_PATH" "test_run_trace.txt"
  if [ $? -ne 0 ]
  then
    echo "File $ASM_PATH run trace doesn't match $TXT_PATH"
    SUCCESS=false
  fi
done

rm "test_run_trace.txt"
rm "test_run.out"

if [ $SUCCESS = true ]
then
  echo "All tests ran correctly"
  exit 0
else
  exit 1
fi
