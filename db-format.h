// Copyright 2014 Aleksi Hartikainen.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.
#pragma once

#include <stdint.h>

#include <climits>
#include <iostream>

namespace format {

typedef uint32_t ID;

struct Header {
  uint64_t k;
  uint64_t kgram_start;
  uint64_t list_start;
  uint64_t name_start;
};

static const uint64_t SHORT_NAME_BIT = 1ull << 63ull;

struct Entry {
  uint64_t name;
  ID parent;
};

static const ID ID_MAX = UINT_MAX;

struct Interval {
  Interval(ID b, ID e) : begin(b), end(e) {}
  Interval() {}
  ID begin;
  ID end;
};

static inline bool operator<(const Interval& a, const Interval& b) {
  if (a.begin == b.begin)
    return a.end < b.end;
  return a.begin < b.begin;
}

static inline uint64_t KGramToID(const char* kgram, int k) {
  uint64_t x = k;
  for (int i = 0; i < k; ++i) {
    if (kgram[i] == 0) {
      std::cerr << "warn: kgram too short for " << k << "\n";
      // assert(false);
      break;
    }
    x <<= 8ull;
    x |= (uint8_t)kgram[i];
  }
  return x;
}

struct KGram {
  KGram() {
    kgram = 0;
    ls_start = 0;
    count = 0;
    ls_len =0;
  }
  uint64_t kgram;
  uint64_t ls_start;
  uint32_t count;
  uint32_t ls_len;
};

}
