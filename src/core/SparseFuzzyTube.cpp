// Copyright 2010,2011,2012,2013,2014,2015 Loïc Cerf (lcerf@dcc.ufmg.br)

// This file is part of multidupehack.

// multidupehack is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License version 3 as published by the Free Software Foundation

// multidupehack is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along with multidupehack; if not, write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

#include "SparseFuzzyTube.h"

float SparseFuzzyTube::densityThreshold;

SparseFuzzyTube::SparseFuzzyTube() : tube()
{
}

SparseFuzzyTube* SparseFuzzyTube::clone() const
{
  return new SparseFuzzyTube(*this);
}

void SparseFuzzyTube::print(vector<unsigned int>& prefix, ostream& out) const
{
  for (const pair<unsigned int, unsigned int>& hyperplane : tube)
    {
      for (const unsigned int id : prefix)
	{
	  out << id << ' ';
	}
      out << hyperplane.first << ' ' << 1 - static_cast<double>(hyperplane.second) / Attribute::noisePerUnit << endl;
    }
}

const bool SparseFuzzyTube::setTuple(const vector<unsigned int>& tuple, const unsigned int membership, vector<unsigned int>::const_iterator attributeIdIt, vector<vector<unsigned int>>::const_iterator oldIds2NewIdsIt, const vector<Attribute*>::iterator attributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts)
{
  const unsigned int element = oldIds2NewIdsIt->at(tuple[*attributeIdIt]);
  (*attributeIt)->substractPotentialNoise(element, membership);
  for (vector<vector<unsigned int>>::iterator intersectionIt : intersectionIts)
    {
      (*intersectionIt)[element] -= membership;
    }
  tube[element] = Attribute::noisePerUnit - membership;
  return tube.bucket_count() * sizeof(unsigned int) + 2 * tube.size() * sizeof(unsigned int*) > (*attributeIt)->sizeOfPresentAndPotential() * densityThreshold; // In the worst case (all values in th same bucket), the unordered_map<unsigned int, unsigned int> takes more space than a vector<unsigned int> * densityThreshold
}

const unsigned int SparseFuzzyTube::setSelfLoopsInSymmetricAttribute(const unsigned int hyperplaneId, const unsigned int lastSymmetricAttributeId, const vector<Attribute*>::iterator attributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts, const unsigned int dimensionId)
{
  // Necessarily symmetric
  for (vector<vector<unsigned int>>::iterator intersectionIt : intersectionIts)
    {
      (*intersectionIt)[hyperplaneId] -= Attribute::noisePerUnit;
    }
  (*attributeIt)->substractPotentialNoise(hyperplaneId, Attribute::noisePerUnit);
  tube[hyperplaneId] = 0;
  return Attribute::noisePerUnit;
}

void SparseFuzzyTube::setDensityThreshold(const float densityThresholdParam)
{
  densityThreshold = densityThresholdParam;
}

const unsigned int SparseFuzzyTube::noiseOnValue(const unsigned int valueDataId) const
{
  const unordered_map<unsigned int, unsigned int>::const_iterator noisyTupleIt = tube.find(valueDataId);
  if (noisyTupleIt == tube.end())
    {
      return Attribute::noisePerUnit;
    }
  return noisyTupleIt->second;
}

const unsigned int SparseFuzzyTube::noiseOnValues(const vector<Attribute*>::const_iterator attributeIt, const vector<unsigned int>& valueDataIds) const
{
  unsigned int oldNoise = 0;
  for (const unsigned int valueDataId : valueDataIds)
    {
      oldNoise += noiseOnValue(valueDataId);
    }
  return oldNoise;
}

