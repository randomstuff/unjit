#include <stdio.h>

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <fstream>
#include <string>
#include <iostream>
#include <unordered_map>

#include <llvm-c/Target.h>
#include <llvm-c/Disassembler.h>

#include <sys/types.h>
#include <sys/uio.h>

#include "unjit.hpp"

const char* lookup_symbol(
  void *DisInfo,
  uint64_t ReferenceValue,
  uint64_t *ReferenceType,
  uint64_t ReferencePC,
  const char **ReferenceName)
{
  DisContext *context = (DisContext *) DisInfo;
  *ReferenceType = 0;
  *ReferenceName = NULL;

  auto i = context->jit_symbols.find(ReferenceValue);
  if (i != context->jit_symbols.end())
    return i->second.name.c_str();

  return NULL;
}

void parse_symbol_stream(FILE* file, DisContext& context)
{
  size_t size = 256;
  char* line = (char*) malloc(size);
  while (getline(&line, &size, file) >= 0) {
    symbol sym;
    char buffer[size];
    if (sscanf(line, "%" SCNx64 " %" SCNx64 " %s", &sym.start, &sym.size, buffer) == 3) {
      sym.name = std::string(buffer);
      context.jit_symbols[sym.start] = std::move(sym);
    }
  }
}

void parse_symbol_file(const char* filename, DisContext& context)
{
  if (std::strcmp(filename, "-") == 0) {
    parse_symbol_stream(stdin, context);
    return;
  }

  unique_file file = unique_file(std::fopen(filename, "r"));
  if (!file) {
    std::fprintf(stderr, "Could not open file %s\n", filename);
    return;
  }
  parse_symbol_stream(file.get(), context);
}

void disassemble(std::FILE *file, LLVMDisasmContextRef dis, uint8_t *code, size_t size, uint64_t pc)
{
  char temp[256];
  while (size) {
    size_t c = LLVMDisasmInstruction(dis,
      code, size, pc, temp, sizeof(temp));
    if (c == 0)
      return;
    std::fprintf(file, "%016" PRIx64 "%s\n", pc, temp);
    size -= c;
    code += c;
    pc += c;
  }
}

void disassemble_sym(std::FILE* file, LLVMDisasmContextRef dis, DisContext const& context, symbol const& sym)
{
  uint8_t buffer[sym.size];
  struct iovec local, remote;

  local.iov_base = buffer;
  local.iov_len = sym.size;

  remote.iov_base = (void*)sym.start;
  remote.iov_len = sym.size;

  if (process_vm_readv(context.pid, &local, 1, &remote, 1, 0) != sym.size) {
    std::fprintf(stderr, "Error, could not read the instructions for %s\n", sym.name.c_str());
    return;
  }

  std::fprintf(file, "%s\n", sym.name.c_str());
  disassemble(file, dis, buffer, sym.size, sym.start);
  std::fprintf(file, "\n");
}

int main(int argc, const char** argv)
{
  LLVMInitializeAllTargetInfos();
  LLVMInitializeAllTargetMCs();
  LLVMInitializeAllDisassemblers();
  LLVMInitializeNativeDisassembler();

  DisContext context;
  context.pid = std::atoll(argv[1]);

  // Fetch the JITed symbols:
  if (argc <= 2) {
    std::string filename = std::string("/tmp/perf-") + std::to_string(context.pid) + std::string(".map");
    parse_symbol_file(filename.c_str(), context);
  } else for (size_t i = 2; i < argc ; ++i) {
    parse_symbol_file(argv[i], context);
  }

  // Create and setup the disassembler:
  LLVMDisasmContextRef dis = LLVMCreateDisasmCPU(
    LLVM_HOST_TRIPLE, "core2",
    &context, 0, NULL, lookup_symbol);
  if (!dis) {
    std::fputs("Could not create disassembler.\n", stderr);
    return 1;
  }
  LLVMSetDisasmOptions(dis,
    LLVMDisassembler_Option_SetInstrComments
    | LLVMDisassembler_Option_PrintLatency
    );

  // Disassemble:
  for (auto const& k : context.jit_symbols)
    disassemble_sym(stdout, dis, context, k.second);

  LLVMDisasmDispose(dis);
  return 0;
}
