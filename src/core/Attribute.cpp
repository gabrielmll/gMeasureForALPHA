// Copyright 2007,2008,2009,2010,2011,2012,2013,2014,2015 Loïc Cerf (lcerf@dcc.ufmg.br)

// This file is part of multidupehack.

// multidupehack is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License version 3 as published by the Free Software Foundation

// multidupehack is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along with multidupehack; if not, write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

#include "Attribute.h"

unsigned int Attribute::noisePerUnit;

#if defined VERBOSE_DIM_CHOICE || defined DEBUG || defined ASSERT
vector<unsigned int> Attribute::internal2ExternalAttributeOrder;
#endif

unsigned int Attribute::maxId = 0;
vector<unsigned int> Attribute::epsilonVector;
vector<bool> Attribute::isClosedVector;
vector<vector<string>> Attribute::labelsVector;
bool Attribute::isDensestValuePreferred;

string Attribute::outputElementSeparator;
string Attribute::emptySetString;
string Attribute::elementNoiseSeparator;
bool Attribute::isNoisePrinted;

Attribute::Attribute(): id(0), values(), potentialIndex(0), irrelevantIndex(0), absentIndex(0)
{
}

Attribute::Attribute(const vector<unsigned int>& nbOfValuesPerAttribute, const double epsilon, const vector<string>& labels): id(maxId++), values(), potentialIndex(0), irrelevantIndex(0), absentIndex(0)
{
  const vector<unsigned int>::const_iterator nbOfValuesInThisAttributeIt = nbOfValuesPerAttribute.begin() + id;
  irrelevantIndex = *nbOfValuesInThisAttributeIt;
  absentIndex = *nbOfValuesInThisAttributeIt;
  unsigned int sizeOfAValue = noisePerUnit;
  for (vector<unsigned int>::const_iterator nbOfValuesInAttributeIt = nbOfValuesPerAttribute.begin(); nbOfValuesInAttributeIt != nbOfValuesPerAttribute.end(); ++nbOfValuesInAttributeIt)
    {
      if (nbOfValuesInAttributeIt != nbOfValuesInThisAttributeIt)
	{
	  sizeOfAValue *= *nbOfValuesInAttributeIt;
	}
    }
  vector<unsigned int> noisesInIntersections(nbOfValuesPerAttribute.size() - id - 1, noisePerUnit);
  vector<unsigned int>::const_iterator attributeIt = nbOfValuesInThisAttributeIt + 1;
  for (unsigned int& noiseInIntersection : noisesInIntersections)
    {
      for (vector<unsigned int>::const_iterator nbOfValuesInAttributeIt = nbOfValuesPerAttribute.begin(); nbOfValuesInAttributeIt != nbOfValuesPerAttribute.end(); ++nbOfValuesInAttributeIt)
	{
	  if (nbOfValuesInAttributeIt != attributeIt && nbOfValuesInAttributeIt != nbOfValuesInThisAttributeIt)
	    {
	      noiseInIntersection *= *nbOfValuesInAttributeIt;
	    }
	}
      ++attributeIt;
    }
  const vector<unsigned int>::const_iterator nbOfValuesInNextAttributeIt = nbOfValuesInThisAttributeIt + 1;
  values.reserve(absentIndex);
  for (unsigned int valueId = 0; valueId != *nbOfValuesInThisAttributeIt; ++valueId)
    {
      values.push_back(new Value(valueId, sizeOfAValue, nbOfValuesInNextAttributeIt, nbOfValuesPerAttribute.end(), noisesInIntersections));
    }
  epsilonVector.push_back(epsilon * noisePerUnit);
  labelsVector.push_back(labels);
}

