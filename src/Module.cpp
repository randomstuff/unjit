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

#include <memory>

#include <unistd.h>
#include <fcntl.h>

#include <libelf.h>

#include "unjit.hpp"

namespace unjit {

class elf_deleter {
public:
  void operator()(Elf* elf) const
  {
    elf_end(elf);
  }
};

static Elf_Scn *elf_scn(Elf *elf, Elf64_Word sh_type)
{
  for (Elf_Scn *scn = elf_getscn(elf, 0); scn; scn = elf_nextscn(elf, scn)) {
    Elf64_Shdr *shdr64 = elf64_getshdr(scn);
    if (shdr64 && shdr64->sh_type == sh_type)
      return scn;
    Elf32_Shdr *shdr32 = elf32_getshdr(scn);
    if (shdr32 && shdr32->sh_type == sh_type)
      return scn;
  }
  return nullptr;
}

static Elf_Scn *elf_scn_symbol(Elf *elf)
{
  Elf_Scn *scn = elf_scn(elf, SHT_SYMTAB);
  if (scn)
    return scn;
  else
    return elf_scn(elf, SHT_DYNSYM);
}

static Elf64_Half elf_e_type(Elf *elf)
{
  Elf32_Ehdr* ehdr32 = elf32_getehdr(elf);
  Elf64_Ehdr* ehdr64 = elf64_getehdr(elf);
  if (ehdr32)
    return ehdr32->e_type;
  else if (ehdr64)
    return ehdr64->e_type;
  else
    return ET_NONE;
}

Module load_module(std::uint64_t start, std::string const& name)
{
  Module module;

  // Init libelf:
  if (elf_version(EV_CURRENT) == EV_NONE) {
    std::cerr << "Elf version error\n";
    return std::move(module);
  }

  // Open the file:
  FileDescriptor fd(open(name.c_str(), O_RDONLY));
  if (fd < 0) {
    std::cerr << "Could not open file " << name << "\n";
    return std::move(module);
  }
  std::unique_ptr<Elf, elf_deleter> elf(elf_begin(fd, ELF_C_READ, nullptr));
  if (!elf)
    return std::move(module);

  // Check if it is a suitable ELF file:
  if (elf_kind(elf.get()) != ELF_K_ELF)
    return std::move(module);

  Elf64_Half e_type = elf_e_type(elf.get());
  std::uint64_t offset;
  switch(e_type) {
  case ET_EXEC:
    offset = 0;
    break;
  case ET_DYN:
    offset = start;
    break;
  default:
    return std::move(module);
  }

  // Find SHT_SYMTAB ot SHT_DYNSYM (symbol table):
  Elf_Scn *symbol_scn = elf_scn_symbol(elf.get());
  if (!symbol_scn)
    return std::move(module);

  Elf32_Shdr *shdr32 = elf32_getshdr(symbol_scn);
  Elf64_Shdr *shdr64 = elf64_getshdr(symbol_scn);
  if (!shdr32 && !shdr64)
    return std::move(module);
  Elf64_Word sh_link = shdr64 ? shdr64->sh_link : shdr32->sh_link;
  Elf64_Xword sh_entsize = shdr64 ? shdr64->sh_entsize : shdr32->sh_entsize;
  Elf64_Xword sh_size = shdr64 ? shdr64->sh_size : shdr32->sh_size;

  uint64_t sh_entry_count = sh_size / sh_entsize;
  if (!sh_entsize)
    return std::move(module);

  module.name = name;

  // For each element in the symbol table (we skip the first element with
  // is always a NULL entry):
  Elf_Data *data = elf_getdata(symbol_scn, NULL);
  for (uint64_t i = 1; i != sh_entry_count; ++i) {
    Elf64_Sym *sym64 = shdr64 ? (Elf64_Sym*) data->d_buf + i : nullptr;
    Elf32_Sym *sym32 = shdr64 ? nullptr : (Elf32_Sym*) data->d_buf + i;

    Elf64_Word st_name = shdr64 ? sym64->st_name : sym32->st_name;
    if (!st_name)
      continue;
    Elf64_Section st_shndx = shdr64 ? sym64->st_shndx : sym32->st_shndx;
    Elf64_Addr st_value = shdr64 ? sym64->st_value : sym32->st_value;
    Elf64_Xword st_size = shdr64 ? sym64->st_size : sym32->st_size;
    unsigned char st_type = shdr64 ? ELF64_ST_TYPE(sym64->st_info) : ELF32_ST_TYPE(sym32->st_info);
    if (st_shndx == SHN_UNDEF || st_shndx == SHN_ABS)
      continue;
    char *symbol_name = elf_strptr(elf.get(), sh_link, st_name);
    if (!symbol_name)
      continue;

    Symbol symbol;
    symbol.value = st_value + offset;
    symbol.size = st_size;
    symbol.name = std::string(symbol_name);
    if (st_type == STT_FUNC)
      symbol.flags |= SYMBOL_FLAG_CODE;

    module.symbols[symbol.value] = std::move(symbol);
  }

  return std::move(module);
}

}
