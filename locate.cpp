#include "kgram-db.h"
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>

using namespace std;
const int K = 3;

string pattern;
KGramDB* db;
KGramDB::PathIterator* path_iter;

bool match(const string& path) {
  return path.find(pattern) != string::npos;
}
void filterPrint(format::Interval iv, const vector<KGramDB::IntervalList>& lists, size_t start ) {
  if (iv.end <= iv.begin) return;
  if (start == lists.size()) {
    path_iter->setID(iv.begin);
    for (format::ID id = iv.begin; id != iv.end; ++id) {
      if (match(path_iter->path())) {
        std::cout << path_iter->path() << "\n";
      }
      path_iter->next();
    }
    return;
  }
  const KGramDB::IntervalList& ivl = lists[start];
  auto range = lists[start].findRange(iv);
  for (size_t i = range.first; i < range.second; ++i) {
    format::Interval iv2 = lists[start][i];
    iv2.begin = std::max(iv2.begin, iv.begin);
    iv2.end = std::min(iv2.end, iv.end);
    filterPrint(iv2, lists, start + 1);
  }
}


void printMatching(const vector<KGramDB::IntervalList>& ivs) {
  filterPrint(format::Interval(0, db->numEntries()),
              ivs,
              0);
}

int main(int argc, char** argv) {
  KGramDB db("kgram.db");
  ::db = &db;
  KGramDB::PathIterator path_it(&db, KGramDB::ROOT);
  ::path_iter = &path_it;

  std::vector<std::string> patterns(argv + 1, argv + argc);
  if (patterns.size() == 0) {
    std::cerr << "usage: " << argv[0] << " PATTERN...\n";
  }

  for (string pat : patterns) {
    ::pattern = pat;
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

    printMatching(lists);
  }
  return 0;
}