Attribute::Attribute(const vector<Attribute*>::const_iterator parentAttributeIt, const vector<Attribute*>::const_iterator parentAttributeEnd, const vector<unsigned int>::const_iterator sizeOfAttributeIt, const vector<unsigned int>::const_iterator sizeOfAttributeEnd): id((*parentAttributeIt)->id), values(), potentialIndex((*parentAttributeIt)->potentialIndex), irrelevantIndex((*parentAttributeIt)->irrelevantIndex), absentIndex((*parentAttributeIt)->absentIndex)
{
  const vector<Attribute*>::const_iterator parentNextAttributeIt = parentAttributeIt + 1;
  unsigned int newId = 0;
  values.reserve((*parentAttributeIt)->values.size());
  for (const Value* value : (*parentAttributeIt)->values)
    {
      values.push_back(createChildValue(*value, newId++, sizeOfAttributeIt, sizeOfAttributeEnd, parentNextAttributeIt, parentAttributeEnd));
    }
}

Attribute::~Attribute()
{
  for (const Value* value : values)
    {
      delete value;
    }
}

Attribute* Attribute::clone(const vector<Attribute*>::const_iterator parentAttributeIt, const vector<Attribute*>::const_iterator parentAttributeEnd, const vector<unsigned int>::const_iterator sizeOfAttributeIt, const vector<unsigned int>::const_iterator sizeOfAttributeEnd) const
{
  return new Attribute(parentAttributeIt, parentAttributeEnd, sizeOfAttributeIt, sizeOfAttributeEnd);
}

ostream& operator<<(ostream& out, const Attribute& attribute)
{
  attribute.printValues(attribute.values.begin(), attribute.values.begin() + attribute.potentialIndex, out);
  return out;
}

Value* Attribute::createChildValue(const Value& parentValue, const unsigned int newId, const vector<unsigned int>::const_iterator sizeOfAttributeIt, const vector<unsigned int>::const_iterator sizeOfAttributeEnd, const vector<Attribute*>::const_iterator parentNextAttributeIt, const vector<Attribute*>::const_iterator parentAttributeEnd) const
{
  Value* childValue = new Value(parentValue, newId, sizeOfAttributeIt, sizeOfAttributeEnd);
  vector<vector<unsigned int>>::iterator noiseInIntersectionWithPresentValuesIt = childValue->getIntersectionsBeginWithPresentValues();
  vector<vector<unsigned int>>::iterator noiseInIntersectionWithPresentAndPotentialValuesIt = childValue->getIntersectionsBeginWithPresentAndPotentialValues();
  vector<vector<unsigned int>>::const_iterator parentNoiseInIntersectionWithPresentValuesIt = parentValue.getIntersectionsBeginWithPresentValues();
  vector<vector<unsigned int>>::const_iterator parentNoiseInIntersectionWithPresentAndPotentialValuesIt = parentValue.getIntersectionsBeginWithPresentAndPotentialValues();
  for (vector<Attribute*>::const_iterator attributeIt = parentNextAttributeIt; attributeIt != parentAttributeEnd; ++attributeIt)
    {
      (*attributeIt)->setChildValueIntersections(*parentNoiseInIntersectionWithPresentValuesIt++, *parentNoiseInIntersectionWithPresentAndPotentialValuesIt++, *noiseInIntersectionWithPresentValuesIt++, *noiseInIntersectionWithPresentAndPotentialValuesIt++);
    }
  return childValue;
}

void Attribute::setChildValueIntersections(const vector<unsigned int>& noiseInIntersectionWithPresentValues, const vector<unsigned int>& noiseInIntersectionWithPresentAndPotentialValues, vector<unsigned int>& childNoiseInIntersectionWithPresentValues, vector<unsigned int>& childNoiseInIntersectionWithPresentAndPotentialValues) const
{
  for (const Value* value : values)
    {
      const unsigned int valueIntersectionId = value->getIntersectionId();
      childNoiseInIntersectionWithPresentValues.push_back(noiseInIntersectionWithPresentValues[valueIntersectionId]);
      childNoiseInIntersectionWithPresentAndPotentialValues.push_back(noiseInIntersectionWithPresentAndPotentialValues[valueIntersectionId]);
    }
}

