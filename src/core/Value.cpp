// Copyright 2007,2008,2009,2010,2011,2012,2013,2014 Loïc Cerf (lcerf@dcc.ufmg.br)

// This file is part of multidupehack.

// multidupehack is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License version 3 as published by the Free Software Foundation

// multidupehack is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along with multidupehack; if not, write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

#include "Value.h"

Value::Value(const unsigned int dataIdParam): dataId(dataIdParam), intersectionId(dataIdParam), presentNoise(0), presentAndPotentialNoise(0), intersectionsWithPresentValues(), intersectionsWithPresentAndPotentialValues()
{
}

Value::Value(const unsigned int dataIdParam, const unsigned int presentAndPotentialNoiseParam, const vector<unsigned int>::const_iterator nbOfValuesPerAttributeBegin, const vector<unsigned int>::const_iterator nbOfValuesPerAttributeEnd, const vector<unsigned int>& noisesInIntersections): dataId(dataIdParam), intersectionId(dataIdParam), presentNoise(0), presentAndPotentialNoise(presentAndPotentialNoiseParam), intersectionsWithPresentValues(), intersectionsWithPresentAndPotentialValues()
{
  intersectionsWithPresentValues.reserve(noisesInIntersections.size());
  intersectionsWithPresentAndPotentialValues.reserve(noisesInIntersections.size());
  vector<unsigned int>::const_iterator sizeOfIntersectionIt = noisesInIntersections.begin();
  for (vector<unsigned int>::const_iterator nbOfValuesPerAttributeIt = nbOfValuesPerAttributeBegin; nbOfValuesPerAttributeIt != nbOfValuesPerAttributeEnd; ++nbOfValuesPerAttributeIt)
    {
      vector<unsigned int> intersectionsWithPresent(*nbOfValuesPerAttributeIt);
      intersectionsWithPresentValues.push_back(intersectionsWithPresent);
      vector<unsigned int> intersectionsWithPresentAndPotential(*nbOfValuesPerAttributeIt, *sizeOfIntersectionIt++);
      intersectionsWithPresentAndPotentialValues.push_back(intersectionsWithPresentAndPotential);
    }
}

Value::Value(const Value& parent, const unsigned int intersectionIdParam, const vector<unsigned int>::const_iterator sizeOfAttributeIt, const vector<unsigned int>::const_iterator sizeOfAttributeEnd): dataId(parent.dataId), intersectionId(intersectionIdParam), presentNoise(parent.presentNoise), presentAndPotentialNoise(parent.presentAndPotentialNoise), intersectionsWithPresentValues(), intersectionsWithPresentAndPotentialValues()
{
  const unsigned int sizeOfIntersections = parent.intersectionsWithPresentValues.size();
  intersectionsWithPresentValues.resize(sizeOfIntersections);
  vector<vector<unsigned int>>::iterator intersectionsWithPresentValuesIt = intersectionsWithPresentValues.begin();
  intersectionsWithPresentAndPotentialValues.resize(sizeOfIntersections);
  vector<vector<unsigned int>>::iterator intersectionsWithPresentAndPotentialValuesIt = intersectionsWithPresentAndPotentialValues.begin();
  for (vector<unsigned int>::const_iterator sizeIt = sizeOfAttributeIt; sizeIt != sizeOfAttributeEnd; ++sizeIt)
    {
      intersectionsWithPresentValuesIt->reserve(*sizeIt);
      ++intersectionsWithPresentValuesIt;
      intersectionsWithPresentAndPotentialValuesIt->reserve(*sizeIt);
      ++intersectionsWithPresentAndPotentialValuesIt;
    }
}

const bool Value::operator<(const Value& otherValue) const
{
#if ENUMERATION_PROCESS == 0
  const unsigned int potentialNoise = presentAndPotentialNoise - presentNoise;
  const unsigned int otherPotentialNoise = otherValue.presentAndPotentialNoise - otherValue.presentNoise;
  return potentialNoise < otherPotentialNoise || (potentialNoise == otherPotentialNoise && presentNoise < otherValue.presentNoise);
#endif
#if ENUMERATION_PROCESS == 1
  return presentNoise < otherValue.presentNoise || (presentNoise == otherValue.presentNoise && presentAndPotentialNoise < otherValue.presentAndPotentialNoise);
#endif
}

