#!/bin/bash

for ASM_PATH in "$@"
do
  nasm "$ASM_PATH" -o "test_asm_diff_nasm.out"
  ./dev/sim86 "test_asm_diff_nasm.out" > "test_asm_diff_sim86.asm"
  nasm "test_asm_diff_sim86.asm" -o "test_asm_diff_sim86.out"

  cmp -s "test_asm_diff_nasm.out" "test_asm_diff_sim86.out"
  if [ $? -ne 0 ]
  then
    echo "File $ASM_PATH could not be disassembled correctly"

    rm "test_asm_diff_sim86.asm"
    rm "test_asm_diff_nasm.out"
    rm "test_asm_diff_sim86.out"

    exit 1
  fi
done

rm "test_asm_diff_sim86.asm"
rm "test_asm_diff_nasm.out"
rm "test_asm_diff_sim86.out"

echo "All files disassembled correctly"



