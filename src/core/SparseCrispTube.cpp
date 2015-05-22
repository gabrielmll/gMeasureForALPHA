// Copyright 2010,2011,2012,2013,2014,2015 Loïc Cerf (lcerf@dcc.ufmg.br)

// This file is part of multidupehack.

// multidupehack is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License version 3 as published by the Free Software Foundation

// multidupehack is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along with multidupehack; if not, write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

#include "SparseCrispTube.h"

float SparseCrispTube::densityThreshold;

SparseCrispTube::SparseCrispTube() : tube()
{
}

SparseCrispTube* SparseCrispTube::clone() const
{
  return new SparseCrispTube(*this);
}

void SparseCrispTube::print(vector<unsigned int>& prefix, ostream& out) const
{
  for (const unsigned int hyperplane : tube)
    {
      for (const unsigned int id : prefix)
	{
	  out << id << ' ';
	}
      out << hyperplane << " 1" << endl;
    }
}

const bool SparseCrispTube::setTuple(const vector<unsigned int>& tuple, const unsigned int membership, vector<unsigned int>::const_iterator attributeIdIt, vector<vector<unsigned int>>::const_iterator oldIds2NewIdsIt, const vector<Attribute*>::iterator attributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts)
{
  const unsigned int element = oldIds2NewIdsIt->at(tuple[*attributeIdIt]);
  (*attributeIt)->substractPotentialNoise(element, Attribute::noisePerUnit);
  for (vector<vector<unsigned int>>::iterator intersectionIt : intersectionIts)
    {
      (*intersectionIt)[element] -= Attribute::noisePerUnit;
    }
  tube.insert(element);
  return tube.bucket_count() + 2 * tube.size() * sizeof(unsigned int*) > (*attributeIt)->sizeOfPresentAndPotential() * densityThreshold; // In the worst case (all values in th same bucket), the unordered_set<unsigned int> takes more space than a vector<unsigned int> * densityThreshold
}

const unsigned int SparseCrispTube::setSelfLoopsInSymmetricAttribute(const unsigned int hyperplaneId, const unsigned int lastSymmetricAttributeId, const vector<Attribute*>::iterator attributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts, const unsigned int dimensionId)
{
  // Necessarily symmetric
  for (vector<vector<unsigned int>>::iterator intersectionIt : intersectionIts)
    {
      (*intersectionIt)[hyperplaneId] -= Attribute::noisePerUnit;
    }
  (*attributeIt)->substractPotentialNoise(hyperplaneId, Attribute::noisePerUnit);
  tube.insert(hyperplaneId);
  return Attribute::noisePerUnit;
}

const unordered_set<unsigned int>& SparseCrispTube::getTube() const
{
  return tube;
}

void SparseCrispTube::setDensityThreshold(const float densityThresholdParam)
{
  densityThreshold = densityThresholdParam / 8;
}

const unsigned int SparseCrispTube::noiseOnValues(const vector<Attribute*>::const_iterator attributeIt, const vector<unsigned int>& valueDataIds) const
{
  unsigned int oldNoise = 0;
  for (const unsigned int valueDataId : valueDataIds)
    {
      if (tube.find(valueDataId) == tube.end())
	{
	  oldNoise += Attribute::noisePerUnit;
	}
    }
  return oldNoise;
}

