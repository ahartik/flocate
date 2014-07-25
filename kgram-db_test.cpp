#include "kgram-db.h"

#include <cassert>

using namespace std;

void listAll(KGramDB& db, string kgram) {
  uint64_t id = format::KGramToID(kgram.c_str(), kgram.size());
  KGramDB::IntervalList list = db.getList(id);
  std::string name;
  KGramDB::PathIterator it = db.pathIterator();
  for (size_t i = 0; i < list.size(); ++i) {
    it.setID(list[i].begin);
    for (format::ID f = list[i].begin;
         f != list[i].end;
         ++f) {
      size_t ivi = list.find(f);
      assert(ivi == i);
      auto p = list.findRange(format::Interval(f, f + 1));
      assert (p.first == i);
      assert (p.second == i+1);
      db.getFullPath(f, &name);
      std::cout << name << "\n";

      // Check that path_iterator corresponds to getFullPath
      assert (name == it.path());

      name.clear();
      it.next();
    }
  }
}

int main() {
  KGramDB db("kgram.db");
  listAll(db, "aba");
}
