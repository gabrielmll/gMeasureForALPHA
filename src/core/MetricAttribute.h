// Copyright 2007,2008,2009,2010,2011,2012,2013,2014,2015 Loïc Cerf (lcerf@dcc.ufmg.br)

// This file is part of multidupehack.

// multidupehack is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License version 3 as published by the Free Software Foundation

// multidupehack is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along with multidupehack; if not, write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

#ifndef METRIC_ATTRIBUTE_H_
#define METRIC_ATTRIBUTE_H_

#include <unordered_map>

#include "Attribute.h"

class MetricAttribute: public Attribute
{
 public:
  MetricAttribute(const vector<unsigned int>& nbOfValuesPerAttribute, const double epsilon, const vector<string>& labels, const double tau);

  MetricAttribute* clone(const vector<Attribute*>::const_iterator parentAttributeIt, const vector<Attribute*>::const_iterator parentAttributeEnd, const vector<unsigned int>::const_iterator sizeOfAttributeIt, const vector<unsigned int>::const_iterator sizeOfAttributeEnd) const;

  void chooseValue();
  void setChosenValuePresent();	 /* to be called on the attribute that is the child of the one where the value was chosen */
  void setChosenValueAbsent(const bool isValuePotentiallyPreventingClosedness); /* to be called chooseValue (on the same object) */

  const bool findIrrelevantValuesAndCheckTauContiguity(const vector<Attribute*>::iterator attributeBegin, const vector<Attribute*>::iterator attributeEnd);
#ifdef MIN_SIZE_ELEMENT_PRUNING
  pair<bool, vector<unsigned int>> findPresentAndPotentialIrrelevantValuesAndCheckTauContiguity(const unsigned int presentAndPotentialIrrelevancyThreshold);
#endif
  const bool unclosed(const vector<Attribute*>::const_iterator attributeBegin, const vector<Attribute*>::const_iterator attributeEnd) const;
  pair<const bool, vector<unsigned int>> tauFarValueDataIdsAndCheckTauContiguity();
  const bool finalizable() const;
  vector<unsigned int> finalize(); /* returns the original ids of the elements moved to present */

  void eraseAbsentValuesBeneathTimestamp(const double timestamp);
  void eraseAbsentValuesBeyondTimestamp(const double timestamp);

 protected:
  static vector<double> tauVector; // 0 if the attribute is not totally ordered
  static vector<vector<double>> timestampsVector;

  MetricAttribute(const vector<Attribute*>::const_iterator parentAttributeIt, const vector<Attribute*>::const_iterator parentAttributeEnd, const vector<unsigned int>::const_iterator sizeOfAttributeIt, const vector<unsigned int>::const_iterator sizeOfAttributeEnd);

  void setChildValueIntersections(const vector<unsigned int>& noiseInIntersectionWithPresentValues, const vector<unsigned int>& noiseInIntersectionWithPresentAndPotentialValues, vector<unsigned int>& childNoiseInIntersectionWithPresentValues, vector<unsigned int>& childNoiseInIntersectionWithPresentAndPotentialValues) const;

  void removeAbsentValue(vector<Value*>::iterator& valueIt);
};

#endif /*METRIC_ATTRIBUTE_H_*/
