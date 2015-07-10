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

#include "unjit.hpp"

namespace unjit {

Process::Process(pid_t pid) : pid_(pid)
{
  jit_symbols_.clear();
  std::string filename =
    std::string("/tmp/perf-") + std::to_string(this->pid_) + std::string(".map");
  this->load_map_file(filename);
}

Process::~Process()
{

}

void Process::load_map_file(std::string const& map_file)
{
  std::ifstream file(map_file);

  size_t size = 256;
  std::string line;
  while (getline(file, line)) {
    Symbol symbol;
    int name_index;
    if (sscanf(line.c_str(), "%" SCNx64 " %" SCNx64 "%n", &symbol.start, &symbol.size, &name_index) == 2) {
      while (name_index < line.size() && line[name_index] == ' ')
        ++name_index;
      symbol.name = std::string(line.c_str() + name_index);
      this->jit_symbols_[symbol.start] = std::move(symbol);
    }
  }
}

const char* Process::lookup_symbol(uint64_t ReferenceValue)
{
  auto i = this->jit_symbols_.find(ReferenceValue);
  if (i != this->jit_symbols_.end())
    return i->second.name.c_str();
  return NULL;
}

}
