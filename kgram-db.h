#pragma once
#include "db-format.h"
#include <string>
#include <unistd.h>

class KGramDB {
 public:

  KGramDB(const std::string& file);
  ~KGramDB();
  format::Dir getDir(format::FileID id) const;
  format::File getFile(format::FileID id) const;
  const char* getName(uint64_t start) const;
  format::FileID* getList(uint64_t start) const;
  format::KGram getKGram(uint64_t id) const;
  bool isDir(format::FileID) const;
  static const format::FileID ROOT = format::DIR_BIT;

  void getFullPath(format::FileID id, std::string* out) const;
 private:
  format::Header header_;
  int fd_;
  void* mapping_;
  size_t map_size_;
  format::Dir* dirs_;
  format::File* files_;
  format::FileID* lists_;
  format::KGram* kgrams_;
  const char* names_;
};
