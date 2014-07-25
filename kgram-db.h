#pragma once
#include "db-format.h"
#include <string>
#include <unistd.h>

class KGramDB {
 public:
  struct IntervalList {
    format::Interval* list;
    size_t size;
    size_t total;
  };

  KGramDB(const std::string& file);
  ~KGramDB();
  const char* getName(format::ID id) const;
  format::ID getParent(format::ID id) const;
  IntervalList getList(uint64_t kgram) const;
  void getFullPath(format::ID id, std::string* out) const;

  static const format::ID ROOT = 0;
 private:
  format::Header header_;
  int fd_;
  void* mapping_;
  size_t map_size_;
  format::Entry* entries_;
  format::KGram* kgrams_;
  size_t kgrams_size_;
  format::Interval* lists_;
  const char* names_;
};