const unsigned int Attribute::getId() const
{
  return id;
}

vector<unsigned int> Attribute::getPresentDataIds() const
{
  vector<unsigned int> dataIds;
  dataIds.reserve(potentialIndex);
  const vector<Value*>::const_iterator end = values.begin() + potentialIndex;
  for (vector<Value*>::const_iterator valueIt = values.begin(); valueIt != end; ++valueIt)
    {
      dataIds.push_back((*valueIt)->getDataId());
    }
  return dataIds;
}

vector<unsigned int> Attribute::getIrrelevantDataIds() const
{
  vector<unsigned int> dataIds;
  dataIds.reserve(absentIndex - irrelevantIndex);
  const vector<Value*>::const_iterator end = values.begin() + absentIndex;
  for (vector<Value*>::const_iterator valueIt = values.begin() + irrelevantIndex; valueIt != end; ++valueIt)
    {
      dataIds.push_back((*valueIt)->getDataId());
    }
  return dataIds;
}

vector<Value*>::iterator Attribute::presentBegin()
{
  return values.begin();
}

vector<Value*>::iterator Attribute::presentEnd()
{
  return values.begin() + potentialIndex;
}

vector<Value*>::iterator Attribute::potentialBegin()
{
  return values.begin() + potentialIndex;
}

vector<Value*>::iterator Attribute::potentialEnd()
{
  return values.begin() + irrelevantIndex;
}

vector<Value*>::iterator Attribute::irrelevantBegin()
{
  return values.begin() + irrelevantIndex;
}

vector<Value*>::iterator Attribute::irrelevantEnd()
{
  return values.begin() + absentIndex;
}

vector<Value*>::iterator Attribute::absentBegin()
{
  return values.begin() + absentIndex;
}

vector<Value*>::iterator Attribute::absentEnd()
{
  return values.end();
}

const unsigned int Attribute::sizeOfPresent() const
{
  return potentialIndex;
}

const unsigned int Attribute::sizeOfPresentAndPotential() const
{
  return absentIndex;
}

const bool Attribute::potentialEmpty() const
{
  return potentialIndex == irrelevantIndex;
}

const bool Attribute::irrelevantEmpty() const
{
  return irrelevantIndex == absentIndex;
}

const unsigned int Attribute::globalSize() const
{
  return values.size();
}

const double Attribute::totalPresentAndPotentialNoise() const
{
  double totalNoise = 0;
  for (const Value* value : values)
    {
      totalNoise += value->getPresentAndPotentialNoise();      
    }
  return totalNoise;
}

const double Attribute::averagePresentAndPotentialNoise() const
{
  return totalPresentAndPotentialNoise() / values.size();
}

void Attribute::chooseValue()
{
  const vector<Value*>::iterator potentialBegin = values.begin() + potentialIndex;
  if (isDensestValuePreferred)
    {
      swap(*potentialBegin, *min_element(potentialBegin, values.begin() + absentIndex, lessNoisy));
    }
  swap(*potentialBegin, *max_element(potentialBegin, values.begin() + absentIndex, lessNoisy));
}

Value& Attribute::getChosenValue() const
{
  return *values[potentialIndex];
}

void Attribute::setChosenValuePresent()
{
  ++potentialIndex;
}

void Attribute::setChosenValueAbsent(const bool isValuePotentiallyPreventingClosedness)
{
  --irrelevantIndex;
  Value*& lastPotentialValue = values[--absentIndex];
  swap(values[potentialIndex], lastPotentialValue);
#ifdef DETECT_NON_EXTENSION_ELEMENTS
  if (!(isValuePotentiallyPreventingClosedness && isClosedVector[id]))
#else
  if (!isClosedVector[id])
#endif
    {
      delete lastPotentialValue;
      lastPotentialValue = values.back();
      values.pop_back();
    }
}

