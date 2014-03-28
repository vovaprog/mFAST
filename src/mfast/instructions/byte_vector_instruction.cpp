// Copyright (c) 2013, 2014, Huang-Ming Huang,  Object Computing, Inc.
// All rights reserved.
//
// This file is part of mFAST.
//
//     mFAST is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     mFAST is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with mFast.  If not, see <http://www.gnu.org/licenses/>.

#include "byte_vector_instruction.h"

namespace mfast
{

  byte_vector_field_instruction*
  byte_vector_field_instruction::clone(arena_allocator& alloc) const
  {
    return new (alloc) byte_vector_field_instruction(*this);
  }

  std::ptrdiff_t
  byte_vector_field_instruction::hex2binary(const char* src, unsigned char* target)
  {
    unsigned char* dest = target;
    char c = 0;
    int shift_bits = 4;

    for (; *src != '\0'; ++src) {

      char tmp =0;

      if (*src >= '0' && *src <= '9') {
        tmp = (*src - '0');
      }
      else if (*src >= 'A' && *src <= 'F') {
        tmp = (*src - 'A') + '\x0A';
      }
      else if (*src >= 'a' && *src <= 'f') {
        tmp = (*src - 'a') + '\x0a';
      }
      else if (*src == ' ' || *src == '-')
        continue;
      else
        return -1;

      c |= (tmp << shift_bits);

      if (shift_bits == 0) {
        *dest++ = c;
        c = 0;
        shift_bits = 4;
      }
      else
        shift_bits = 0;
    }

    if (shift_bits == 0)
      return -1;

    return dest - target;
  }


  const byte_vector_field_instruction*
  byte_vector_field_instruction::default_instruction()
  {
    static const byte_vector_field_instruction inst(0,operator_none,presence_mandatory,0,"","",0, string_value_storage(), 0, "", "");
    return &inst;
  }


} /* mfast */