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

#include "unjit.hpp"

#include <sys/uio.h>

#include <stdexcept>

#include <llvm-c/Target.h>
#include <llvm-c/Disassembler.h>

// This is an adapter for the API expected by LLVMDisasmContextRef:
static const char* lookup_symbol(
  void *DisInfo,
  uint64_t ReferenceValue,
  uint64_t *ReferenceType,
  uint64_t ReferencePC,
  const char **ReferenceName)
{
  unjit::Process *process = (unjit::Process *) DisInfo;
  *ReferenceType = 0;
  *ReferenceName = NULL;
  return process->lookup_symbol(ReferenceValue);
}

namespace unjit
{

Disassembler::Disassembler(Process& process) : process_(&process)
{
  // Create and setup the disassembler:
  this->disassembler_ = LLVMCreateDisasmCPU(
    LLVM_HOST_TRIPLE, "core2",
    this->process_, 0, NULL, lookup_symbol);
  if (!this->disassembler_) {
    throw std::runtime_error("Could not intialize LLVM disassembler");
  }
  LLVMSetDisasmOptions(this->disassembler_,
    LLVMDisassembler_Option_SetInstrComments
    | LLVMDisassembler_Option_PrintLatency
    );
}

Disassembler::~Disassembler()
{
  LLVMDisasmDispose(this->disassembler_);
}

void Disassembler::disassemble(std::FILE *file, uint8_t *code, size_t size, uint64_t pc)
{
  char temp[256];
  while (size) {
    size_t c = LLVMDisasmInstruction(this->disassembler_,
      code, size, pc, temp, sizeof(temp));
    if (c == 0)
      return;
    std::fprintf(file, "%016" PRIx64 "%s\n", pc, temp);
    size -= c;
    code += c;
    pc += c;
  }
}

void Disassembler::disassemble(std::FILE* file, Symbol const& symbol)
{
  uint8_t buffer[symbol.size];
  struct iovec local, remote;

  local.iov_base = buffer;
  local.iov_len = symbol.size;

  remote.iov_base = (void*) symbol.start;
  remote.iov_len = symbol.size;

  if (process_vm_readv(this->process_->pid(), &local, 1, &remote, 1, 0) != symbol.size) {
    // TODO, return/throw error
    std::fprintf(stderr, "Error, could not read the instructions for %s\n", symbol.name.c_str());
    return;
  }

  std::fprintf(file, "%s\n", symbol.name.c_str());
  this->disassemble(file, buffer, symbol.size, symbol.start);
  std::fprintf(file, "\n");
}

}
