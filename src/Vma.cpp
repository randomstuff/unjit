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

#include <string>
#include <iomanip>

#include <sys/mman.h>

#include "unjit.hpp"

namespace unjit {

std::ostream& operator<<(std::ostream& stream, Vma const& vma)
{
  stream << std::setfill('0') << std::setw(16) << std::hex
    << vma.start << "-" << vma.end
    << " " << (vma.prot & PROT_READ ? 'r' : '-')
    << (vma.prot & PROT_WRITE ? 'w' : '-')
    << (vma.prot & PROT_EXEC ? 'x' : '-')
    << (vma.flags & MAP_PRIVATE ? 'p' : '-')
    << ' ' << vma.offset
    << ' ' << '-'
    << ' ' << '-'
    << ' ' << '-'
    << ' ' << vma.name
    << '\n' << std::setw(0) << std::dec;
  return stream;
}

}