// It should be called after initialization only (values is ordered)
vector<vector<unsigned int>>::iterator Attribute::getIntersectionsBeginWithPotentialValues(const unsigned int valueId)
{
  return values[valueId]->getIntersectionsBeginWithPresentAndPotentialValues();
}

// It should be called after initialization only (values is ordered)
void Attribute::substractPotentialNoise(const unsigned int valueId, const unsigned int noise)
{
  values[valueId]->substractPotentialNoise(noise);
}

void Attribute::printValues(const vector<Value*>::const_iterator begin, const vector<Value*>::const_iterator end, ostream& out) const
{
  bool isFirstElement = true;
  vector<string>& labels = labelsVector[id];
  for (vector<Value*>::const_iterator valueIt = begin; valueIt != end; ++valueIt)
    {
      if (isFirstElement)
	{
	  isFirstElement = false;
	}
      else
	{
	  out << outputElementSeparator;
	}
      out << labels[(*valueIt)->getDataId()];
      if (isNoisePrinted)
	{
	  out << elementNoiseSeparator << static_cast<float>((*valueIt)->getPresentAndPotentialNoise()) / noisePerUnit;
	}
    }
  if (isFirstElement)
    {
      out << emptySetString;
    }
}

void Attribute::printValueFromDataId(const unsigned int valueDataId, ostream& out) const
{
  out << labelsVector[id][valueDataId];
}

#ifdef DEBUG
void Attribute::printPresent(ostream& out) const
{
  printValues(values.begin(), values.begin() + potentialIndex, out);
}

void Attribute::printPotential(ostream& out) const
{
  printValues(values.begin() + potentialIndex, values.begin() + absentIndex, out);
}

void Attribute::printAbsent(ostream& out) const
{
  printValues(values.begin() + absentIndex, values.end(), out);
}
#endif

#if defined DEBUG || defined ASSERT
void Attribute::printValue(const Value& value, ostream& out) const
{
  out << labelsVector[id][value.getDataId()] << " in attribute " << internal2ExternalAttributeOrder[id];
}

void Attribute::setInternal2ExternalAttributeOrder(const vector<unsigned int>& internal2ExternalAttributeOrderParam)
{
  internal2ExternalAttributeOrder = internal2ExternalAttributeOrderParam;
}
#endif