const unsigned int SparseFuzzyTube::setPresent(const vector<Attribute*>::iterator presentAttributeIt, const vector<Attribute*>::iterator attributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const
{
  // *this necessarily relates to the present attribute
  return noiseOnValue((*attributeIt)->getChosenValue().getDataId());
}

const unsigned int SparseFuzzyTube::setPresentAfterPotentialOrAbsentUsed(const vector<Attribute*>::iterator presentAttributeIt, const vector<Attribute*>::iterator attributeIt, const vector<vector<unsigned int>>::iterator potentialOrAbsentValueIntersectionIt) const
{
  // *this necessarily relates to the present attribute
  const Value& presentValue = (*attributeIt)->getChosenValue();
  const unsigned int noise = noiseOnValue(presentValue.getDataId());
  (*potentialOrAbsentValueIntersectionIt)[presentValue.getIntersectionId()] += noise;
  return noise;
}

const unsigned int SparseFuzzyTube::presentFixPresentValuesAfterPresentValueMet(Attribute& currentAttribute) const
{
  unsigned int newNoise = 0;
  const vector<Value*>::iterator end = currentAttribute.presentEnd();
  for (vector<Value*>::iterator valueIt = currentAttribute.presentBegin(); valueIt != end; ++valueIt)
    {
      const unsigned int newNoiseInHyperplane = noiseOnValue((*valueIt)->getDataId());
      (*valueIt)->addPresentNoise(newNoiseInHyperplane);
      newNoise += newNoiseInHyperplane;
    }
  return newNoise;
}

const unsigned int SparseFuzzyTube::presentFixPresentValuesAfterPresentValueMetAndPotentialOrAbsentUsed(Attribute& currentAttribute, const vector<vector<unsigned int>>::iterator potentialOrAbsentValueIntersectionIt) const
{
  unsigned int newNoise = 0;
  const vector<Value*>::iterator end = currentAttribute.presentEnd();
  for (vector<Value*>::iterator valueIt = currentAttribute.presentBegin(); valueIt != end; ++valueIt)
    {
      const unsigned int newNoiseInHyperplane = noiseOnValue((*valueIt)->getDataId());
      (*potentialOrAbsentValueIntersectionIt)[(*valueIt)->getIntersectionId()] += newNoiseInHyperplane;
      newNoise += newNoiseInHyperplane;
    }
  return newNoise;
}

void SparseFuzzyTube::presentFixPotentialOrAbsentValuesAfterPresentValueMet(Attribute& currentAttribute, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const
{
  const vector<Value*>::iterator end = currentAttribute.absentEnd();
  for (vector<Value*>::iterator valueIt = currentAttribute.potentialBegin(); valueIt != end; ++valueIt)
    {
      const unsigned int newNoiseInHyperplane = noiseOnValue((*valueIt)->getDataId());
      (*valueIt)->addPresentNoise(newNoiseInHyperplane);
      const unsigned int valueId = (*valueIt)->getIntersectionId();
      for (vector<vector<unsigned int>>::iterator intersectionIt : intersectionIts)
	{
	  (*intersectionIt)[valueId] += newNoiseInHyperplane;
	}
    }
}

void SparseFuzzyTube::presentFixPotentialOrAbsentValuesInSecondSymmetricAttribute(Attribute& currentAttribute, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const
{
  const vector<Value*>::iterator end = currentAttribute.absentEnd();
  // The first potential value actually is the value set present and there is no noise to be found at the insection of a vertex (seen as an outgoing vertex) and itself (seen as an ingoing vertex)
  for (vector<Value*>::iterator valueIt = currentAttribute.potentialBegin(); ++valueIt != end; )
    {
      const unsigned int newNoiseInHyperplane = noiseOnValue((*valueIt)->getDataId());
      (*valueIt)->addPresentNoise(newNoiseInHyperplane);
      const unsigned int valueId = (*valueIt)->getIntersectionId();
      for (vector<vector<unsigned int>>::iterator intersectionIt : intersectionIts)
	{
	  (*intersectionIt)[valueId] += newNoiseInHyperplane;
	}
    }
}

const unsigned int SparseFuzzyTube::setAbsent(const vector<Attribute*>::iterator absentAttributeIt, const vector<unsigned int>& absentValueDataIds, const vector<Attribute*>::iterator attributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const
{
  // *this necessarily relates to the absent attribute
  return noiseOnValues(absentAttributeIt, absentValueDataIds);
}

const unsigned int SparseFuzzyTube::setAbsentAfterAbsentUsed(const vector<Attribute*>::iterator absentAttributeIt, const vector<unsigned int>& absentValueDataIds, const vector<Attribute*>::iterator attributeIt, const vector<vector<unsigned int>>::iterator absentValueIntersectionIt) const
{
  // *this necessarily relates to the absent attribute
  return noiseOnValues(absentAttributeIt, absentValueDataIds);
}

const unsigned int SparseFuzzyTube::absentFixPresentOrPotentialValuesAfterAbsentValuesMet(Attribute& currentAttribute, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const
{
  unsigned int oldNoise = 0;
  const vector<Value*>::iterator end = currentAttribute.irrelevantEnd();
  for (vector<Value*>::iterator valueIt = currentAttribute.presentBegin(); valueIt != end; ++valueIt)
    {
      const unsigned int oldNoiseInHyperplane = noiseOnValue((*valueIt)->getDataId());
      (*valueIt)->substractPotentialNoise(oldNoiseInHyperplane);
      const unsigned int valueId = (*valueIt)->getIntersectionId();
      for (vector<vector<unsigned int>>::iterator intersectionIt : intersectionIts)
	{
	  (*intersectionIt)[valueId] -= oldNoiseInHyperplane;
	}
      oldNoise += oldNoiseInHyperplane;
    }
  return oldNoise;
}

const unsigned int SparseFuzzyTube::absentFixPresentOrPotentialValuesInSecondSymmetricAttribute(Attribute& currentAttribute, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const
{
  unsigned int oldNoise = 0;
  vector<Value*>::iterator end = currentAttribute.presentEnd();
  vector<Value*>::iterator valueIt = currentAttribute.presentBegin();
  for (; valueIt != end; ++valueIt)
    {
      const unsigned int oldNoiseInHyperplane = noiseOnValue((*valueIt)->getDataId());
      (*valueIt)->substractPotentialNoise(oldNoiseInHyperplane);
      const unsigned int valueId = (*valueIt)->getIntersectionId();
      for (vector<vector<unsigned int>>::iterator intersectionIt : intersectionIts)
	{
	  (*intersectionIt)[valueId] -= oldNoiseInHyperplane;
	}
      oldNoise += oldNoiseInHyperplane;
    }
  end = currentAttribute.irrelevantEnd();
  while (++valueIt != end)
    {
      const unsigned int oldNoiseInHyperplane = noiseOnValue((*valueIt)->getDataId());
      (*valueIt)->substractPotentialNoise(oldNoiseInHyperplane);
      const unsigned int valueId = (*valueIt)->getIntersectionId();
      for (vector<vector<unsigned int>>::iterator intersectionIt : intersectionIts)
	{
	  (*intersectionIt)[valueId] -= oldNoiseInHyperplane;
	}
      oldNoise += oldNoiseInHyperplane;
    }
  return oldNoise;
}

const unsigned int SparseFuzzyTube::absentFixPresentOrPotentialValuesAfterAbsentValuesMetAndAbsentUsed(Attribute& currentAttribute, const vector<vector<unsigned int>>::iterator absentValueIntersectionIt) const
{
  unsigned int oldNoise = 0;
  const vector<Value*>::iterator end = currentAttribute.irrelevantEnd();
  for (vector<Value*>::iterator valueIt = currentAttribute.presentBegin(); valueIt != end; ++valueIt)
    {
      const unsigned int oldNoiseInHyperplane = noiseOnValue((*valueIt)->getDataId());
      (*absentValueIntersectionIt)[(*valueIt)->getIntersectionId()] -= oldNoiseInHyperplane;
      oldNoise += oldNoiseInHyperplane;
    }
  return oldNoise;
}

const unsigned int SparseFuzzyTube::absentFixPresentOrPotentialValuesInSecondSymmetricAttributeAfterAbsentUsed(Attribute& currentAttribute, const vector<vector<unsigned int>>::iterator absentValueIntersectionIt) const
{
  unsigned int oldNoise = 0;
  vector<Value*>::iterator end = currentAttribute.presentEnd();
  vector<Value*>::iterator valueIt = currentAttribute.presentBegin();
  for (; valueIt != end; ++valueIt)
    {
      const unsigned int oldNoiseInHyperplane = noiseOnValue((*valueIt)->getDataId());
      (*absentValueIntersectionIt)[(*valueIt)->getIntersectionId()] -= oldNoiseInHyperplane;
      oldNoise += oldNoiseInHyperplane;
    }
  end = currentAttribute.irrelevantEnd();
  while (++valueIt != end)
    {
      const unsigned int oldNoiseInHyperplane = noiseOnValue((*valueIt)->getDataId());
      (*absentValueIntersectionIt)[(*valueIt)->getIntersectionId()] -= oldNoiseInHyperplane;
      oldNoise += oldNoiseInHyperplane;
    }
  return oldNoise;
}
  
void SparseFuzzyTube::absentFixAbsentValuesAfterAbsentValuesMet(Attribute& currentAttribute, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const
{
  const vector<Value*>::iterator end = currentAttribute.absentEnd();
  for (vector<Value*>::iterator valueIt = currentAttribute.absentBegin(); valueIt != end; ++valueIt)
    {
      const unsigned int oldNoiseInHyperplane = noiseOnValue((*valueIt)->getDataId());
      (*valueIt)->substractPotentialNoise(oldNoiseInHyperplane);
      const unsigned int valueId = (*valueIt)->getIntersectionId();
      for (vector<vector<unsigned int>>::iterator intersectionIt : intersectionIts)
	{
	  (*intersectionIt)[valueId] -= oldNoiseInHyperplane;
	}
    }
}

const unsigned int SparseFuzzyTube::countNoise(const vector<vector<unsigned int>>::const_iterator dimensionIt) const
{
  unsigned int noise = 0;
  for (const unsigned int id : *dimensionIt)
    {
      noise += noiseOnValue(id);
    }
  return noise;
}

const bool SparseFuzzyTube::decreaseMembershipDownToThreshold(const double membershipThreshold, const vector<vector<unsigned int>>::const_iterator dimensionIt, const vector<vector<unsigned int>::const_iterator>::iterator tupleIt, double& membershipSum) const
{
  for (; *tupleIt != dimensionIt->end(); ++*tupleIt)
    {
      membershipSum -= noiseOnValue((*tupleIt)->getId());
      if (membershipSum < membershipThreshold)
	{
	  ++*tupleIt;
	  return true;
	}
    }
  *tupleIt = dimensionIt->begin();
  return false;
}

#ifdef ASSERT
const unsigned int SparseFuzzyTube::countNoiseOnPresent(const vector<Attribute*>::const_iterator valueAttributeIt, const Value& value, const vector<Attribute*>::const_iterator attributeIt) const
{
  if (attributeIt == valueAttributeIt)
    {
      return noiseOnValue(value.getDataId());
    }
  unsigned int noise = 0;
  const vector<Value*>::const_iterator end = (*attributeIt)->presentEnd();
  for (vector<Value*>::const_iterator valueIt = (*attributeIt)->presentBegin(); valueIt != end; ++valueIt)
    {
      noise += noiseOnValue((*valueIt)->getDataId());
    }
  return noise;
}

const unsigned int SparseFuzzyTube::countNoiseOnPresentAndPotential(const vector<Attribute*>::const_iterator valueAttributeIt, const Value& value, const vector<Attribute*>::const_iterator attributeIt) const
{
  if (attributeIt == valueAttributeIt)
    {
      return noiseOnValue(value.getDataId());
    }
  unsigned int noise = 0;
  vector<Value*>::const_iterator end = (*attributeIt)->irrelevantEnd();
  for (vector<Value*>::const_iterator valueIt = (*attributeIt)->presentBegin(); valueIt != end; ++valueIt)
    {
      noise += noiseOnValue((*valueIt)->getDataId());
    }
  return noise;
}
#endif
