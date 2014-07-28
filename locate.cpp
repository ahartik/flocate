// Copyright 2014 Aleksi Hartikainen.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.
#include "kgram-db.h"
#include "interval-set.h"

#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <cstring>

using namespace std;
const int K = 3;

KGramDB* db;
KGramDB::PathIterator* path_iter;

vector<string> patterns;

bool print_debug = false;

size_t filtered_count = 0;

bool match(const string& path) {
  for (const string& pattern : patterns) {
    if (strstr(path.c_str(), pattern.c_str()) != nullptr)
      return true;
  }
  return false;
}

void printMatching(IntervalSetPtr set) {
  std::unique_ptr<IntervalSetIterator> iter = set->iterator();

  for (; !iter->end(); iter->next()) {
    format::Interval iv = iter->get();
    path_iter->setID(iv.begin);
    filtered_count += iv.end - iv.begin;
    for (format::ID id = iv.begin; id != iv.end; ++id) {
      if (match(path_iter->path())) {
        std::cout << path_iter->path() << "\n";
      }
      path_iter->next();
    }
  }
}

int main(int argc, char** argv) {
  KGramDB db("kgram.db");
  ::db = &db;
  KGramDB::PathIterator path_it(&db, KGramDB::ROOT);
  ::path_iter = &path_it;

  patterns = vector<string>(argv + 1, argv + argc);
  if (patterns.size() == 0) {
    std::cerr << "usage: " << argv[0] << " PATTERN...\n";
  }
  // Build IntervalSetOr of patterns
  
  IntervalSetVector or_vec;
  for (string pattern : patterns) {
    std::vector<std::string> kgrams;
    if (pattern.size() <= K) {
      kgrams.push_back(pattern);
    } else {
      for (size_t i = 0; i < pattern.size() - K + 1; ++i) {
        kgrams.push_back(pattern.substr(i, K));
      }
    }
    // remove kgrams with non-starting '/'
    for (size_t i = 0; i < kgrams.size(); ++i) {
      int slash_pos = -1;
      for (size_t j = 1; j < kgrams[i].size(); ++j) {
        if (kgrams[i][j] == '/') {
          slash_pos = j;
          break;
        }
      }
      if (slash_pos != -1) {
        // split kgram in two
        string kg = kgrams[i];
        kgrams[i] = kgrams.back();
        kgrams.pop_back();
        // kgrams.push_back(kg.substr(0, slash_pos));

        // never add single slash
        if (slash_pos != int(kg.size()) - 1) {
          kgrams.push_back(kg.substr(slash_pos));
        }
        i--;
      }
    }
    bool empty_result = false;
    std::vector<KGramDB::IntervalList> lists;
    for (string x : kgrams) {
      uint64_t id = format::KGramToID(x.c_str(), x.size());
      lists.emplace_back(db.getList(id));
      if (lists.back().total() == 0) {
        empty_result = true;
        break;
      }
    }

    if (empty_result) {
      continue;
    }

   auto cmp = [](const KGramDB::IntervalList& a,
                  const KGramDB::IntervalList& b) {
      return a.total() < b.total();
    };

    std::sort(lists.begin(), lists.end(), cmp);
    IntervalSetVector and_vec(lists.size());
    std::transform(lists.begin(), lists.end(), and_vec.begin(), toIntervalSet);
    or_vec.emplace_back(andIntervalSet(and_vec));
  }
  IntervalSetPtr or_set = orIntervalSet(or_vec);
  if (or_vec.size() == 1)
    or_set = or_vec[0];
  printMatching(or_set);
  if (print_debug) {
    std::cerr << "filtered_count: " << filtered_count << "\n";
  }
  return 0;
}
