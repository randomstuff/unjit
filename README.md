# unjit

## Overview

### Features

* disassemble code from a living process;

* based on LLVM disassembler;

* by default disassemble all (JITed) subprograms found in `/tmp/perf-$pid.map`;

* symbolication of JIT-ed symbols using `/tmp/perf-$pid.map`;

* symbolication of AOT symbols using ELF `SHT_SYMTAB` and `SHT_DYNSYM` sections;

* does not `ptrace`, does not stop the process;

* output similar to the output of `objdump` and compatible with
  what Linux `perf` expects.

### Compatibility

* currently working on Linux 3.2 (`process_vm_readv()`) and a suitable libc

## Usage

### Basic usage

~~~sh
unjit -p $pid > dis.txt
~~~

 1. Find the JIT-ed function from a process from `/tmp/perf-$pid.map`;

 2. Read the corresponding instructions from the remote process memory;

 3. Disassemble them to stdout.

### Using with perf

~~~sh
perf top -p $pid --objdump ./perfobjdump
~~~

## Discussion

### Linux `perf` map (`/tmp/perf-$pid.map`)

The `/tmp/perf-$pid.map` is a file used by JIT compilers to tell Linux
perf the location and name of JITed subprograms. The format is:

~~~
$startAddressInHexa $sizeInHexa $name
~~~

Example:

~~~
41f3ae82 34 foo
41f3aec6 52 bar
~~~

## Roadmap

Without any specific order:

* better detection of modules;

* disassemble by symbol name;

* symbolicate GOT and PLT addresses;

* load symbols from `DT_SYMTAB`;

* load symbols from DWARF (optional);

* load DWARF info from a separate file;

* do not hardcode the CPU model (CLI option);

* select the native CPU model by default;

* [Capstone](http://www.capstone-engine.org/) support.
