// Copyright 2007,2008,2009,2010,2011,2012,2013,2014,2015 Loïc Cerf (lcerf@dcc.ufmg.br)

// This file is part of multidupehack.

// multidupehack is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License version 3 as published by the Free Software Foundation

// multidupehack is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along with multidupehack; if not, write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

#ifndef ATTRIBUTE_H_
#define ATTRIBUTE_H_

#include <string>
#include <algorithm>
#include <boost/lexical_cast.hpp>

#include "Value.h"
#include "Element.h"

#ifdef DEBUG
#include <iostream>
#endif

using namespace boost;

class Attribute
{
 public:
  static unsigned int noisePerUnit;

#ifdef VERBOSE_DIM_CHOICE
  static vector<unsigned int> internal2ExternalAttributeOrder;
#endif

#ifdef DEBUG
  void printPresent(ostream& out) const;
  void printPotential(ostream& out) const;
  void printAbsent(ostream& out) const;
#endif

#if defined DEBUG || defined ASSERT
  void printValue(const Value& value, ostream& out) const;
  static void setInternal2ExternalAttributeOrder(const vector<unsigned int>& internal2ExternalAttributeOrder);
#endif

  Attribute();
  Attribute(const Attribute& otherAttribute) = delete;
  Attribute(Attribute&& otherAttribute) = delete;
  Attribute(const vector<unsigned int>& nbOfValuesPerAttribute, const double epsilon, const vector<string>& labels);
  virtual ~Attribute();

  virtual Attribute* clone(const vector<Attribute*>::const_iterator parentAttributeIt, const vector<Attribute*>::const_iterator parentAttributeEnd, const vector<unsigned int>::const_iterator sizeOfAttributeIt, const vector<unsigned int>::const_iterator sizeOfAttributeEnd) const;

  Attribute& operator=(const Attribute& otherAttribute) = delete;
  Attribute& operator=(Attribute&& otherAttribute) = delete;
  friend ostream& operator<<(ostream& out, const Attribute& attribute);
  void printValueFromDataId(const unsigned int valueDataId, ostream& out) const;

  const unsigned int getId() const;
  vector<unsigned int> getPresentDataIds() const;
  vector<unsigned int> getIrrelevantDataIds() const;
  vector<Value*>::iterator presentBegin();
  vector<Value*>::iterator presentEnd();
  vector<Value*>::iterator potentialBegin();
  vector<Value*>::iterator potentialEnd();
  vector<Value*>::iterator irrelevantBegin();
  vector<Value*>::iterator irrelevantEnd();
  vector<Value*>::iterator absentBegin();
  vector<Value*>::iterator absentEnd();
  const unsigned int sizeOfPresent() const;
  const unsigned int sizeOfPresentAndPotential() const;
  const bool potentialEmpty() const;
  const bool irrelevantEmpty() const;
  const unsigned int globalSize() const;
  const double totalPresentAndPotentialNoise() const;
  const double averagePresentAndPotentialNoise() const;
  
  virtual void chooseValue();
  Value& getChosenValue() const; /* to be called after chooseValue (on the same attribute or on its child) */
  virtual void setChosenValuePresent();	 /* to be called on the attribute that is the child of the one where the value was chosen */
  virtual void setChosenValueAbsent(const bool isValuePotentiallyPreventingClosedness); /* to be called chooseValue (on the same object) */
  void setPotentialValueIrrelevant(const vector<Value*>::iterator potentialValueIt);
  vector<unsigned int> eraseIrrelevantValues(); /* returns the original ids of the erased elements */

  // WARNING: These two methods should be called after initialization only (all values ordered in potential)
  vector<vector<unsigned int>>::iterator getIntersectionsBeginWithPotentialValues(const unsigned int valueId);
  void substractPotentialNoise(const unsigned int valueId, const unsigned int noise);

  const double getAppeal(const vector<Attribute*>::const_iterator attributeBegin, const vector<Attribute*>::const_iterator attributeEnd) const;
  virtual const bool findIrrelevantValuesAndCheckTauContiguity(const vector<Attribute*>::iterator attributeBegin, const vector<Attribute*>::iterator attributeEnd);

#ifdef MIN_SIZE_ELEMENT_PRUNING
  const bool presentAndPotentialIrrelevant(const unsigned int presentAndPotentialIrrelevancyThreshold) const;
  virtual pair<bool, vector<unsigned int>> findPresentAndPotentialIrrelevantValuesAndCheckTauContiguity(const unsigned int presentAndPotentialIrrelevancyThreshold);
  virtual void presentAndPotentialCleanAbsent(const unsigned int presentAndPotentialIrrelevancyThreshold);
#endif

