// Copyright 2007,2008,2009,2010,2011,2012,2013,2014,2015 Loïc Cerf (lcerf@dcc.ufmg.br)

// This file is part of multidupehack.

// multidupehack is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License version 3 as published by the Free Software Foundation

// multidupehack is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along with multidupehack; if not, write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

#include "SymmetricAttribute.h"

SymmetricAttribute::SymmetricAttribute(): Attribute(), symmetricAttribute(nullptr)
{
}

SymmetricAttribute::SymmetricAttribute(const vector<unsigned int>& nbOfValuesPerAttribute, const double epsilon, const vector<string>& labels): Attribute(nbOfValuesPerAttribute, epsilon, labels), symmetricAttribute(nullptr)
{
}

SymmetricAttribute::SymmetricAttribute(const vector<Attribute*>::const_iterator parentAttributeIt, const vector<Attribute*>::const_iterator parentAttributeEnd, const vector<unsigned int>::const_iterator sizeOfAttributeIt, const vector<unsigned int>::const_iterator sizeOfAttributeEnd): Attribute(parentAttributeIt, parentAttributeEnd, sizeOfAttributeIt, sizeOfAttributeEnd), symmetricAttribute(nullptr)
{
}

SymmetricAttribute* SymmetricAttribute::clone(const vector<Attribute*>::const_iterator parentAttributeIt, const vector<Attribute*>::const_iterator parentAttributeEnd, const vector<unsigned int>::const_iterator sizeOfAttributeIt, const vector<unsigned int>::const_iterator sizeOfAttributeEnd) const
{
  return new SymmetricAttribute(parentAttributeIt, parentAttributeEnd, sizeOfAttributeIt, sizeOfAttributeEnd);
}

void SymmetricAttribute::setSymmetricAttribute(SymmetricAttribute* symmetricAttributeParam)
{
  symmetricAttribute = symmetricAttributeParam;
}

void SymmetricAttribute::chooseValue()
{
  vector<Value*>::iterator valueIt = values.begin() + potentialIndex;
  vector<Value*>::iterator bestValueIt = valueIt;
  Value*& potentialFront = *valueIt;
  vector<Value*>::iterator symmetricValueIt = symmetricAttribute->potentialBegin();
  vector<Value*>::iterator bestSymmetricValueIt = symmetricValueIt;
  Value*& symmetricPotentialFront = *symmetricValueIt;
  unsigned int bestPresentNoise = potentialFront->getPresentNoise() + symmetricPotentialFront->getPresentNoise();
  unsigned int bestPresentAndPotentialNoise = (*valueIt)->getPresentAndPotentialNoise() + (*symmetricValueIt)->getPresentAndPotentialNoise();
  const vector<Value*>::iterator end = values.begin() + absentIndex;
  if (isDensestValuePreferred)
    {
      while (++valueIt != end)
	{
	  ++symmetricValueIt;
	  const unsigned int presentNoise = (*valueIt)->getPresentNoise() + (*symmetricValueIt)->getPresentNoise();
	  if (presentNoise < bestPresentNoise)
	    {
	      bestPresentNoise = presentNoise;
	      bestPresentAndPotentialNoise = (*valueIt)->getPresentAndPotentialNoise() + (*symmetricValueIt)->getPresentAndPotentialNoise();
	      bestValueIt = valueIt;
	      bestSymmetricValueIt = symmetricValueIt;
	    }
	  else
	    {
	      if (presentNoise == bestPresentNoise)
		{
		  const unsigned int presentAndPotentialNoise = (*valueIt)->getPresentAndPotentialNoise() + (*symmetricValueIt)->getPresentAndPotentialNoise();
		  if (presentAndPotentialNoise < bestPresentAndPotentialNoise)
		    {
		      bestPresentAndPotentialNoise = presentAndPotentialNoise;
		      bestValueIt = valueIt;
		      bestSymmetricValueIt = symmetricValueIt;
		    }
		}
	    }
	}
    }
  else
    {
      while (++valueIt != end)
	{
	  ++symmetricValueIt;
	  const unsigned int presentNoise = (*valueIt)->getPresentNoise() + (*symmetricValueIt)->getPresentNoise();
	  if (presentNoise > bestPresentNoise)
	    {
	      bestPresentNoise = presentNoise;
	      bestPresentAndPotentialNoise = (*valueIt)->getPresentAndPotentialNoise() + (*symmetricValueIt)->getPresentAndPotentialNoise();
	      bestValueIt = valueIt;
	      bestSymmetricValueIt = symmetricValueIt;
	    }
	  else
	    {
	      if (presentNoise == bestPresentNoise)
		{
		  const unsigned int presentAndPotentialNoise = (*valueIt)->getPresentAndPotentialNoise() + (*symmetricValueIt)->getPresentAndPotentialNoise();
		  if (presentAndPotentialNoise > bestPresentAndPotentialNoise)
		    {
		      bestPresentAndPotentialNoise = presentAndPotentialNoise;
		      bestValueIt = valueIt;
		      bestSymmetricValueIt = symmetricValueIt;
		    }
		}
	    }
	}
    }
  swap(*bestValueIt, potentialFront);
  swap(*bestSymmetricValueIt, symmetricPotentialFront);
}

