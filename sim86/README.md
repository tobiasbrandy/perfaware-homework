# sim86

## Intel 8086/88 decompiler and simulator

### Build Init - Just Once
`cc -o build build.c` 

### Build
`./build`

### Generate compilation database
`./build db`

### Run Decompiler
`./sim86 decompile <src_file>`

### Run Simulation
`./sim86 run <src_file>`

### Run Simulation with tracing enabled
`./sim86 trace <src_file>`

### Test against provided examples
`./build test`

### Clean
`./build clean`
