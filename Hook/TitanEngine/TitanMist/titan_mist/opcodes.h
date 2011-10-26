#pragma once
#include "types.h"

namespace mist {

  enum opcode_t {
    OP_MATCH_BYTE,
    OP_MATCH_BITMASK1,
    OP_MATCH_BITMASK2,
    OP_SKIP,
    OP_SKIP_NEG,
    OP_IGNORE,
    OP_MATCH_RANGE,
    OP_MATCH_STRING,
    OP_OPTIONAL_START,
    OP_OPTIONAL_END,
    OP_JUMP,
    OP_FOLLOW_DELTA,
    OP_FOLLOW_DELTA_NEG,
    OP_EAT_BYTES,
    OP_SEEK_RANGE_START,
    OP_SEEK_RANGE_NEG_START,
    OP_SEEK_RANGE_END
  };

  struct instr_t {
    opcode_t op;
    union {
      uint8_t arg8;
      uint32_t arg32;
    } args;
  };

} // end namespace mist
