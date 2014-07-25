#include "kgram-db.h"

using namespace std;

void listAll(KGramDB& db, string kgram) {
  uint64_t id = format::KGramToID(kgram.c_str(), kgram.size());
  KGramDB::IntervalList list = db.getList(id);
  std::string name;
  for (size_t i = 0; i < list.size; ++i) {
    for (format::ID f = list.list[i].begin;
         f != list.list[i].end;
         ++f) {
      db.getFullPath(f, &name);
      std::cout << name << "\n";
      name.clear();
    }
  }
}

int main() {
  KGramDB db("kgram.db");
  listAll(db, "aba");
}
