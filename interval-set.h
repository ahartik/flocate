#pragma once

#include "kgram-db.h"
#include <memory>
#include <vector>

class IntervalSetIterator {
 public:
  virtual bool next() = 0;
  virtual void seek(format::ID id) = 0;

  virtual bool end() const = 0;
  virtual format::Interval get() const = 0;

  virtual ~IntervalSetIterator() { }
};

class IntervalSet {
 public:
  virtual std::unique_ptr<IntervalSetIterator> iterator() const = 0;
  virtual size_t approx() const = 0;
  virtual ~IntervalSet() {}
}; 


class IntervalSetList : public IntervalSet {
 public:
  IntervalSetList(KGramDB::IntervalList list);
  virtual std::unique_ptr<IntervalSetIterator> iterator() const;
  virtual size_t approx() const;
  virtual ~IntervalSetList();
 private:
  KGramDB::IntervalList list_;
};

typedef std::shared_ptr<IntervalSet> IntervalSetPtr;
typedef std::vector<IntervalSetPtr> IntervalSetVector;

class IntervalSetAnd : public IntervalSet {
 public:
  IntervalSetAnd(const IntervalSetVector& sets);
  virtual std::unique_ptr<IntervalSetIterator> iterator() const;
  virtual size_t approx() const;
  virtual ~IntervalSetAnd();
 private:
  IntervalSetVector sets_;
  size_t approx_;
};

class IntervalSetOr : public IntervalSet {
 public:
  IntervalSetOr(const IntervalSetVector& sets);
  virtual std::unique_ptr<IntervalSetIterator> iterator() const;
  virtual size_t approx() const;
  virtual ~IntervalSetOr();
 private:
  IntervalSetVector sets_;
  size_t approx_;
};
