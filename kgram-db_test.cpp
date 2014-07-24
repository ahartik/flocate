#include "kgram-db.h"

std::string tmp;
void listAll(KGramDB& db, format::FileID id, int depth) {
  tmp.clear();
  db.getFullPath(id, &tmp);
  std::cout << tmp << "\n";
  if (depth == 0) return;
  if (db.isDir(id)) {
    format::Dir d = db.getDir(id);
    format::FileID* list = db.getList(d.ls_id);
    for (unsigned i = 0; i < d.ls_len; ++i) {
      listAll(db, list[i], depth -1);
    }
  }
}

int main() {
  KGramDB db("kgram.db");
  listAll(db, KGramDB::ROOT, 3);
}
