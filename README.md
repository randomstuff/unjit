# Disassemble JITed code

## Usage

### Basic usage

~~~sh
unjit $pid > dis.txt
~~~

 1. Find the JIT-ed function from a process from `/tmp/perf-$pid.map`;

 2. Read the corresponding instructions from the remote process memory;

 3. Disassemble them to stdout.

## Advanced usage

~~~sh
cat /tmp/perf-$pid.map | grep ^foo | unjit $pid - > dis.txt
~~~

### Features

 * based on LLVM disassembler;

 * symbolication of JIT-ed symbols;

 * does not `ptrace`, stop the process.

### About `/tmp/perf-$pid.map`

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

 * compile on systems without `process_vm_readv`;

 * read symbols from ELF dynamic sections and DWARF debug
   informations;

 * handle other means for receiving the JITed subprograms
   informations,

   * GDB JIT infrastructure (`__jit_debug_register_code`);

 * proper CLI argument parsing;

 * autodetect non-native targets;

 * do not hardcode the CPU model;

 * select the native CPU model by default.