  virtual const bool unclosed(const vector<Attribute*>::const_iterator attributeBegin, const vector<Attribute*>::const_iterator attributeEnd) const;
  const bool valueDoesNotExtendPresentAndPotential(const Value& value, const vector<Attribute*>::const_iterator attributeBegin, const vector<Attribute*>::const_iterator attributeEnd) const;
  virtual void cleanAbsent(const vector<Attribute*>::const_iterator attributeBegin, const vector<Attribute*>::const_iterator attributeEnd);
  virtual void removeAbsentValue(vector<Value*>::iterator& valueIt);
  virtual pair<const bool, vector<unsigned int>> tauFarValueDataIdsAndCheckTauContiguity();
  virtual const bool finalizable() const;
  virtual vector<unsigned int> finalize(); /* returns the original ids of the elements moved to present */

  static const unsigned int lastAttributeId();
  static const vector<unsigned int>& getEpsilonVector();
  static void setIsClosedVector(const vector<bool>& isClosedVector);
  static void setDensityPrecedenceAndOutputFormat(const bool isDensestValuePreferred, const char* outputElementSeparator, const char* emptySetString, const char* elementNoiseSeparator, const bool isNoisePrinted);
  static const bool noisePrinted();
  static void printOutputElementSeparator(ostream& out);
  static void printEmptySetString(ostream& out);
  static void printNoise(const float noise, ostream& out);

  static const bool lessAppealingIrrelevant(const Attribute* attribute, const Attribute* otherAttribute);

#ifdef ASSERT
  vector<Value*>::const_iterator presentBegin() const;
  vector<Value*>::const_iterator presentEnd() const;
  vector<Value*>::const_iterator potentialBegin() const;
  vector<Value*>::const_iterator potentialEnd() const;
  vector<Value*>::const_iterator irrelevantBegin() const;
  vector<Value*>::const_iterator irrelevantEnd() const;
  vector<Value*>::const_iterator absentBegin() const;
  vector<Value*>::const_iterator absentEnd() const;
#endif

 protected:
  unsigned int id;
  vector<Value*> values;	/* present, then potential, then irrelevant, then absent */
  unsigned int potentialIndex;
  unsigned int irrelevantIndex;
  unsigned int absentIndex;

  static unsigned int maxId;
  static vector<unsigned int> epsilonVector;
  static vector<bool> isClosedVector;
  static vector<vector<string>> labelsVector;
  static bool isDensestValuePreferred;

  static string outputElementSeparator;
  static string emptySetString;
  static string elementNoiseSeparator;
  static bool isNoisePrinted;

#if !defined VERBOSE_DIM_CHOICE && (defined DEBUG || defined ASSERT)
  static vector<unsigned int> internal2ExternalAttributeOrder;
#endif

  Attribute(const vector<Attribute*>::const_iterator parentAttributeIt, const vector<Attribute*>::const_iterator parentAttributeEnd, const vector<unsigned int>::const_iterator sizeOfAttributeIt, const vector<unsigned int>::const_iterator sizeOfAttributeEnd);

  void printValues(const vector<Value*>::const_iterator begin, const vector<Value*>::const_iterator end, ostream& out) const;

  Value* createChildValue(const Value& parentValue, const unsigned int newId, const vector<unsigned int>::const_iterator sizeOfAttributeIt, const vector<unsigned int>::const_iterator sizeOfAttributeEnd, const vector<Attribute*>::const_iterator parentNextAttributeIt, const vector<Attribute*>::const_iterator parentAttributeEnd) const;
  virtual void setChildValueIntersections(const vector<unsigned int>& noiseInIntersectionWithPresentValues, const vector<unsigned int>& noiseInIntersectionWithPresentAndPotentialValues, vector<unsigned int>& childNoiseInIntersectionWithPresentValues, vector<unsigned int>& childNoiseInIntersectionWithPresentAndPotentialValues) const;

  const bool valueDoesNotExtendPresent(const Value& value, const vector<Attribute*>::const_iterator attributeBegin, const vector<Attribute*>::const_iterator attributeEnd) const;

#ifdef MIN_SIZE_ELEMENT_PRUNING
  const bool presentAndPotentialIrrelevantValue(const Value& value, const unsigned int presentAndPotentialIrrelevancyThreshold) const;
#endif

  static const bool lessNoisy(const Value* value, const Value* otherValue);
};

#endif /*ATTRIBUTE_H_*/
