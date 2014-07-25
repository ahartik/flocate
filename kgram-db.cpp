#include "kgram-db.h"
#include <stdexcept>
#include <cassert>

#include <iostream>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <cstring>
#include <sys/mman.h>
#include <sys/stat.h>

#define ERROR_CHECK(x, msg) \
    if (!(x)) { \
      throw std::runtime_error(std::string(msg) + " : " + strerror(errno)); \
    }

KGramDB::KGramDB(const std::string& file) {
  fd_ = open(file.c_str(), O_RDONLY);
  ERROR_CHECK(fd_ >= 0, "Unable to open " + file);

  struct stat sb;
  fstat(fd_, &sb);
  map_size_ = sb.st_size;
  mapping_ = mmap(nullptr, map_size_, PROT_READ, MAP_SHARED, fd_, 0);
  ERROR_CHECK(mapping_ != MAP_FAILED, "Failed to mmap: ");
  memcpy(&header_, mapping_, sizeof(format::Header));
  entries_ = (format::Entry*)((char*)(mapping_) + sizeof(format::Header));
  kgrams_ = (format::KGram*)((char*)mapping_ + header_.kgram_start);
  size_t dist = (header_.list_start - header_.kgram_start);
  kgrams_size_ = dist / sizeof(format::KGram);
  assert(dist % sizeof(format::KGram) == 0);
  lists_ = (format::Interval*)((char*)mapping_ + header_.list_start);
  names_ = ((char*)mapping_ + header_.name_start);
}

KGramDB::~KGramDB() {
  int e = munmap(mapping_, map_size_);
  ERROR_CHECK(e == 0, "munmap failed");
}

void KGramDB::getFullPath(format::ID id, std::string* out) const {
  if (id == ROOT) {
    out->push_back('/');
    return;
  } else {
    format::ID parent = getParent(id);
    const char* name = getName(id);
    getFullPath(parent, out);
    if (parent != ROOT) {
      out->push_back('/');
    }
    out->append(name);
  }
}

const char* KGramDB::getName(format::ID id) const {
  return names_ + entries_[id].name;
}

format::ID KGramDB::getParent(format::ID id) const {
  return entries_[id].parent;
}

KGramDB::IntervalList KGramDB::getList(uint64_t kgram) const {
  IntervalList ret;
  // binary search for right kgram
  uint64_t left = 0;
  uint64_t right = kgrams_size_;
  while (left != right) {
    uint64_t m = (left + right) / 2;
    if (kgrams_[m].kgram < kgram) {
      left = m + 1;
    } else {
      right = m;
    }
  }

  if (left == kgrams_size_ ||
      kgrams_[left].kgram != kgram) {
    ret.list = nullptr;
    ret.size = 0;
    ret.total = 0;
    std::cout << "kgram " << kgram << " not found \n";
    std::cout << "left = " << left << "\n";
    std::cout << "kgrams_size_ = " << kgrams_size_ << "\n";
    std::cout << kgrams_[left-1].kgram << " " << kgrams_[left].kgram << "\n";
  } else {
    format::KGram found = kgrams_[left];
    ret.list = lists_ + found.ls_start;
    ret.size = found.ls_len;
    ret.total = found.count;
  }
  return ret;
}
