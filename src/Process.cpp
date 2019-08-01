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

#include <cstring>

#include <string>
#include <fstream>
#include <iostream>
#include <regex>

#include <sys/mman.h>

#include "unjit.hpp"

namespace unjit {

Process::Process(pid_t pid) : pid_(pid)
{
}

Process::~Process()
{

}

void Process::load_vm_maps()
{
  vmas_.clear();
  std::string filename =
    std::string("/proc/") + std::to_string(this->pid_) + std::string("/maps");
  std::regex map_regex(
    // Start-end:
    "^([0-9a-f]+)-([0-9a-f]+)"
    // Flags:
    " ([r-])([w-])([x-])([p-])"
    // Offset:
    " ([0-9a-f]+)"
    // Device (major/minor):
    " [0-9a-f]+:[0-9a-f]+"
    // icode:
    " [0-9]*"
    // File name:
    " *(.*)$"
  );
  std::ifstream file(filename);

  std::string line;
  std::smatch match;

  while (getline(file, line)) {
    if (!std::regex_search(line, match, map_regex))
      continue;
    Vma vma;
    vma.start  = strtoll(match[1].str().c_str(), nullptr, 16);
    vma.end    = strtoll(match[2].str().c_str(), nullptr, 16);
    vma.prot   = 0;
    if (match[3].str()[0] == 'r')
      vma.prot |= PROT_READ;
    if (match[4].str()[0] == 'w')
      vma.prot |= PROT_WRITE;
    if (match[5].str()[0] == 'x')
      vma.prot |= PROT_EXEC;
    if (match[6].str()[0] == 'p')
      vma.flags = MAP_PRIVATE;
    else
      vma.flags = MAP_SHARED;
    vma.offset = strtoll(match[7].str().c_str(), nullptr, 16);
    vma.name   = match[8];
    this->vmas_.push_back(std::move(vma));
  }
}

void Process::load_modules()
{
  this->modules_.clear();
  size_t n = this->vmas_.size();
  for (size_t i = 0; i != n; ++ i) {

    Vma const& vma = this->vmas_[i];
    if (vma.name.empty() || vma.name[0] == '[')
      continue;

    Module module = load_module(vma.start, vma.name);
    if (!module.name.empty())
      this->modules_.push_back(std::move(module));
    while (i + 1 < n && this->vmas_[i + 1].name == vma.name) ++i;
  }
}

void Process::load_map_file()
{
  std::string filename =
    std::string("/tmp/perf-") + std::to_string(this->pid_) + std::string(".map");
  this->load_map_file(filename);
}

void Process::load_map_file(std::string const& map_file)
{
  std::ifstream file(map_file);
  std::string line;
  while (getline(file, line)) {
    Symbol symbol;
    int name_index;
    if (sscanf(line.c_str(), "%" SCNx64 " %" SCNx64 "%n", &symbol.value, &symbol.size, &name_index) == 2) {
      while ((std::string::size_type) name_index < line.size() && line[name_index] == ' ')
        ++name_index;
      symbol.name = std::string(line.c_str() + name_index);
      this->jit_symbols_[symbol.value] = std::move(symbol);
    }
  }
}

const char* Process::lookup_symbol(std::uint64_t ReferenceValue)
{
  {
    auto i = this->jit_symbols_.find(ReferenceValue);
    if (i != this->jit_symbols_.end())
      return i->second.name.c_str();
  }
  const Symbol* symbol = this->find_symbol(ReferenceValue);
  if (symbol != nullptr)
    return symbol->name.c_str();
  return nullptr;
}

}