const double Attribute::getAppeal(const vector<Attribute*>::const_iterator attributeBegin, const vector<Attribute*>::const_iterator attributeEnd) const
{
#if ENUMERATION_PROCESS == 0
  unsigned int appeal = 0;
  for (vector<Attribute*>::const_iterator potentialAttributeIt = attributeBegin; potentialAttributeIt != attributeEnd; ++potentialAttributeIt)
    {
      const unsigned int potentialAttributeId = (*potentialAttributeIt)->id;
      if (potentialAttributeId != id)
      	{
      	  unsigned int yesFactor = (*potentialAttributeIt)->sizeOfPresentAndPotential() - (*potentialAttributeIt)->sizeOfPresent();
      	  for (vector<Attribute*>::const_iterator presentAttributeIt = attributeBegin; presentAttributeIt != attributeEnd; ++presentAttributeIt)
      	    {
	      const unsigned int presentAttributeId = (*presentAttributeIt)->id;
      	      if (presentAttributeId != id && presentAttributeId != potentialAttributeId)
      		{
      		  yesFactor *= (*presentAttributeIt)->potentialIndex;
      		}
      	    }
      	  appeal += yesFactor;
      	}
    }
  if (appeal != 0 || potentialIndex == 0)
    {
#ifdef VERBOSE_DIM_CHOICE
      cout << "Appeal of attribute " << internal2ExternalAttributeOrder[id] << ": " << appeal << endl;
#endif
      return appeal;
    }
  return -1;
#else
  unsigned int numerator = 0;
  unsigned int presentAndPotentialArea = 1;
  unsigned int presentArea = 1;
  for (vector<Attribute*>::const_iterator potentialAttributeIt = attributeBegin; potentialAttributeIt != attributeEnd; ++potentialAttributeIt)
    {
      const unsigned int potentialAttributeId = (*potentialAttributeIt)->id;
      if (potentialAttributeId != id)
	{
	  const unsigned int sizeOfPresent = (*potentialAttributeIt)->potentialIndex;
	  presentArea *= sizeOfPresent;
	  unsigned int sizeOfPotential = (*potentialAttributeIt)->absentIndex - sizeOfPresent;
	  presentAndPotentialArea *= (*potentialAttributeIt)->absentIndex;
	  for (vector<Attribute*>::const_iterator presentAttributeIt = attributeBegin; presentAttributeIt != attributeEnd; ++presentAttributeIt)
	    {
	      const unsigned int presentAttributeId = (*presentAttributeIt)->id;
	      if (presentAttributeId != id && presentAttributeId != potentialAttributeId)
		{
		  sizeOfPotential *= (*presentAttributeIt)->potentialIndex;
		}
	    }
	  numerator += sizeOfPotential;
	}
    }
  if (numerator != 0)
    {
      const unsigned int potentialArea = presentAndPotentialArea - presentArea;
      if (potentialArea == 0)
	{
	  // This is the only attribute with a non-empty potential set
#ifdef VERBOSE_DIM_CHOICE
          cout << "Appeal of attribute " << internal2ExternalAttributeOrder[id] << ": 0" << endl;
#endif
	  return 0;
	}
#ifdef VERBOSE_DIM_CHOICE
      cout << "Appeal of attribute " << internal2ExternalAttributeOrder[id] << ": " << static_cast<double>(numerator) / potentialArea << endl;
#endif
      return static_cast<double>(numerator) / potentialArea;
    }
  // At least one other attribute has an empty present set
  if (potentialIndex == 0)
    {
#ifdef VERBOSE_DIM_CHOICE
      cout << "Appeal of attribute " << internal2ExternalAttributeOrder[id] << ": 0" << endl;
#endif
      return 0;
    }
  return -1;
#endif
}

void Attribute::setPotentialValueIrrelevant(const vector<Value*>::iterator potentialValueIt)
{
  swap(*potentialValueIt, values[--irrelevantIndex]);
}

const bool Attribute::findIrrelevantValuesAndCheckTauContiguity(const vector<Attribute*>::iterator attributeBegin, const vector<Attribute*>::iterator attributeEnd)
{
  // These potential values are irrelevant if at least one previous present values is not extensible with it
  vector<Value*>::iterator potentialEnd = values.begin() + irrelevantIndex;
  for (vector<Value*>::iterator potentialValueIt = values.begin() + potentialIndex; potentialValueIt != potentialEnd; )
    {
      if (valueDoesNotExtendPresent(**potentialValueIt, attributeBegin, attributeEnd))
	{
#ifdef DEBUG
	  cout << labelsVector[id][(*potentialValueIt)->getDataId()] << " in attribute " << internal2ExternalAttributeOrder[id] << " will never be present nor prevent the closedness of any future pattern" << endl;
#endif
	  swap(*potentialValueIt, *--potentialEnd);
	  --irrelevantIndex;
	}
      else
	{
	  ++potentialValueIt;
	}
    }
  return false;
}

#ifdef MIN_SIZE_ELEMENT_PRUNING
const bool Attribute::presentAndPotentialIrrelevant(const unsigned int presentAndPotentialIrrelevancyThreshold) const
{
  const vector<Value*>::const_iterator end = values.begin() + potentialIndex;
  vector<Value*>::const_iterator presentValueIt = values.begin();
  for (; presentValueIt != end && (*presentValueIt)->getPresentAndPotentialNoise() <= presentAndPotentialIrrelevancyThreshold; ++presentValueIt)
    {
    }
#ifdef DEBUG
  if (presentValueIt != end)
    {
      cout << labelsVector[id][(*presentValueIt)->getDataId()] << " in attribute " << internal2ExternalAttributeOrder[id] << " contains too much potential noise given the size constraints -> Prune!" << endl;
    }
#endif
  return presentValueIt != end;
}

