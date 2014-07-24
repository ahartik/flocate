#include <vector>
#include <string>
#include <algorithm>

#include <dirent.h>
#include <iostream>
#include <cassert>
#include <stdexcept>
#include <fstream>

#include <fcntl.h>
#include <cstring>
#include <unordered_map>

#include "db-format.h"

using namespace std;

#define ERROR_CHECK(x, msg) \
    if (!(x)) { \
      throw runtime_error(string(msg) + " : " + strerror(errno)); \
    }

const int K = 3;

vector<char> names;

std::vector<format::FileID> lists;
std::vector<format::Dir> dirs;
std::vector<format::File> files;
std::vector<format::KGram> kgrams;

struct KGram {
  KGram() {
    count = 0;
  }
  vector<format::FileID> list; 
  uint64_t count;
};

std::unordered_map<uint64_t, KGram> kgram_map;

uint64_t addName(const char* n) {
  uint64_t name_pos = names.size();
  const char* c = n;
  do {
    names.push_back(*c);
  } while (*c++);
  return name_pos;
}

void addKGrams(const char* name, uint64_t id, uint64_t count) {
  for (int i = 0; name[i] != 0; ++i) {
    for (int j = 1; j < K; ++j) {
      uint64_t kid = format::KGramToID(name + i, j);
      kgram_map[kid].list.push_back(id);
      kgram_map[kid].count += count;
      if (name[i + j] == 0) break;
    }
  }
}

uint64_t addFile(const char* name, format::FileID parent) {
  format::File f;
  f.name = addName(name);
  f.parent = parent;
  files.push_back(f);
  uint64_t id = files.size() - 1;
  char slashed[257] = "/";
  strncpy(slashed + 1, name, 256);
  addKGrams(slashed, id, 1);
  return id;
}

const format::FileID DIR_FAILED = format::FileID(-1);

format::FileID addDir(string& path, const char* name, format::FileID parent) {
  DIR* dir = opendir(path.c_str());
  if (dir == nullptr) {
    std::cerr << "warning: Couldn't open directory " << path
              << " : " << strerror(errno) << "\n";
    return DIR_FAILED;
  }
  uint64_t id = dirs.size();
  size_t idx = id;
  id |= format::DIR_BIT;
  dirs.push_back(format::Dir());
  dirs[idx].name = addName(name);
  dirs[idx].parent = parent;
  struct dirent* ent;
  std::vector<uint64_t> list;

  int file_count_start = dirs.size() + files.size();
  while ((ent = readdir(dir))) {
    if (strcmp(ent->d_name, ".") == 0 ||
        strcmp(ent->d_name, "..") == 0) {
      continue;
    }
    if (ent->d_type == DT_DIR) {
      size_t s = path.size();
      path.append(ent->d_name);
      path.push_back('/');
      uint64_t a = addDir(path, ent->d_name, id);
      path.resize(s);
      if (a != DIR_FAILED) {
        list.push_back(a);
      }
    } else {
      uint64_t a = addFile(ent->d_name, id);
      list.push_back(a);
    }
  }
  int file_count_end = dirs.size() + files.size();
  closedir(dir);

  char slashed[260];
  snprintf(slashed, 257, "/%s/", name);
  addKGrams(slashed, id, 1 + file_count_end - file_count_start);

  dirs[idx].ls_id = lists.size();
  dirs[idx].ls_len = list.size();
  lists.insert(lists.end(), list.begin(), list.end());
  return id;
}


template<typename T>
void writeBytes(std::ostream& out, const T& t) {
  out.write((const char*)(&t), sizeof(T));
}
template<typename T>
void writeVec(std::ostream& out, const std::vector<T>& vec) {
  out.write((const char*)(&vec[0]), sizeof(T) * vec.size());
}

int main() {
  std::string path = "/";
  format::FileID root_id = addDir(path, "", -1);
  assert(root_id == (0|format::DIR_BIT));

  // Add kgrams
  for (auto kgramp : kgram_map) {
    KGram& kgram = kgramp.second;
    format::KGram k;
    k.kgram = kgramp.first;
    k.ls_id = lists.size();
    k.ls_len = kgram.list.size();
    k.count = kgram.count;
    std::sort(kgram.list.begin(), kgram.list.end());
    kgram.list.erase(std::unique(kgram.list.begin(), kgram.list.end()), kgram.list.end());
    lists.insert(lists.end(), kgram.list.begin(), kgram.list.end());
    kgrams.push_back(k);
  }
  
  std::ofstream out("kgram.db");
  format::Header head;
  head.k = K;
  head.num_dirs = dirs.size();
  head.list_start = sizeof(format::Header) + dirs.size() * sizeof(format::Dir) +
      files.size() * sizeof(format::File);
  head.kgram_start = head.list_start + sizeof(format::FileID) * lists.size();
  head.name_start = head.kgram_start+ sizeof(format::KGram) * kgrams.size();

  writeBytes(out, head);

  writeVec(out, dirs);
  writeVec(out, files);
  assert(uint64_t(out.tellp()) == head.list_start);

  writeVec(out, lists);
  assert(uint64_t(out.tellp()) == head.kgram_start);
  writeVec(out, kgrams);
  assert(uint64_t(out.tellp()) == head.name_start);
  writeVec(out, names);

  std::cout << "names.size() = " << names.size() << "\n";
}
