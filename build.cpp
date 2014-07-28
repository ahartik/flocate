// Copyright 2014 Aleksi Hartikainen.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.
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
#include <map>

#include <sys/stat.h>
#include <unistd.h>
#include "db-format.h"

using namespace std;

#define ERROR_CHECK(x, msg) \
    if (!(x)) { \
      throw runtime_error(string(msg) + " : " + strerror(errno)); \
    }

const int K = 3;

vector<char> names;

std::vector<format::Interval> lists;
std::vector<format::Entry> entries;
std::vector<format::KGram> kgrams;

struct KGram {
  KGram() {
    count = 0;
  }
  vector<format::Interval> list; 
  uint64_t count;
};

std::map<uint64_t, KGram> kgram_map;

uint64_t addName(const char* n) {
  uint64_t name_pos = names.size();
  const char* c = n;
  do {
    names.push_back(*c);
  } while (*c++);
  return name_pos;
}

void addKGrams(const char* name, format::Interval iv) {
  int count = iv.end - iv.begin;
  for (int i = 0; name[i] != 0; ++i) {
    for (int j = 1; j <= K; ++j) {
      uint64_t kid = format::KGramToID(name + i, j);
      KGram& kgram = kgram_map[kid];
      kgram.list.push_back(iv);
      kgram.count += count;
      if (name[i + j] == 0) break;
    }
  }
}

uint64_t addFile(const char* name, format::ID parent) {
  format::Entry f;
  f.name = addName(name);
  f.parent = parent;
  entries.push_back(f);
  uint64_t id = entries.size() - 1;
  char slashed[257] = "/";
  strncpy(slashed + 1, name, 256);
  addKGrams(slashed, format::Interval(id, id+1));
  return id;
}

const format::ID DIR_FAILED = format::ID(-1);

format::ID addDir(string& path, const char* name, format::ID parent) {
  DIR* dir = opendir(path.c_str());
  if (dir == nullptr) {
    std::cerr << "warning: Couldn't open directory " << path
              << " : " << strerror(errno) << "\n";
    return DIR_FAILED;
  }
  uint64_t id = entries.size();
  entries.emplace_back();
  entries[id].name = addName(name);
  entries[id].parent = parent;

  // sort entries
  vector<string> ls;
  {
    struct dirent* ent;
    while ((ent = readdir(dir))) {
      if (strcmp(ent->d_name, ".") == 0 ||
          strcmp(ent->d_name, "..") == 0) {
        continue;
      }
      ls.push_back(ent->d_name);
    }
  }
  std::sort(ls.begin(), ls.end());
  
  for (const std::string& ent : ls) {
    struct stat sb;
    size_t s = path.size();
    path.append(ent);
    lstat(path.c_str(), &sb);
    if (S_ISDIR(sb.st_mode)) {
      path.push_back('/');
      addDir(path, ent.c_str(), id);
    } else {
      addFile(ent.c_str(), id);
    }
    path.resize(s);
  }
  closedir(dir);

  char slashed[260];
  snprintf(slashed, 257, "/%s", name);
  addKGrams(slashed,format::Interval(id, entries.size()));
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

void mergeIntervals(std::vector<format::Interval>& list) {
  std::sort(list.begin(), list.end());
  size_t shift = 0;
  size_t last = 0;
  for (size_t i = 1; i < list.size(); ++i) {
    if (list[i].begin <= list[last].end) {
      list[last].end = std::max(list[i].end, list[last].end);
      // this value is skipped
      shift++;
    } else {
      list[i - shift] = list[i];
      last = i - shift;
    }
  }
  list.resize(list.size() - shift);
}

int main() {
  std::string path = "/";
  format::ID root_id = addDir(path, "", -1);
  assert(root_id == 0);

  // Add kgrams
  for (auto kgramp : kgram_map) {
    KGram& kgram = kgramp.second;
    mergeIntervals(kgram.list);
    format::KGram k;
    k.kgram = kgramp.first;
    k.ls_start = lists.size();
    k.ls_len = kgram.list.size();
    k.count = kgram.count;
    lists.insert(lists.end(), kgram.list.begin(), kgram.list.end());
    kgrams.push_back(k);
  }
  
  std::ofstream out("kgram.db");
  format::Header head;
  head.k = K;
  head.kgram_start = sizeof(format::Header) + entries.size() * sizeof(format::Entry);
  head.list_start = head.kgram_start + kgrams.size() * sizeof(format::KGram);
  head.name_start = head.list_start + sizeof(format::Interval) * lists.size();

  writeBytes(out, head);

  writeVec(out, entries);
  assert(uint64_t(out.tellp()) == head.kgram_start);
  writeVec(out, kgrams);
  assert(uint64_t(out.tellp()) == head.list_start);
  writeVec(out, lists);
  assert(uint64_t(out.tellp()) == head.name_start);
  writeVec(out, names);

  std::cout << "names.size() = " << names.size() << "\n";
}