pair<bool, vector<unsigned int>> Attribute::findPresentAndPotentialIrrelevantValuesAndCheckTauContiguity(const unsigned int presentAndPotentialIrrelevancyThreshold)
{
  vector<unsigned int> newIrrelevantValueDataIds;
  vector<Value*>::iterator potentialEnd = values.begin() + irrelevantIndex;
  for (vector<Value*>::iterator potentialValueIt = values.begin() + potentialIndex; potentialValueIt != potentialEnd; )
    {
      // **potentialValueIt is irrelevant if it contains too much noise in any extension satisfying the minimal size constraints
      if (presentAndPotentialIrrelevantValue(**potentialValueIt, presentAndPotentialIrrelevancyThreshold))
	{
	  newIrrelevantValueDataIds.push_back((*potentialValueIt)->getDataId());
	  swap(*potentialValueIt, *--potentialEnd);
	  --irrelevantIndex;
	}
      else
	{
	  ++potentialValueIt;
	}
    }
  return pair<bool, vector<unsigned int>>(false, newIrrelevantValueDataIds);
}

const bool Attribute::presentAndPotentialIrrelevantValue(const Value& value, const unsigned int presentAndPotentialIrrelevancyThreshold) const
{
#ifdef DEBUG
  if (value.getPresentAndPotentialNoise() > presentAndPotentialIrrelevancyThreshold)
    {
      cout << "Given the minimal size constraints, " << labelsVector[id][value.getDataId()] << " in attribute " << internal2ExternalAttributeOrder[id] << " will never be present nor prevent the closedness of any future pattern" << endl;
    }
#endif
  return value.getPresentAndPotentialNoise() > presentAndPotentialIrrelevancyThreshold;
}

void Attribute::presentAndPotentialCleanAbsent(const unsigned int presentAndPotentialIrrelevancyThreshold)
{
  for (vector<Value*>::iterator valueIt = values.begin() + absentIndex; valueIt != values.end(); )
    {
      if ((*valueIt)->getPresentAndPotentialNoise() > presentAndPotentialIrrelevancyThreshold)
	{
	  removeAbsentValue(valueIt);
	}
      else
	{
	  ++valueIt;
	}
    }
}
#endif

const bool Attribute::valueDoesNotExtendPresent(const Value& value, const vector<Attribute*>::const_iterator attributeBegin, const vector<Attribute*>::const_iterator attributeEnd) const
{
  const vector<unsigned int>::const_iterator thisEpsilonIt = epsilonVector.begin() + id;
  if (value.getPresentNoise() > *thisEpsilonIt)
    {
      return true;
    }
  vector<Attribute*>::const_iterator attributeIt = attributeBegin;
  vector<unsigned int>::const_iterator epsilonIt = epsilonVector.begin() + (*attributeIt)->id;
  for (unsigned int intersectionIndex = id; epsilonIt != thisEpsilonIt && value.extendsPastPresent((*attributeIt)->presentBegin(), (*attributeIt)->presentEnd(), *epsilonIt, --intersectionIndex); ++epsilonIt)
    {
      ++attributeIt;
    }
  if (epsilonIt != thisEpsilonIt)
    {
      return true;
    }
  for (unsigned int reverseAttributeIndex = maxId - id; ++attributeIt != attributeEnd && value.extendsFuturePresent((*attributeIt)->presentBegin(), (*attributeIt)->presentEnd(), *(++epsilonIt), --reverseAttributeIndex); )
    {
    }
  return attributeIt != attributeEnd;
}