void SymmetricAttribute::setChosenValuePresent()
{
  Attribute::setChosenValuePresent();
  if (id < symmetricAttribute->id)
    {
      symmetricAttribute->setChosenValuePresent();
    }
}

void SymmetricAttribute::setChosenValueAbsent(const bool isValuePotentiallyPreventingClosedness)
{
  Attribute::setChosenValueAbsent(isValuePotentiallyPreventingClosedness);
  if (id < symmetricAttribute->id)
    {
      symmetricAttribute->setChosenValueAbsent(isValuePotentiallyPreventingClosedness);
    }
}

const bool SymmetricAttribute::findIrrelevantValuesAndCheckTauContiguity(const vector<Attribute*>::iterator attributeBegin, const vector<Attribute*>::iterator attributeEnd)
{
  if (id < symmetricAttribute->id)
    {
      vector<Value*>::iterator symmetricPotentialValueIt = symmetricAttribute->potentialBegin();
      vector<Value*>::iterator potentialEnd = values.begin() + irrelevantIndex;
      for (vector<Value*>::iterator potentialValueIt = values.begin() + potentialIndex; potentialValueIt != potentialEnd; )
	{
	  if (symmetricValuesDoNotExtendPresent(**potentialValueIt, **symmetricPotentialValueIt, attributeBegin, attributeEnd))
	    {
#ifdef DEBUG
	      cout << labelsVector[id][(*potentialValueIt)->getDataId()] << " in attributes " << internal2ExternalAttributeOrder[id] << " and " << internal2ExternalAttributeOrder[id + 1] << " will never be present nor prevent the closedness of any future pattern" << endl;
#endif
	      swap(*potentialValueIt, *--potentialEnd);
	      --irrelevantIndex;
	      symmetricAttribute->setPotentialValueIrrelevant(symmetricPotentialValueIt);
	    }
	  else
	    {
	      ++potentialValueIt;
	      ++symmetricPotentialValueIt;
	    }
	}
    }
  return false;
}

#ifdef MIN_SIZE_ELEMENT_PRUNING
pair<bool, vector<unsigned int>> SymmetricAttribute::findPresentAndPotentialIrrelevantValuesAndCheckTauContiguity(const unsigned int presentAndPotentialIrrelevancyThreshold)
{
  vector<unsigned int> newIrrelevantValueDataIds;
  vector<Value*>::iterator symmetricPotentialValueIt = symmetricAttribute->potentialBegin();
  vector<Value*>::iterator potentialEnd = values.begin() + irrelevantIndex;
  for (vector<Value*>::iterator potentialValueIt = values.begin() + potentialIndex; potentialValueIt != potentialEnd; )
    {
      // **potentialValueIt is irrelevant if it contains too much noise in any extension satisfying the minimal size constraints
      if (presentAndPotentialIrrelevantValue(**potentialValueIt, presentAndPotentialIrrelevancyThreshold))
	{
	  newIrrelevantValueDataIds.push_back((*potentialValueIt)->getDataId());
	  swap(*potentialValueIt, *--potentialEnd);
	  --irrelevantIndex;
	  symmetricAttribute->setPotentialValueIrrelevant(symmetricPotentialValueIt);
	}
      else
	{
	  ++potentialValueIt;
	  ++symmetricPotentialValueIt;
	}
    }
  return pair<bool, vector<unsigned int>>(false, newIrrelevantValueDataIds);
}

void SymmetricAttribute::presentAndPotentialCleanAbsent(const unsigned int presentAndPotentialIrrelevancyThreshold)
{
  vector<Value*>::iterator symmetricAbsentValueIt = symmetricAttribute->absentBegin();
  for (vector<Value*>::iterator absentValueIt = values.begin() + absentIndex; absentValueIt != values.end(); )
    {
      if ((*absentValueIt)->getPresentAndPotentialNoise() > presentAndPotentialIrrelevancyThreshold)
	{
	  removeAbsentValue(absentValueIt);
	  symmetricAttribute->removeAbsentValue(symmetricAbsentValueIt);
	}
      else
	{
	  ++absentValueIt;
	  ++symmetricAbsentValueIt;
	}
    }
}
#endif

