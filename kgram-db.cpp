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
  dirs_ = (format::Dir*)((char*)(mapping_) + sizeof(format::Header));
  files_ = (format::File*)((char*)(mapping_) + sizeof(format::Header) + header_.num_dirs * sizeof(format::Dir));
  lists_ = (format::FileID*)((char*)mapping_ + header_.list_start);
  kgrams_ = (format::KGram*)((char*)mapping_ + header_.kgram_start);
  names_ = ((char*)mapping_ + header_.name_start);
  std::cout << "num_dirs = " << header_.num_dirs << "\n";
}

KGramDB::~KGramDB() {
  int e = munmap(mapping_, map_size_);
  ERROR_CHECK(e == 0, "munmap failed");
}

void KGramDB::getFullPath(format::FileID id, std::string* out) const {
  if (id == ROOT) {
    out->push_back('/');
    return;
  } else {
    format::FileID parent;
    const char* name;
    if (isDir(id)) {
      format::Dir d = getDir(id);
      name = getName(d.name);
      parent = d.parent;
    } else {
      format::File f = getFile(id);
      parent = f.parent;
      name = getName(f.name);
    }
    getFullPath(parent, out);
    out->append(name);
    if (isDir(id)) {
      out->push_back('/');
    }
  }
}


format::Dir KGramDB::getDir(format::FileID id) const {
  assert((id & format::DIR_BIT) != 0);
  id ^= format::DIR_BIT;
  uint64_t offset = sizeof(format::Header);
  offset += id * sizeof(format::Dir);
  format::Dir ret;
  memcpy(&ret, (char*)(mapping_) + offset, sizeof(format::Dir));
  return ret;
}

format::File KGramDB::getFile(format::FileID id) const {
  uint64_t offset = sizeof(format::Header) +
      header_.num_dirs * sizeof(format::Dir);
  offset += id * sizeof(format::File);
  format::File ret;
  memcpy(&ret, (char*)(mapping_) + offset, sizeof(format::File));
  return ret;
}

const char* KGramDB::getName(uint64_t id) const {
  char* start = (char*)(mapping_) + header_.name_start;
  return start + id;
}

format::KGram KGramDB::getKGram(uint64_t id) const {
  char* start = (char*)(mapping_) + header_.kgram_start;
  format::KGram ret;
  memcpy(&ret, start, sizeof(format::KGram));
  return ret;
}

format::FileID* KGramDB::getList(uint64_t id) const {
  char* start = (char*)(mapping_) + header_.list_start;
  // Check alignment
  assert((intptr_t(start) & 7) == 0);
  format::FileID* p = (format::FileID*)start;
  return p + id;
}

bool KGramDB::isDir(format::FileID id) const {
  return (id & format::DIR_BIT) != 0;
}
