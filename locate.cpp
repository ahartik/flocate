#include "kgram-db.h"
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>

using namespace std;
const int K = 3;

bool match(const string& path, const string& pattern) {
  return path.find(pattern) != string::npos;
}


void printMatching(const KGramDB& db,
                   const vector<format::Interval>& vec,
                   const std::string& pattern) {
  std::string path;
  for (format::Interval iv : vec) {
    for (format::ID id = iv.begin; id != iv.end; ++id) {
      path.clear();
      db.getFullPath(id, &path);
      if (match(path, pattern)) {
        std::cout << path<< "\n";
      }
    }
  }
}

int main(int argc, char** argv) {
  KGramDB db("kgram.db");

  std::vector<std::string> patterns(argv + 1, argv + argc);
  if (patterns.size() == 0) {
    std::cerr << "usage: " << argv[0] << " PATTERN...\n";
  }

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
      if (lists.back().total == 0) {
        empty_result = true;
        break;
      }
    }

    if (empty_result) {
      continue;
    }

    auto cmp = [](const KGramDB::IntervalList& a,
                  const KGramDB::IntervalList& b) {
      return a.total < b.total;
    };

    std::sort(lists.begin(), lists.end(), cmp);
    vector<format::Interval> filtered(lists[0].list,
                                      lists[0].list + lists[0].size);
    printMatching(db, filtered, pattern);
  }
}
