#!/bin/bash

SUCCESS=true

for ASM_PATH in "$@"
do
  nasm "$ASM_PATH" -o "test_asm_diff_nasm.out"
  ./dev/sim86 "decompile" "test_asm_diff_nasm.out" > "test_asm_diff_sim86.asm"
  nasm "test_asm_diff_sim86.asm" -o "test_asm_diff_sim86.out"

  diff "test_asm_diff_nasm.out" "test_asm_diff_sim86.out"
  if [ $? -ne 0 ]
  then
    echo "File $ASM_PATH could not be disassembled correctly"
    SUCCESS=false
  fi
done

rm "test_asm_diff_sim86.asm"
rm "test_asm_diff_nasm.out"
rm "test_asm_diff_sim86.out"

if [ $SUCCESS = true ]
then
  echo "All files decompiled correctly"
  exit 0
else
  exit 1
fi
