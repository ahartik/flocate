#include "kgram-db.h"
#include <stdexcept>
#include <cassert>
#include <algorithm>

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

static bool rangeCmp(format::Interval a, format::Interval b) {
  return a.end <= b.begin;
}

size_t KGramDB::IntervalList::find(format::ID id) const {
  auto it = std::lower_bound(list, list + size_, format::Interval(id, id + 1), rangeCmp);
  if (it == list + size_) return size_;
  if (it->begin > id || it->end <= id) return size_;
  return it - list;
}

std::pair<size_t,size_t> KGramDB::IntervalList::findRange(format::Interval iv) const {
  auto p = std::equal_range(list, list + size(), iv, &rangeCmp);
  return std::pair<size_t, size_t>(p.first - list, p.second - list);
}

KGramDB::PathIterator::PathIterator(const KGramDB* db, format::ID id) {
  this->db = db;
  setID(id);
}

void KGramDB::PathIterator::addDir(format::ID id) {
  if (id == ROOT) {
    str.push_back('/');
  } else {
    format::ID parent = db->getParent(id);
    const char* name = db->getName(id);
    addDir(parent);
    if (parent != ROOT) {
      str.push_back('/');
    }
    str.append(name);
  }
  stack.push_back(id);
  parts.push_back(str.size());
}

void KGramDB::PathIterator::setID(format::ID id) {
  str.clear();
  stack.clear();
  parts.clear();
  addDir(id);
}

void KGramDB::PathIterator::next() {
  format::ID last = stack.back();
  format::ID next = last + 1;
  if (db->numEntries() <= next) {
    str.clear();
    stack.clear();
    parts.clear();
    stack.push_back(next);
    end_ = true;
    return;
  }
  format::ID p = db->getParent(next);
  const char* name = db->getName(next);
  while (p != stack.back()) {
    stack.pop_back();
    parts.pop_back();
    str.resize(parts.back());
  }

  if (stack.back() != ROOT) {
    str.push_back('/');
  }
  str.append(name);
  stack.push_back(next);
  parts.push_back(str.size());
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

format::ID KGramDB::numEntries() const {
  size_t ent_start = sizeof(header_);
  size_t dist = (header_.kgram_start - ent_start);
  assert(dist % sizeof(format::Entry) == 0);
  return dist / sizeof(format::Entry);
}


KGramDB::IntervalList KGramDB::getList(uint64_t kgram) const {
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
    std::cout << "kgram " << kgram << " not found \n";
    std::cout << "left = " << left << "\n";
    std::cout << kgrams_[left-1].kgram << " " << kgrams_[left].kgram << "\n";
    return IntervalList(0,0,0);
  } else {
    format::KGram found = kgrams_[left];
    return IntervalList(lists_ + found.ls_start,
                        found.ls_len,
                        found.count);
  }
}
