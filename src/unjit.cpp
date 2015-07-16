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

#include <sys/types.h>

#include <cstdlib> // atoll, exit
#include <cinttypes>

#include <iostream>

#include <llvm-c/Target.h>
#include <llvm-c/Disassembler.h>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options.hpp>

#include "unjit.hpp"

struct Config {
  pid_t pid = -1;
  std::uint64_t start = 0, stop = 0;
};

static unsigned long long int parse_integer(char const* value)
{
  if (value[0] == '0' && value[1] == 'x') {
    return strtoull(value + 2, NULL, 16);
  } else {
    return strtoull(value, NULL, 10);
  }
}

static int parse_config(Config& config, int argc, const char** argv)
{
  using boost::program_options::options_description;
  using boost::program_options::value;
  using boost::program_options::variables_map;
  using boost::program_options::store;
  using boost::program_options::option;
  using boost::program_options::notify;
  using boost::program_options::parse_command_line;
  using boost::program_options::command_line_parser;
  using boost::program_options::positional_options_description;

  options_description desc("Commandline options");
  desc.add_options()
    ("help,h,?", "help")
    ("pid,p", value<pid_t>(), "PID of the target process")
    ("start-address", value<std::string>(), "Address")
    ("stop-address", value<std::string>(), "Address")
    ;
  variables_map vm;
  store(command_line_parser(argc, argv).options(desc).run(), vm);
  notify(vm);

  if (vm.count("help")) {
    std::cout << desc  << '\n';
    std::exit(0);
  }

  if (vm.count("pid"))
    config.pid = vm["pid"].as<pid_t>();
  if (vm.count("start-address"))
    config.start = parse_integer(vm["start-address"].as<std::string>().c_str());
  if (vm.count("stop-address"))
    config.stop = parse_integer(vm["stop-address"].as<std::string>().c_str());

  return 0;
}

int main(int argc, const char** argv)
{
  Config config;
  try {
    int res = parse_config(config, argc, argv);
    if (res)
      return res;
  }
  catch(boost::program_options::unknown_option) {
    std::cerr << "Unknown option\n";
    return 1;
  }
  if (config.pid < 0 ) {
    std::cerr << "Missing PID\n";
    return 1;
  }

  LLVMInitializeAllTargetInfos();
  LLVMInitializeAllTargetMCs();
  LLVMInitializeAllDisassemblers();
  LLVMInitializeNativeDisassembler();

  unjit::Process process(config.pid);
  process.load_vm_maps();
  process.load_modules();
  process.load_map_file();

  unjit::Disassembler disassembler(process);
  if (config.start == 0 && config.stop == 0) {
    for (auto const& k : process.jit_symbols())
      disassembler.disassemble(std::cout, k.second.value, k.second.size);
  } else {
    disassembler.disassemble(std::cout, config.start, config.stop - config.start);
  }

  return 0;
}