const unsigned int SparseCrispTube::setPresent(const vector<Attribute*>::iterator presentAttributeIt, const vector<Attribute*>::iterator attributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const
{
  // *this necessarily relates to the present attribute
  if (tube.find((*attributeIt)->getChosenValue().getDataId()) == tube.end())
    {
      return Attribute::noisePerUnit;
    }
  return 0;
}

const unsigned int SparseCrispTube::setPresentAfterPotentialOrAbsentUsed(const vector<Attribute*>::iterator presentAttributeIt, const vector<Attribute*>::iterator attributeIt, const vector<vector<unsigned int>>::iterator potentialOrAbsentValueIntersectionIt) const
{
  // *this necessarily relates to the present attribute
  const Value& presentValue = (*attributeIt)->getChosenValue();
  if (tube.find(presentValue.getDataId()) == tube.end())
    {
      (*potentialOrAbsentValueIntersectionIt)[presentValue.getIntersectionId()] += Attribute::noisePerUnit;
      return Attribute::noisePerUnit;
    }
  return 0;
}

const unsigned int SparseCrispTube::setAbsent(const vector<Attribute*>::iterator absentAttributeIt, const vector<unsigned int>& absentValueDataIds, const vector<Attribute*>::iterator attributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const
{
  // *this necessarily relates to the absent attribute
  return noiseOnValues(absentAttributeIt, absentValueDataIds);
}

const unsigned int SparseCrispTube::setAbsentAfterAbsentUsed(const vector<Attribute*>::iterator absentAttributeIt, const vector<unsigned int>& absentValueDataIds, const vector<Attribute*>::iterator attributeIt, const vector<vector<unsigned int>>::iterator absentValueIntersectionIt) const
{
  // *this necessarily relates to the absent attribute
  return noiseOnValues(absentAttributeIt, absentValueDataIds);
}

const unsigned int SparseCrispTube::presentFixPresentValuesAfterPresentValueMet(Attribute& currentAttribute) const
{
  unsigned int newNoise = 0;
  const vector<Value*>::iterator end = currentAttribute.presentEnd();
  for (vector<Value*>::iterator valueIt = currentAttribute.presentBegin(); valueIt != end; ++valueIt)
    {
      if (tube.find((*valueIt)->getDataId()) == tube.end())
	{
	  (*valueIt)->addPresentNoise(Attribute::noisePerUnit);
	  newNoise += Attribute::noisePerUnit;
	}
    }
  return newNoise;
}

const unsigned int SparseCrispTube::presentFixPresentValuesAfterPresentValueMetAndPotentialOrAbsentUsed(Attribute& currentAttribute, const vector<vector<unsigned int>>::iterator potentialOrAbsentValueIntersectionIt) const
{
  unsigned int newNoise = 0;
  const vector<Value*>::iterator end = currentAttribute.presentEnd();
  for (vector<Value*>::iterator valueIt = currentAttribute.presentBegin(); valueIt != end; ++valueIt)
    {
      if (tube.find((*valueIt)->getDataId()) == tube.end())
	{
	  (*potentialOrAbsentValueIntersectionIt)[(*valueIt)->getIntersectionId()] += Attribute::noisePerUnit;
	  newNoise += Attribute::noisePerUnit;
	}
    }
  return newNoise;
}

void SparseCrispTube::presentFixPotentialOrAbsentValuesAfterPresentValueMet(Attribute& currentAttribute, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const
{
  const vector<Value*>::iterator end = currentAttribute.absentEnd();
  for (vector<Value*>::iterator valueIt = currentAttribute.potentialBegin(); valueIt != end; ++valueIt)
    {
      if (tube.find((*valueIt)->getDataId()) == tube.end())
	{
	  (*valueIt)->addPresentNoise(Attribute::noisePerUnit);
	  const unsigned int valueId = (*valueIt)->getIntersectionId();
	  for (vector<vector<unsigned int>>::iterator intersectionIt : intersectionIts)
	    {
	      (*intersectionIt)[valueId] += Attribute::noisePerUnit;
	    }
	}
    }
}

void SparseCrispTube::presentFixPotentialOrAbsentValuesInSecondSymmetricAttribute(Attribute& currentAttribute, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const
{
  const vector<Value*>::iterator end = currentAttribute.absentEnd();
  // The first potential value actually is the value set present and there is no noise to be found at the insection of a vertex (seen as an outgoing vertex) and itself (seen as an ingoing vertex)
  for (vector<Value*>::iterator valueIt = currentAttribute.potentialBegin(); ++valueIt != end; )
    {
      if (tube.find((*valueIt)->getDataId()) == tube.end())
	{
	  (*valueIt)->addPresentNoise(Attribute::noisePerUnit);
	  const unsigned int valueId = (*valueIt)->getIntersectionId();
	  for (vector<vector<unsigned int>>::iterator intersectionIt : intersectionIts)
	    {
	      (*intersectionIt)[valueId] += Attribute::noisePerUnit;
	    }
	}
    }
}

const unsigned int SparseCrispTube::absentFixPresentOrPotentialValuesAfterAbsentValuesMet(Attribute& currentAttribute, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const
{
  unsigned int oldNoise = 0;
  const vector<Value*>::iterator end = currentAttribute.irrelevantEnd();
  for (vector<Value*>::iterator valueIt = currentAttribute.presentBegin(); valueIt != end; ++valueIt)
    {
      if (tube.find((*valueIt)->getDataId()) == tube.end())
	{
	  (*valueIt)->substractPotentialNoise(Attribute::noisePerUnit);
	  const unsigned int valueId = (*valueIt)->getIntersectionId();
	  for (vector<vector<unsigned int>>::iterator intersectionIt : intersectionIts)
	    {
	      (*intersectionIt)[valueId] -= Attribute::noisePerUnit;
	    }
	  oldNoise += Attribute::noisePerUnit;
	}
    }
  return oldNoise;
}

const unsigned int SparseCrispTube::absentFixPresentOrPotentialValuesInSecondSymmetricAttribute(Attribute& currentAttribute, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const
{
  unsigned int oldNoise = 0;
  vector<Value*>::iterator end = currentAttribute.presentEnd();
  vector<Value*>::iterator valueIt = currentAttribute.presentBegin();
  for (; valueIt != end; ++valueIt)
    {
      if (tube.find((*valueIt)->getDataId()) == tube.end())
	{
	  (*valueIt)->substractPotentialNoise(Attribute::noisePerUnit);
	  const unsigned int valueId = (*valueIt)->getIntersectionId();
	  for (vector<vector<unsigned int>>::iterator intersectionIt : intersectionIts)
	    {
	      (*intersectionIt)[valueId] -= Attribute::noisePerUnit;
	    }
	  oldNoise += Attribute::noisePerUnit;
	}
    }
  end = currentAttribute.irrelevantEnd();
  while (++valueIt != end)
    {
      if (tube.find((*valueIt)->getDataId()) == tube.end())
	{
	  (*valueIt)->substractPotentialNoise(Attribute::noisePerUnit);
	  const unsigned int valueId = (*valueIt)->getIntersectionId();
	  for (vector<vector<unsigned int>>::iterator intersectionIt : intersectionIts)
	    {
	      (*intersectionIt)[valueId] -= Attribute::noisePerUnit;
	    }
	  oldNoise += Attribute::noisePerUnit;
	}
    }
  return oldNoise;
}

const unsigned int SparseCrispTube::absentFixPresentOrPotentialValuesAfterAbsentValuesMetAndAbsentUsed(Attribute& currentAttribute, const vector<vector<unsigned int>>::iterator absentValueIntersectionIt) const
{
  unsigned int oldNoise = 0;
  const vector<Value*>::iterator end = currentAttribute.irrelevantEnd();
  for (vector<Value*>::iterator valueIt = currentAttribute.presentBegin(); valueIt != end; ++valueIt)
    {
      if (tube.find((*valueIt)->getDataId()) == tube.end())
	{
	  (*absentValueIntersectionIt)[(*valueIt)->getIntersectionId()] -= Attribute::noisePerUnit;
	  oldNoise += Attribute::noisePerUnit;
	}
    }
  return oldNoise;
}

const unsigned int SparseCrispTube::absentFixPresentOrPotentialValuesInSecondSymmetricAttributeAfterAbsentUsed(Attribute& currentAttribute, const vector<vector<unsigned int>>::iterator absentValueIntersectionIt) const
{
  unsigned int oldNoise = 0;
  vector<Value*>::iterator end = currentAttribute.presentEnd();
  vector<Value*>::iterator valueIt = currentAttribute.presentBegin();
  for (; valueIt != end; ++valueIt)
    {
      if (tube.find((*valueIt)->getDataId()) == tube.end())
	{
	  (*absentValueIntersectionIt)[(*valueIt)->getIntersectionId()] -= Attribute::noisePerUnit;
	  oldNoise += Attribute::noisePerUnit;
	}
    }
  end = currentAttribute.irrelevantEnd();
  while (++valueIt != end)
    {
      if (tube.find((*valueIt)->getDataId()) == tube.end())
	{
	  (*absentValueIntersectionIt)[(*valueIt)->getIntersectionId()] -= Attribute::noisePerUnit;
	  oldNoise += Attribute::noisePerUnit;
	}
    }
  return oldNoise;
}

void SparseCrispTube::absentFixAbsentValuesAfterAbsentValuesMet(Attribute& currentAttribute, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const
{
  const vector<Value*>::iterator end = currentAttribute.absentEnd();
  for (vector<Value*>::iterator valueIt = currentAttribute.absentBegin(); valueIt != end; ++valueIt)
    {
      if (tube.find((*valueIt)->getDataId()) == tube.end())
	{
	  (*valueIt)->substractPotentialNoise(Attribute::noisePerUnit);
	  const unsigned int valueId = (*valueIt)->getIntersectionId();
	  for (vector<vector<unsigned int>>::iterator intersectionIt : intersectionIts)
	    {
	      (*intersectionIt)[valueId] -= Attribute::noisePerUnit;
	    }
	}
    }
}

const unsigned int SparseCrispTube::countNoise(const vector<vector<unsigned int>>::const_iterator dimensionIt) const
{
  unsigned int noise = 0;
  for (const unsigned int id : *dimensionIt)
    {
      if (tube.find(id) == tube.end())
	{
	  noise += Attribute::noisePerUnit;
	}
    }
  return noise;
}

const bool SparseCrispTube::decreaseMembershipDownToThreshold(const double membershipThreshold, const vector<vector<unsigned int>>::const_iterator dimensionIt, const vector<vector<unsigned int>::const_iterator>::iterator tupleIt, double& membershipSum) const
{
  for (; *tupleIt != dimensionIt->end(); ++*tupleIt)
    {
      if (tube.find(**tupleIt) == tube.end())
	{
	  membershipSum -= Attribute::noisePerUnit;
	  if (membershipSum < membershipThreshold)
	    {
	      ++*tupleIt;
	      return true;
	    }
	}
    }
  *tupleIt = dimensionIt->begin();
  return false;
}

#ifdef ASSERT
const unsigned int SparseCrispTube::countNoiseOnPresent(const vector<Attribute*>::const_iterator valueAttributeIt, const Value& value, const vector<Attribute*>::const_iterator attributeIt) const
{
  if (attributeIt == valueAttributeIt)
    {
      if (tube.find(value.getDataId()) == tube.end())
	{
	  return Attribute::noisePerUnit;
	}
      return 0;
    }
  unsigned int noise = 0;
  const vector<Value*>::const_iterator end = (*attributeIt)->presentEnd();
  for (vector<Value*>::const_iterator valueIt = (*attributeIt)->presentBegin(); valueIt != end; ++valueIt)
    {
      if (tube.find((*valueIt)->getDataId()) == tube.end())
	{
	  noise += Attribute::noisePerUnit;
	}
    }
  return noise;
}

const unsigned int SparseCrispTube::countNoiseOnPresentAndPotential(const vector<Attribute*>::const_iterator valueAttributeIt, const Value& value, const vector<Attribute*>::const_iterator attributeIt) const
{
  if (attributeIt == valueAttributeIt)
    {
      if (tube.find(value.getDataId()) == tube.end())
	{
	  return Attribute::noisePerUnit;
	}
      return 0;
    }
  unsigned int noise = 0;
  vector<Value*>::const_iterator end = (*attributeIt)->irrelevantEnd();
  for (vector<Value*>::const_iterator valueIt = (*attributeIt)->presentBegin(); valueIt != end; ++valueIt)
    {
      if (tube.find((*valueIt)->getDataId()) == tube.end())
	{
	  noise += Attribute::noisePerUnit;
	}
    }
  return noise;
}
#endif
