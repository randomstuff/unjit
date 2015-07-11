/* The MIT License (MIT)

Copyright (c) 2015 Gabriel Corona

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifndef UNJIT_UNJIT_HPP
#define UNJIT_UNJIT_HPP

#include <sys/types.h> // pid_t

#include <cinttypes>  // uint64_t
#include <string>
#include <memory>     // unique_ptr
#include <unordered_map>
#include <iostream>
#include <vector>

#include <llvm-c/Target.h>
#include <llvm-c/Disassembler.h>

namespace unjit {

struct Symbol {
  std::uint64_t start;
  std::uint64_t size;
  std::string name;
};

struct Vma {
  uint64_t start, end;
  int prot; // PROT_EXEC, PROT_READ, PROT_WRITE, PROT_NONE
  int flags; // MAP_SHARED, MAP_PRIVATE
  uint64_t offset;
  std::string name;
};

std::ostream& operator<<(std::ostream& stream, Vma const& vma);

class Process {
private:
  pid_t pid_;
  std::unordered_map<std::uint64_t, Symbol> jit_symbols_;
  std::vector<Vma> vmas_;

public:
  Process(pid_t pid);
  ~Process();
  void load_vm_maps();
  void load_map_file();
  void load_map_file(std::string const& map_file);
  const char* lookup_symbol(uint64_t ReferenceValue);

  std::unordered_map<std::uint64_t, Symbol> const& jit_symbols()
  {
    return jit_symbols_;
  }

  pid_t pid()
  {
    return pid_;
  }

  std::vector<Vma> vm_maps()
  {
    return vmas_;
  }

};

class Disassembler {
private:
  Process* process_;
  LLVMDisasmContextRef disassembler_;
public:
  Disassembler(Process& process);
  ~Disassembler();
  void disassemble(std::ostream& stream, uint8_t *code, size_t size, uint64_t pc);
  void disassemble(std::ostream& stream, Symbol const& symbol);
};

}

#endif