const unsigned int Value::getIntersectionId() const
{
  return intersectionId;
}

const unsigned int Value::getDataId() const
{
  return dataId;
}

const unsigned int Value::getPresentNoise() const
{
  return presentNoise;
}

const unsigned int Value::getPresentAndPotentialNoise() const
{
  return presentAndPotentialNoise;
}

void Value::addPresentNoise(const unsigned int noise)
{
  presentNoise += noise;
}

void Value::substractPotentialNoise(const unsigned int noise)
{
  presentAndPotentialNoise -= noise;
}

vector<vector<unsigned int>>::iterator Value::getIntersectionsBeginWithPresentValues()
{
  return intersectionsWithPresentValues.begin();
}

vector<vector<unsigned int>>::const_iterator Value::getIntersectionsBeginWithPresentValues() const
{
  return intersectionsWithPresentValues.begin();
}

vector<vector<unsigned int>>::iterator Value::getIntersectionsBeginWithPresentAndPotentialValues()
{
  return intersectionsWithPresentAndPotentialValues.begin();
}

vector<vector<unsigned int>>::const_iterator Value::getIntersectionsBeginWithPresentAndPotentialValues() const
{
  return intersectionsWithPresentAndPotentialValues.begin();
}

const bool Value::extendsPastPresent(const vector<Value*>::const_iterator valueBegin, const vector<Value*>::const_iterator valueEnd, const unsigned int threshold, const unsigned int intersectionIndex) const
{
  vector<Value*>::const_iterator valueIt = valueBegin;
  for (; valueIt != valueEnd && (*valueIt)->presentNoise + (*valueIt)->intersectionsWithPresentValues[intersectionIndex][intersectionId] <= threshold; ++valueIt)
    {
    }
  return valueIt == valueEnd;
}

const bool Value::extendsFuturePresent(const vector<Value*>::const_iterator valueBegin, const vector<Value*>::const_iterator valueEnd, const unsigned int threshold, const unsigned int reverseAttributeIndex) const
{
  const vector<unsigned int>& presentIntersectionsWithFutureValues = intersectionsWithPresentValues[intersectionsWithPresentValues.size() - reverseAttributeIndex];
  vector<Value*>::const_iterator valueIt = valueBegin;
  for (; valueIt != valueEnd && (*valueIt)->presentNoise + presentIntersectionsWithFutureValues[(*valueIt)->intersectionId] <= threshold; ++valueIt)
    {
    }
  return valueIt == valueEnd;
}

const bool Value::extendsPastPresentAndPotential(const vector<Value*>::const_iterator valueBegin, const vector<Value*>::const_iterator valueEnd, const unsigned int threshold, const unsigned int intersectionIndex) const
{
  vector<Value*>::const_iterator valueIt = valueBegin;
  for (; valueIt != valueEnd && (*valueIt)->presentAndPotentialNoise + (*valueIt)->intersectionsWithPresentAndPotentialValues[intersectionIndex][intersectionId] <= threshold; ++valueIt)
    {
    }
  return valueIt == valueEnd;
}

const bool Value::extendsFuturePresentAndPotential(const vector<Value*>::const_iterator valueBegin, const vector<Value*>::const_iterator valueEnd, const unsigned int threshold, const unsigned int reverseAttributeIndex) const
{
  const vector<unsigned int>& presentAndPotentialIntersectionsWithFutureValues = intersectionsWithPresentAndPotentialValues[intersectionsWithPresentAndPotentialValues.size() - reverseAttributeIndex];
  vector<Value*>::const_iterator valueIt = valueBegin;
  for (; valueIt != valueEnd && (*valueIt)->presentAndPotentialNoise + presentAndPotentialIntersectionsWithFutureValues[(*valueIt)->intersectionId] <= threshold; ++valueIt)
    {
    }
  return valueIt == valueEnd;
}

