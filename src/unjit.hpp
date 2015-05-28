#ifndef UNJIT_UNJIT_HTPP
#define UNJIT_UNJIT_HTPP

#include <cstdio>

#include <memory>

class fcloser {
public:
  void operator()(std::FILE* f) const
  {
    fclose(f);
  }
};

typedef std::unique_ptr<std::FILE, fcloser> unique_file;

struct symbol {
  std::uint64_t start;
  std::uint64_t size;
  std::string name;
};

struct DisContext {
  pid_t pid;
  std::unordered_map<std::uint64_t, symbol> jit_symbols;
};

#endif
