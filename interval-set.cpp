#include "interval-set.h"

#include <memory>
#include <cassert>
#include <vector>
#include <algorithm>

static bool rangeCmp(format::Interval a, format::Interval b) {
  return a.end <= b.begin;
}

class IntervalSetListIterator : public IntervalSetIterator {
 public:
  IntervalSetListIterator(KGramDB::IntervalList l) : list(l), pos(0) {}
  virtual bool next() {
    if (end()) return false;
    pos++;
    return !end();
  }

  virtual void seek(format::ID id) {
    auto it = std::lower_bound(list.begin(), list.end(), format::Interval(id, id + 1), rangeCmp);
    pos = it - list.begin();
  }

  virtual format::Interval get() const {
    return list[pos];
  }

  virtual bool end() const {
    return pos >= list.size();
  }

 private:
  KGramDB::IntervalList list;
  size_t pos;
};

IntervalSetList::IntervalSetList(KGramDB::IntervalList list) : list_(list) {}

std::unique_ptr<IntervalSetIterator> IntervalSetList::iterator() const {
  return std::unique_ptr<IntervalSetIterator>(new IntervalSetListIterator(list_));
}

size_t IntervalSetList::approx() const {
  return list_.total();
}
IntervalSetList::~IntervalSetList() {

}

class IntervalSetAndIterator : public IntervalSetIterator {
 public:
  IntervalSetAndIterator(const IntervalSetVector& v) : vec(v) {
    for (const auto& s : vec) {
      iters.emplace_back(s->iterator());
    }
    stack.push_back(format::Interval(0, format::ID_MAX));
    get_next();
  }

  virtual bool next() {
    if (end()) return false;
    stack.pop_back();
    get_next();
    return !end();
  }

  virtual void seek(format::ID id) {
    assert(false && "Not implemented");
  }

  virtual format::Interval get() const {
    return stack.back();
  }

  virtual bool end() const {
    return stack.empty();
    // return iters[0]->end();
  }

 private:
  void get_next() {
    while (!end()) {
      size_t start = stack.size() - 1;
      if (start == vec.size()) return;
      format::Interval iv = stack.back();
      IntervalSetIterator* iter = iters[start].get();
      if (iter->end()) {
        stack.pop_back();
        continue;
      }
      if (iter->get().begin >= iv.end) {
        stack.pop_back();
        continue;
      } else {
        format::Interval niv = iter->get();
        niv.begin = std::max(iv.begin, niv.begin);
        niv.end = std::min(iv.end, niv.end);
        stack.push_back(niv);
        iter->next();
        if (start + 1 < vec.size()) {
          iters[start + 1]->seek(niv.begin);
        }
      }
    }
  }
  
  IntervalSetVector vec;
  std::vector<format::Interval> stack;
  std::vector<std::unique_ptr<IntervalSetIterator>> iters;
};

IntervalSetAnd::IntervalSetAnd(const IntervalSetVector& v) : sets_(v) {
  static auto cmp = [](const IntervalSetPtr& a, const IntervalSetPtr& b) {
    return a->approx() < b->approx();
  };
  std::sort(sets_.begin(), sets_.end(), cmp);
  if (v.empty()) {
    std::cerr << "warning: empty IntervalSetAnd\n";
    approx_ = 0;
  } else {
    approx_ = sets_[0]->approx();
  }
}

std::unique_ptr<IntervalSetIterator> IntervalSetAnd::iterator() const {
  return std::unique_ptr<IntervalSetIterator>(new IntervalSetAndIterator(sets_));
}

size_t IntervalSetAnd::approx() const {
  return approx_;
}

IntervalSetAnd::~IntervalSetAnd() { }

class IntervalSetOrIterator : public IntervalSetIterator {
 public:
  IntervalSetOrIterator(const IntervalSetVector& v) : vec(v) {
    for (const auto& s : vec) {
      iters.emplace_back(s->iterator());
    }
  }

  virtual bool next() {
    return !end();
  }

  virtual void seek(format::ID id) {
    assert(false && "Not implemented");
  }

  virtual format::Interval get() const {
    assert(false && "Not implemented");
    return format::Interval(0,0);
  }

  virtual bool end() const {
    return true;
  }

 private:

  IntervalSetVector vec;
  std::vector<std::unique_ptr<IntervalSetIterator>> iters;
};

IntervalSetOr::IntervalSetOr(const IntervalSetVector& v) : sets_(v) {
  static auto cmp = [](const IntervalSetPtr& a, const IntervalSetPtr& b) {
    return a->approx() < b->approx();
  };
  std::sort(sets_.begin(), sets_.end(), cmp);
  if (v.empty()) {
    std::cerr << "warning: empty IntervalSetOr\n";
    approx_ = 0;
  } else {
    approx_ = sets_[0]->approx();
  }
}

std::unique_ptr<IntervalSetIterator> IntervalSetOr::iterator() const {
  return std::unique_ptr<IntervalSetIterator>(new IntervalSetOrIterator(sets_));
}

size_t IntervalSetOr::approx() const {
  return approx_;
}

IntervalSetOr::~IntervalSetOr() { }