const bool Value::symmetricValuesExtendPastPresent(const vector<Value*>::const_iterator valueBegin, const vector<Value*>::const_iterator valueEnd, const unsigned int threshold, const unsigned int intersectionIndex) const
{
  vector<Value*>::const_iterator valueIt = valueBegin;
  for (; valueIt != valueEnd && (*valueIt)->presentNoise + (*valueIt)->intersectionsWithPresentValues[intersectionIndex][intersectionId] + (*valueIt)->intersectionsWithPresentValues[intersectionIndex + 1][intersectionId] <= threshold; ++valueIt)
    {
    }
  return valueIt == valueEnd;
}

const bool Value::symmetricValuesExtendFuturePresent(const Value& symmetricValue, const vector<Value*>::const_iterator valueBegin, const vector<Value*>::const_iterator valueEnd, const unsigned int threshold, const unsigned int reverseAttributeIndex) const
{
  const unsigned int intersectionIndex = intersectionsWithPresentValues.size() - reverseAttributeIndex;
  const vector<unsigned int>& presentIntersectionsWithFutureValues1 = intersectionsWithPresentValues[intersectionIndex + 1];
  const vector<unsigned int>& presentIntersectionsWithFutureValues2 = symmetricValue.intersectionsWithPresentValues[intersectionIndex];
  vector<Value*>::const_iterator valueIt = valueBegin;
  for (; valueIt != valueEnd && (*valueIt)->presentNoise + presentIntersectionsWithFutureValues1[(*valueIt)->intersectionId] + presentIntersectionsWithFutureValues2[(*valueIt)->intersectionId] <= threshold; ++valueIt)
    {
    }
  return valueIt == valueEnd;
}

const bool Value::symmetricValuesExtendPastPresentAndPotential(const vector<Value*>::const_iterator valueBegin, const vector<Value*>::const_iterator valueEnd, const unsigned int threshold, const unsigned int intersectionIndex) const
{
  vector<Value*>::const_iterator valueIt = valueBegin;
  for (; valueIt != valueEnd && (*valueIt)->presentAndPotentialNoise + (*valueIt)->intersectionsWithPresentAndPotentialValues[intersectionIndex][intersectionId] + (*valueIt)->intersectionsWithPresentAndPotentialValues[intersectionIndex + 1][intersectionId] <= threshold; ++valueIt)
    {
    }
  return valueIt == valueEnd;
}

const bool Value::symmetricValuesExtendFuturePresentAndPotential(const Value& symmetricValue, const vector<Value*>::const_iterator valueBegin, const vector<Value*>::const_iterator valueEnd, const unsigned int threshold, const unsigned int reverseAttributeIndex) const
{
  const unsigned int intersectionIndex = intersectionsWithPresentAndPotentialValues.size() - reverseAttributeIndex;
  const vector<unsigned int>& presentAndPotentialIntersectionsWithFutureValues1 = intersectionsWithPresentAndPotentialValues[intersectionIndex + 1];
  const vector<unsigned int>& presentAndPotentialIntersectionsWithFutureValues2 = symmetricValue.intersectionsWithPresentAndPotentialValues[intersectionIndex];
  vector<Value*>::const_iterator valueIt = valueBegin;
  for (; valueIt != valueEnd && (*valueIt)->presentAndPotentialNoise + presentAndPotentialIntersectionsWithFutureValues1[(*valueIt)->intersectionId] + presentAndPotentialIntersectionsWithFutureValues2[(*valueIt)->intersectionId] <= threshold; ++valueIt)
    {
    }
  return valueIt == valueEnd;
}

/*debug*/
// vector<vector<unsigned int>> Value::getIntersectionsWithPresentValues() const
// {
//   return intersectionsWithPresentValues;
// }

// vector<vector<unsigned int>> Value::getIntersectionsWithPresentAndPotentialValues() const
// {
//   return intersectionsWithPresentAndPotentialValues;
// }
/*/debug*/

const bool Value::smallerDataId(const Value* value, const Value* otherValue)
{
  return value->getDataId() < otherValue->getDataId();
}