const bool SymmetricAttribute::symmetricValuesDoNotExtendPresent(const Value& value, const Value& symmetricValue, const vector<Attribute*>::const_iterator attributeBegin, const vector<Attribute*>::const_iterator attributeEnd) const
{
  const vector<unsigned int>::const_iterator thisEpsilonIt = epsilonVector.begin() + id;
  if (value.getPresentNoise() > *thisEpsilonIt || symmetricValue.getPresentNoise() > *(thisEpsilonIt + 1))
    {
      return true;
    }
  vector<Attribute*>::const_iterator attributeIt = attributeBegin;
  vector<unsigned int>::const_iterator epsilonIt = epsilonVector.begin();
  for (unsigned int intersectionIndex = id; epsilonIt != thisEpsilonIt && value.symmetricValuesExtendPastPresent((*attributeIt)->presentBegin(), (*attributeIt)->presentEnd(), *epsilonIt, --intersectionIndex); ++epsilonIt)
    {
      ++attributeIt;
    }
  unsigned int reverseAttributeIndex = maxId - id - 1;
  if (!(epsilonIt == thisEpsilonIt && symmetricValue.extendsPastPresent(values.begin(), values.begin() + potentialIndex, *epsilonIt, 0) && value.extendsFuturePresent(symmetricAttribute->presentBegin(), symmetricAttribute->presentEnd(), *++epsilonIt, reverseAttributeIndex)))
    {
      return true;
    }
  for (attributeIt += 2; attributeIt != attributeEnd && value.symmetricValuesExtendFuturePresent(symmetricValue, (*attributeIt)->presentBegin(), (*attributeIt)->presentEnd(), *++epsilonIt, reverseAttributeIndex--); ++attributeIt)
    {
    }
  return attributeIt != attributeEnd;
}

const bool SymmetricAttribute::unclosed(const vector<Attribute*>::const_iterator attributeBegin, const vector<Attribute*>::const_iterator attributeEnd) const
{
  if (id > symmetricAttribute->id)
    {
      return false;
    }
  vector<Value*>::const_iterator symmetricAbsentValueIt = symmetricAttribute->absentBegin();
  vector<Value*>::const_iterator absentEnd = values.end();
  vector<Value*>::const_iterator absentValueIt = values.begin() + absentIndex;
  for (; absentValueIt != absentEnd && symmetricValuesDoNotExtendPresentAndPotential(**absentValueIt, **symmetricAbsentValueIt, attributeBegin, attributeEnd); ++absentValueIt)
    {
      ++symmetricAbsentValueIt;
    }
#ifdef DEBUG
  if (absentValueIt != absentEnd)
    {
      cout << labelsVector[id][(*absentValueIt)->getDataId()] << " in attributes " << internal2ExternalAttributeOrder[id] << " and " << internal2ExternalAttributeOrder[id + 1] << " extend the pattern -> Prune!" << endl;
    }
#endif
  return absentValueIt != absentEnd;
}

const bool SymmetricAttribute::symmetricValuesDoNotExtendPresentAndPotential(const Value& value, const Value& symmetricValue, const vector<Attribute*>::const_iterator attributeBegin, const vector<Attribute*>::const_iterator attributeEnd) const
{
  const vector<unsigned int>::const_iterator thisEpsilonIt = epsilonVector.begin() + id;
  if (value.getPresentAndPotentialNoise() > *thisEpsilonIt || symmetricValue.getPresentAndPotentialNoise() > (*thisEpsilonIt + 1))
    {
      return true;
    }
  vector<Attribute*>::const_iterator attributeIt = attributeBegin;
  vector<unsigned int>::const_iterator epsilonIt = epsilonVector.begin();
  for (unsigned int intersectionIndex = id; epsilonIt != thisEpsilonIt && value.symmetricValuesExtendPastPresentAndPotential((*attributeIt)->presentBegin(), (*attributeIt)->presentEnd(), *epsilonIt, --intersectionIndex); ++epsilonIt)
    {
      ++attributeIt;
    }
  unsigned int reverseAttributeIndex = maxId - id - 1;
  if (!(epsilonIt == thisEpsilonIt && symmetricValue.extendsPastPresentAndPotential(values.begin(), values.begin() + potentialIndex, *epsilonIt, 0) && value.extendsFuturePresentAndPotential(symmetricAttribute->presentBegin(), symmetricAttribute->presentEnd(), *++epsilonIt, reverseAttributeIndex)))
    {
      return true;
    }
  for (attributeIt += 2; attributeIt != attributeEnd && value.symmetricValuesExtendFuturePresentAndPotential(symmetricValue, (*attributeIt)->presentBegin(), (*attributeIt)->presentEnd(), *++epsilonIt, reverseAttributeIndex--); ++attributeIt)
    {
    }
  return attributeIt != attributeEnd;
}

void SymmetricAttribute::cleanAbsent(const vector<Attribute*>::const_iterator attributeBegin, const vector<Attribute*>::const_iterator attributeEnd)
{
  if (id < symmetricAttribute->id)
    {
      vector<Value*>::iterator symmetricAbsentValueIt = symmetricAttribute->absentBegin();
      for (vector<Value*>::iterator absentValueIt = values.begin() + absentIndex; absentValueIt != values.end(); )
	{
	  if (symmetricValuesDoNotExtendPresent(**absentValueIt, **symmetricAbsentValueIt, attributeBegin, attributeEnd))
	    {
	      removeAbsentValue(absentValueIt);
	      symmetricAttribute->removeAbsentValue(symmetricAbsentValueIt);
	    }
	  else
	    {
	      ++absentValueIt;
	      ++symmetricAbsentValueIt;
	    }
	}
    }
}
