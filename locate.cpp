#include "kgram-db.h"
#include <vector>
#include <string>
#include <iostream>

using namespace std;
const int K = 3;

bool match(const string& path, const string& pattern) {
  return path.find(pattern) != string::npos;
}

bool operator<(const format::KGram& a, const format::KGram& b) {
  return a.count < b.count;
}

void filter(vector<format::FileID>& vec, format::FileID* check, size_t check_len) {

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
      for (size_t i = 0; i < pattern.size()-K+1; ++i) {
        kgrams.push_back(pattern.substr(i, K));
      }
    }
    // remove kgrams with inner '/'
    for (size_t i = 0; i < kgrams.size(); ++i) {
      int slash_pos = -1;
      for (size_t j = 1; j < K-1; ++j) {
        if (kgrams[i][j] == '/') {
          slash_pos = j;
          break;
        }
      }
      if (slash_pos != -1) {
        kgrams[i] = kgrams.back();
        kgrams.pop_back();
        i--;
      }
    }
    std::vector<format::KGram> lists;
    for (string x : kgrams) {
      std::cout << x << "\n";
      uint64_t id = format::KGramToID(x.c_str(), x.size());
      lists.push_back(db.getKGram(id));
    }
  }
}
