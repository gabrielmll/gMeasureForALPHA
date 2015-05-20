// Copyright 2007,2008,2009,2010,2011,2012,2013,2014,2015 Loïc Cerf (lcerf@dcc.ufmg.br)

// This file is part of multidupehack.

// multidupehack is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License version 3 as published by the Free Software Foundation

// multidupehack is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along with multidupehack; if not, write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

#ifndef SYMMETRIC_ATTRIBUTE_H_
#define SYMMETRIC_ATTRIBUTE_H_

#include "Attribute.h"

/* Although the code in Tree works for any number of symmetric attributes, this implementation is specific to two such attributes */
class SymmetricAttribute: public Attribute
{
 public:
  SymmetricAttribute();
  SymmetricAttribute(const SymmetricAttribute& otherSymmetricAttribute) = delete;
  SymmetricAttribute(Attribute&& otherSymmetricAttribute) = delete;
  SymmetricAttribute(const vector<unsigned int>& nbOfValuesPerAttribute, const double epsilon, const vector<string>& labels);
  
  SymmetricAttribute* clone(const vector<Attribute*>::const_iterator parentAttributeIt, const vector<Attribute*>::const_iterator parentAttributeEnd, const vector<unsigned int>::const_iterator sizeOfAttributeIt, const vector<unsigned int>::const_iterator sizeOfAttributeEnd) const;

  SymmetricAttribute& operator=(const SymmetricAttribute& otherSymmetricAttribute) = delete;
  SymmetricAttribute& operator=(SymmetricAttribute&& otherSymmetricAttribute) = delete;

  void setSymmetricAttribute(SymmetricAttribute* symmetricAttribute);
  
  void chooseValue();
  void setChosenValuePresent();
  void setChosenValueAbsent(const bool isValuePotentiallyPreventingClosedness);

  const bool findIrrelevantValuesAndCheckTauContiguity(const vector<Attribute*>::iterator attributeBegin, const vector<Attribute*>::iterator attributeEnd);
  pair<bool, vector<unsigned int>> findPresentAndPotentialIrrelevantValuesAndCheckTauContiguity(const unsigned int presentAndPotentialIrrelevancyThreshold);
  void presentAndPotentialCleanAbsent(const unsigned int presentAndPotentialIrrelevancyThreshold);
  const bool unclosed(const vector<Attribute*>::const_iterator attributeBegin, const vector<Attribute*>::const_iterator attributeEnd) const;
  void cleanAbsent(const vector<Attribute*>::const_iterator attributeBegin, const vector<Attribute*>::const_iterator attributeEnd);

 protected:
  SymmetricAttribute* symmetricAttribute;

  SymmetricAttribute(const vector<Attribute*>::const_iterator parentAttributeIt, const vector<Attribute*>::const_iterator parentAttributeEnd, const vector<unsigned int>::const_iterator sizeOfAttributeIt, const vector<unsigned int>::const_iterator sizeOfAttributeEnd);

  const bool symmetricValuesDoNotExtendPresent(const Value& value, const Value& symmetricValue, const vector<Attribute*>::const_iterator attributeBegin, const vector<Attribute*>::const_iterator attributeEnd) const; /* must only be called on the first symmetric attribute */
  const bool symmetricValuesDoNotExtendPresentAndPotential(const Value& value, const Value& symmetricValue, const vector<Attribute*>::const_iterator attributeBegin, const vector<Attribute*>::const_iterator attributeEnd) const; /* must only be called on the first symmetric attribute */
};

#endif /*SYMMETRIC_ATTRIBUTE_H_*/
