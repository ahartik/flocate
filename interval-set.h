// Copyright 2014 Aleksi Hartikainen.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.
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

// Convenience:
namespace {
IntervalSetPtr toIntervalSet(KGramDB::IntervalList list) {
  return std::make_shared<IntervalSetList>(list);
}

IntervalSetPtr andIntervalSet(const IntervalSetVector& vec) {
  return std::make_shared<IntervalSetAnd>(vec);
}

IntervalSetPtr orIntervalSet(const IntervalSetVector& vec) {
  return std::make_shared<IntervalSetOr>(vec);
}
}