vector<unsigned int> Attribute::eraseIrrelevantValues()
{
  vector<unsigned int> dataIds;
  dataIds.reserve(absentIndex - irrelevantIndex);
  const vector<Value*>::iterator begin = values.begin() + irrelevantIndex;
  const vector<Value*>::iterator end = values.begin() + absentIndex;
  for (vector<Value*>::iterator valueIt = begin; valueIt != end; ++valueIt)
    {
      dataIds.push_back((*valueIt)->getDataId());
      delete *valueIt;
    }
  values.erase(begin, end);
  absentIndex = irrelevantIndex;
  return dataIds;
}

const bool Attribute::unclosed(const vector<Attribute*>::const_iterator attributeBegin, const vector<Attribute*>::const_iterator attributeEnd) const
{
  const vector<Value*>::const_iterator absentEnd = values.end();
  vector<Value*>::const_iterator absentValueIt = values.begin() + absentIndex;
  for (; absentValueIt != absentEnd && valueDoesNotExtendPresentAndPotential(**absentValueIt, attributeBegin, attributeEnd); ++absentValueIt)
    {
    }
#ifdef DEBUG
  if (absentValueIt != absentEnd)
    {
      cout << labelsVector[id][(*absentValueIt)->getDataId()] << " in attribute " << internal2ExternalAttributeOrder[id] << " extends any future pattern -> Prune!" << endl;
    }
#endif
  return absentValueIt != absentEnd;
}

const bool Attribute::valueDoesNotExtendPresentAndPotential(const Value& value, const vector<Attribute*>::const_iterator attributeBegin, const vector<Attribute*>::const_iterator attributeEnd) const
{
  const vector<unsigned int>::const_iterator thisEpsilonIt = epsilonVector.begin() + id;
  if (value.getPresentAndPotentialNoise() > *thisEpsilonIt)
    {
      return true;
    }
  vector<Attribute*>::const_iterator attributeIt = attributeBegin;
  vector<unsigned int>::const_iterator epsilonIt = epsilonVector.begin() + (*attributeIt)->id;
  for (unsigned int intersectionIndex = id; epsilonIt != thisEpsilonIt && value.extendsPastPresentAndPotential((*attributeIt)->presentBegin(), (*attributeIt)->potentialEnd(), *epsilonIt, --intersectionIndex); ++epsilonIt)
    {
      ++attributeIt;
    }
  if (epsilonIt != thisEpsilonIt)
    {
      return true;
    }
  for (unsigned int reverseAttributeIndex = maxId - id; ++attributeIt != attributeEnd && value.extendsFuturePresentAndPotential((*attributeIt)->presentBegin(), (*attributeIt)->potentialEnd(), *(++epsilonIt), --reverseAttributeIndex); )
    {
    }
  return attributeIt != attributeEnd;
}

void Attribute::cleanAbsent(const vector<Attribute*>::const_iterator attributeBegin, const vector<Attribute*>::const_iterator attributeEnd)
{
  for (vector<Value*>::iterator valueIt = values.begin() + absentIndex; valueIt != values.end(); )
    {
      if (valueDoesNotExtendPresent(**valueIt, attributeBegin, attributeEnd))
	{
	  removeAbsentValue(valueIt);
	}
      else
	{
	  ++valueIt;
	}
    }
}

void Attribute::removeAbsentValue(vector<Value*>::iterator& valueIt)
{
#ifdef DEBUG
  cout << labelsVector[id][(*valueIt)->getDataId()] << " in attribute " << internal2ExternalAttributeOrder[id] << " will never extend any future pattern" << endl;
#endif
  delete *valueIt;
  *valueIt = values.back();
  values.pop_back();
}

pair<const bool, vector<unsigned int>> Attribute::tauFarValueDataIdsAndCheckTauContiguity()
{
  return pair<const bool, vector<unsigned int>>(false, vector<unsigned int> {values[potentialIndex]->getDataId()});
}

