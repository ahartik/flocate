#pragma once
#include "db-format.h"
#include <string>
#include <vector>
#include <utility>
#include <unistd.h>

class KGramDB {
 public:
  struct IntervalList {
   public:
    IntervalList(format::Interval* l, size_t size, size_t total) {
      list = l;
      size_ = size;
      total_ = total;
    }

    const format::Interval* begin() const {
      return list;
    }
    const format::Interval* end() const {
      return list + size_;
    }
    const format::Interval& operator[](size_t i) const {
      return list[i];
    }
    size_t size() const {
      return size_;
    }
    size_t total() const {
      return total_;
    }
    size_t find(format::ID id) const;
    std::pair<size_t,size_t> findRange(format::Interval iv) const;
   private:
    format::Interval* list;
    size_t size_;
    size_t total_;
  };

  class PathIterator {
    public:
     PathIterator(const KGramDB* db, format::ID id);
     void setID(format::ID id);
     void next();
     const std::string& path() const {
       return str;
     }
     format::ID id() const {
       return stack.back();
     }
     bool end() const {
       return end_;
     }
    private:
     void addDir(format::ID id);
     std::string str;
     std::vector<format::ID> stack;
     std::vector<size_t> parts;
     const KGramDB* db;
     bool end_;
  };

  KGramDB(const std::string& file);
  ~KGramDB();
  const char* getName(format::ID id) const;
  format::ID getParent(format::ID id) const;
  IntervalList getList(uint64_t kgram) const;
  format::ID numEntries() const;
  void getFullPath(format::ID id, std::string* out) const;

  PathIterator pathIterator() const {
    return PathIterator(this, ROOT);
  }
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
