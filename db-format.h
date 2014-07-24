#pragma once

#include <stdint.h>

#include <iostream>

namespace format {

typedef uint32_t FileID;

struct Header {
  uint64_t k;
  uint64_t num_dirs;
  uint64_t list_start;
  uint64_t kgram_start;
  uint64_t name_start;
};

static const FileID DIR_BIT = 1ull << (8 * sizeof(FileID) - 1);
static const uint64_t SHORT_NAME_BIT = 1ull << 63ull;

struct Dir {
  uint64_t name;
  FileID parent;
  uint64_t ls_id;
  uint64_t ls_len;
};

struct File {
  uint64_t name;
  FileID parent;
};

static inline uint64_t KGramToID(const char* kgram, int k) {
  uint64_t x = k;
  for (int i = 0; i < k; ++i) {
    if (kgram[i] == 0) {
      std::cerr << "warn: kgram too short for " << k << "\n";
      break;
    }
    x <<= 8ull;
    x |= (uint8_t)kgram[i];
  }
  return x;
}

struct KGram {
  uint64_t kgram;
  uint64_t count;
  uint64_t ls_id;
  uint64_t ls_len;
};

}
