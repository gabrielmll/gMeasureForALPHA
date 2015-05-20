// Copyright 2007,2008,2009,2010,2011,2012,2013,2014 Loïc Cerf (lcerf@dcc.ufmg.br)

// This file is part of multidupehack.

// multidupehack is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License version 3 as published by the Free Software Foundation

// multidupehack is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along with multidupehack; if not, write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

#ifndef VALUE_H_
#define VALUE_H_

#include "../../Parameters.h"

#include <vector>

using namespace std;

class Value
{
 public:
  Value(const unsigned int dataId);
  Value(const unsigned int dataId, const unsigned int presentAndPotentialNoise, const vector<unsigned int>::const_iterator nbOfValuesPerAttributeBegin, const vector<unsigned int>::const_iterator nbOfValuesPerAttributeEnd, const vector<unsigned int>& noisesInIntersections);
  Value(const Value& otherValue, const unsigned int intersectionId, const vector<unsigned int>::const_iterator sizeOfAttributeIt, const vector<unsigned int>::const_iterator sizeOfAttributeEnd);

  const bool operator<(const Value& otherValue) const;

  const unsigned int getIntersectionId() const;
  const unsigned int getDataId() const;
  const unsigned int getPresentNoise() const;
  const unsigned int getPresentAndPotentialNoise() const;
  void addPresentNoise(const unsigned int noise);
  void substractPotentialNoise(const unsigned int noise);
  vector<vector<unsigned int>>::iterator getIntersectionsBeginWithPresentValues();
  vector<vector<unsigned int>>::const_iterator getIntersectionsBeginWithPresentValues() const;
  vector<vector<unsigned int>>::iterator getIntersectionsBeginWithPresentAndPotentialValues();
  vector<vector<unsigned int>>::const_iterator getIntersectionsBeginWithPresentAndPotentialValues() const;

  const unsigned int getNoiseInPresentIntersectionWith(const unsigned int reverseIndex, const Value& otherValue) const;

  const bool extendsPastPresent(const vector<Value*>::const_iterator valueBegin, const vector<Value*>::const_iterator valueEnd, const unsigned int threshold, const unsigned int intersectionIndex) const;
  const bool extendsFuturePresent(const vector<Value*>::const_iterator valueBegin, const vector<Value*>::const_iterator valueEnd, const unsigned int threshold, const unsigned int reverseAttributeIndex) const;
  const bool extendsPastPresentAndPotential(const vector<Value*>::const_iterator valueBegin, const vector<Value*>::const_iterator valueEnd, const unsigned int threshold, const unsigned int intersectionIndex) const;
  const bool extendsFuturePresentAndPotential(const vector<Value*>::const_iterator valueBegin, const vector<Value*>::const_iterator valueEnd, const unsigned int threshold, const unsigned int reverseAttributeIndex) const;

  const bool symmetricValuesExtendPastPresent(const vector<Value*>::const_iterator valueBegin, const vector<Value*>::const_iterator valueEnd, const unsigned int threshold, const unsigned int intersectionIndex) const;
  const bool symmetricValuesExtendFuturePresent(const Value& symmetricValue, const vector<Value*>::const_iterator valueBegin, const vector<Value*>::const_iterator valueEnd, const unsigned int threshold, const unsigned int reverseAttributeIndex) const;
  const bool symmetricValuesExtendPastPresentAndPotential(const vector<Value*>::const_iterator valueBegin, const vector<Value*>::const_iterator valueEnd, const unsigned int threshold, const unsigned int intersectionIndex) const;
  const bool symmetricValuesExtendFuturePresentAndPotential(const Value& symmetricValue, const vector<Value*>::const_iterator valueBegin, const vector<Value*>::const_iterator valueEnd, const unsigned int threshold, const unsigned int reverseAttributeIndex) const;

  /*debug*/
  /* vector<vector<unsigned int>> getIntersectionsWithPresentAndPotentialValues() const; */
  /* vector<vector<unsigned int>> getIntersectionsWithPresentValues() const; */
  /*/debug*/

  static const bool smallerDataId(const Value* value, const Value* otherValue);

 protected:
  unsigned int dataId;
  unsigned int intersectionId;
  unsigned int presentNoise;
  unsigned int presentAndPotentialNoise;
  vector<vector<unsigned int>> intersectionsWithPresentValues;
  vector<vector<unsigned int>> intersectionsWithPresentAndPotentialValues;
};

#endif /*VALUE_H_*/