const bool Attribute::finalizable() const
{
  if (isClosedVector[id] || potentialIndex == absentIndex)
    {
      const unsigned int epsilon = epsilonVector[id];
      const vector<Value*>::const_iterator end = values.begin() + absentIndex;
      vector<Value*>::const_iterator valueIt = values.begin();
      for (; valueIt != end && (*valueIt)->getPresentAndPotentialNoise() <= epsilon; ++valueIt)
	{
	}
      return valueIt == end;
    }
  return false;
}

vector<unsigned int> Attribute::finalize()
{
  vector<unsigned int> dataIdsOfValuesSetPresent;
  dataIdsOfValuesSetPresent.reserve(absentIndex - potentialIndex);
  const vector<Value*>::const_iterator end = values.begin() + absentIndex;
  for (vector<Value*>::const_iterator valueIt = values.begin() + potentialIndex; valueIt != end; ++valueIt)
    {
      dataIdsOfValuesSetPresent.push_back((*valueIt)->getDataId());
    }
  potentialIndex = absentIndex;
  return dataIdsOfValuesSetPresent;
}

const unsigned int Attribute::lastAttributeId()
{
  return maxId - 1;
}

const vector<unsigned int>& Attribute::getEpsilonVector()
{
  return epsilonVector;
}

void Attribute::setIsClosedVector(const vector<bool>& isClosedVectorParam)
{
  isClosedVector = isClosedVectorParam;
}

void Attribute::setDensityPrecedenceAndOutputFormat(const bool isDensestValuePreferredParam, const char* outputElementSeparatorParam, const char* emptySetStringParam, const char* elementNoiseSeparatorParam, const bool isNoisePrintedParam)
{
  isDensestValuePreferred = isDensestValuePreferredParam;
  outputElementSeparator = outputElementSeparatorParam;
  emptySetString = emptySetStringParam;
  elementNoiseSeparator = elementNoiseSeparatorParam;
  isNoisePrinted = isNoisePrintedParam;
}

const bool Attribute::noisePrinted()
{
  return isNoisePrinted;
}

void Attribute::printOutputElementSeparator(ostream& out)
{
  out << outputElementSeparator;
}

void Attribute::printEmptySetString(ostream& out)
{
  out << emptySetString;
}

void Attribute::printNoise(const float noise, ostream& out)
{
  out << elementNoiseSeparator << noise;
}

#ifdef ASSERT
vector<Value*>::const_iterator Attribute::presentBegin() const
{
  return values.begin();
}

vector<Value*>::const_iterator Attribute::presentEnd() const
{
  return values.begin() + potentialIndex;
}

vector<Value*>::const_iterator Attribute::potentialBegin() const
{
  return values.begin() + potentialIndex;
}

vector<Value*>::const_iterator Attribute::potentialEnd() const
{
  return values.begin() + irrelevantIndex;
}

vector<Value*>::const_iterator Attribute::irrelevantBegin() const
{
  return values.begin() + irrelevantIndex;
}

vector<Value*>::const_iterator Attribute::irrelevantEnd() const
{
  return values.begin() + absentIndex;
}

vector<Value*>::const_iterator Attribute::absentBegin() const
{
  return values.begin() + absentIndex;
}

vector<Value*>::const_iterator Attribute::absentEnd() const
{
  return values.end();
}
#endif

#ifdef DEBUG_HA
string Attribute::printLabelsById(unsigned int v1, unsigned int v2)
{
  return labelsVector[v1][v2];
}
#endif

const bool Attribute::lessNoisy(const Value* value, const Value* otherValue)
{
  return *value < *otherValue;
}

const bool Attribute::lessAppealingIrrelevant(const Attribute* attribute, const Attribute* otherAttribute)
{
  return (attribute->absentIndex - attribute->irrelevantIndex) * otherAttribute->values.size() < (otherAttribute->absentIndex - otherAttribute->irrelevantIndex) * attribute->values.size();
}

