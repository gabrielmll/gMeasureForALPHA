// Copyright 2010,2011,2012,2013,2014,2015 Loïc Cerf (lcerf@dcc.ufmg.br)

// This file is part of multidupehack.

// multidupehack is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License version 3 as published by the Free Software Foundation

// multidupehack is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along with multidupehack; if not, write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

#include "DenseCrispTube.h"

DenseCrispTube::DenseCrispTube(const SparseCrispTube& sparseCrispTube, const unsigned int nbOfHyperplanes) : tube()
{
  tube.resize(nbOfHyperplanes, true);
  const unordered_set<unsigned int>& sparseTube = sparseCrispTube.getTube();
  for (const unsigned int presentHyperplaneId : sparseTube)
    {
      tube.set(presentHyperplaneId, false);
    }
}

DenseCrispTube* DenseCrispTube::clone() const
{
  return new DenseCrispTube(*this);
}

void DenseCrispTube::print(vector<unsigned int>& prefix, ostream& out) const
{
  unsigned int hyperplaneId = 0;
  for (dynamic_bitset<>::size_type absentHyperplaneId = tube.find_first(); absentHyperplaneId != dynamic_bitset<>::npos; absentHyperplaneId = tube.find_next(absentHyperplaneId))
    {
      for (; hyperplaneId != absentHyperplaneId; ++hyperplaneId)
	{
	  for (const unsigned int id : prefix)
	    {
	      out << id << ' ';
	    }
	  out << hyperplaneId << " 1" << endl;	  
	}
      ++hyperplaneId;
    }
}

const bool DenseCrispTube::setTuple(const vector<unsigned int>& tuple, const unsigned int membership, vector<unsigned int>::const_iterator attributeIdIt, vector<vector<unsigned int>>::const_iterator oldIds2NewIdsIt, const vector<Attribute*>::iterator attributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts)
{
  const unsigned int element = oldIds2NewIdsIt->at(tuple[*attributeIdIt]);
  (*attributeIt)->substractPotentialNoise(element, Attribute::noisePerUnit);
  for (vector<vector<unsigned int>>::iterator intersectionIt : intersectionIts)
    {
      (*intersectionIt)[element] -= Attribute::noisePerUnit;
    }
  tube.set(element, false);
  return false;
}

const unsigned int DenseCrispTube::setSelfLoopsInSymmetricAttribute(const unsigned int hyperplaneId, const unsigned int lastSymmetricAttributeId, const vector<Attribute*>::iterator attributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts, const unsigned int dimensionId)
{
  // Never called
  return 0;
}

const unsigned int DenseCrispTube::noiseOnValues(const vector<Attribute*>::const_iterator attributeIt, const vector<unsigned int>& valueDataIds) const
{
  unsigned int oldNoise = 0;
  for (const unsigned int valueDataId : valueDataIds)
    {
      if (tube[valueDataId])
	{
	  oldNoise += Attribute::noisePerUnit;
	}
    }
  return oldNoise;
}

const unsigned int DenseCrispTube::setPresent(const vector<Attribute*>::iterator presentAttributeIt, const vector<Attribute*>::iterator attributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const
{
  // *this necessarily relates to the present attribute
  if (tube[(*attributeIt)->getChosenValue().getDataId()])
    {
      return Attribute::noisePerUnit;
    }
  return 0;
}

const unsigned int DenseCrispTube::setPresentAfterPotentialOrAbsentUsed(const vector<Attribute*>::iterator presentAttributeIt, const vector<Attribute*>::iterator attributeIt, const vector<vector<unsigned int>>::iterator potentialOrAbsentValueIntersectionIt) const
{
  // *this necessarily relates to the present attribute
  const Value& presentValue = (*attributeIt)->getChosenValue();
  if (tube[presentValue.getDataId()])
    {
      (*potentialOrAbsentValueIntersectionIt)[presentValue.getIntersectionId()] += Attribute::noisePerUnit;
      return Attribute::noisePerUnit;
    }
  return 0;
}

const unsigned int DenseCrispTube::setAbsent(const vector<Attribute*>::iterator absentAttributeIt, const vector<unsigned int>& absentValueDataIds, const vector<Attribute*>::iterator attributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const
{
  // *this necessarily relates to the absent attribute
  return noiseOnValues(absentAttributeIt, absentValueDataIds);
}

const unsigned int DenseCrispTube::setAbsentAfterAbsentUsed(const vector<Attribute*>::iterator absentAttributeIt, const vector<unsigned int>& absentValueDataIds, const vector<Attribute*>::iterator attributeIt, const vector<vector<unsigned int>>::iterator absentValueIntersectionIt) const
{
  // *this necessarily relates to the absent attribute
  return noiseOnValues(absentAttributeIt, absentValueDataIds);
}

const unsigned int DenseCrispTube::presentFixPresentValuesAfterPresentValueMet(Attribute& currentAttribute) const
{
  unsigned int newNoise = 0;
  const vector<Value*>::iterator end = currentAttribute.presentEnd();
  for (vector<Value*>::iterator valueIt = currentAttribute.presentBegin(); valueIt != end; ++valueIt)
    {
      if (tube[(*valueIt)->getDataId()])
	{
	  (*valueIt)->addPresentNoise(Attribute::noisePerUnit);
	  newNoise += Attribute::noisePerUnit;
	}
    }
  return newNoise;
}

const unsigned int DenseCrispTube::presentFixPresentValuesAfterPresentValueMetAndPotentialOrAbsentUsed(Attribute& currentAttribute, const vector<vector<unsigned int>>::iterator potentialOrAbsentValueIntersectionIt) const
{
  unsigned int newNoise = 0;
  const vector<Value*>::iterator end = currentAttribute.presentEnd();
  for (vector<Value*>::iterator valueIt = currentAttribute.presentBegin(); valueIt != end; ++valueIt)
    {
      if (tube[(*valueIt)->getDataId()])
	{
	  (*potentialOrAbsentValueIntersectionIt)[(*valueIt)->getIntersectionId()] += Attribute::noisePerUnit;
	  newNoise += Attribute::noisePerUnit;
	}
    }
  return newNoise;
}

void DenseCrispTube::presentFixPotentialOrAbsentValuesAfterPresentValueMet(Attribute& currentAttribute, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const
{
  const vector<Value*>::iterator end = currentAttribute.absentEnd();
  for (vector<Value*>::iterator valueIt = currentAttribute.potentialBegin(); valueIt != end; ++valueIt)
    {
      if (tube[(*valueIt)->getDataId()])
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

void DenseCrispTube::presentFixPotentialOrAbsentValuesInSecondSymmetricAttribute(Attribute& currentAttribute, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const
{
  const vector<Value*>::iterator end = currentAttribute.absentEnd();
  // The first potential value actually is the value set present and there is no noise to be found at the insection of a vertex (seen as an outgoing vertex) and itself (seen as an ingoing vertex)
  for (vector<Value*>::iterator valueIt = currentAttribute.potentialBegin(); ++valueIt != end; )
    {
      if (tube[(*valueIt)->getDataId()])
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

const unsigned int DenseCrispTube::absentFixPresentOrPotentialValuesAfterAbsentValuesMet(Attribute& currentAttribute, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const
{
  unsigned int oldNoise = 0;
  const vector<Value*>::iterator end = currentAttribute.irrelevantEnd();
  for (vector<Value*>::iterator valueIt = currentAttribute.presentBegin(); valueIt != end; ++valueIt)
    {
      if (tube[(*valueIt)->getDataId()])
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

const unsigned int DenseCrispTube::absentFixPresentOrPotentialValuesInSecondSymmetricAttribute(Attribute& currentAttribute, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const
{
  unsigned int oldNoise = 0;
  vector<Value*>::iterator end = currentAttribute.presentEnd();
  vector<Value*>::iterator valueIt = currentAttribute.presentBegin();
  for (; valueIt != end; ++valueIt)
    {
      if (tube[(*valueIt)->getDataId()])
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
  // The first potential value actually is the value set absent and there is no noise to be found at the intersection of a vertex (seen as an outgoing vertex) and itself (seen as an ingoing vertex)
  while (++valueIt != end)
    {
      if (tube[(*valueIt)->getDataId()])
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

const unsigned int DenseCrispTube::absentFixPresentOrPotentialValuesAfterAbsentValuesMetAndAbsentUsed(Attribute& currentAttribute, const vector<vector<unsigned int>>::iterator absentValueIntersectionIt) const
{
  unsigned int oldNoise = 0;
  const vector<Value*>::iterator end = currentAttribute.irrelevantEnd();
  for (vector<Value*>::iterator valueIt = currentAttribute.presentBegin(); valueIt != end; ++valueIt)
    {
      if (tube[(*valueIt)->getDataId()])
	{
	  (*absentValueIntersectionIt)[(*valueIt)->getIntersectionId()] -= Attribute::noisePerUnit;
	  oldNoise += Attribute::noisePerUnit;
	}
    }
  return oldNoise;
}

const unsigned int DenseCrispTube::absentFixPresentOrPotentialValuesInSecondSymmetricAttributeAfterAbsentUsed(Attribute& currentAttribute, const vector<vector<unsigned int>>::iterator absentValueIntersectionIt) const
{
  unsigned int oldNoise = 0;
  vector<Value*>::iterator end = currentAttribute.presentEnd();
  vector<Value*>::iterator valueIt = currentAttribute.presentBegin();
  for (; valueIt != end; ++valueIt)
    {
      if (tube[(*valueIt)->getDataId()])
	{
	  (*absentValueIntersectionIt)[(*valueIt)->getIntersectionId()] -= Attribute::noisePerUnit;
	  oldNoise += Attribute::noisePerUnit;
	}
    }
  end = currentAttribute.irrelevantEnd();
  while (++valueIt != end)
    {
      if (tube[(*valueIt)->getDataId()])
	{
	  (*absentValueIntersectionIt)[(*valueIt)->getIntersectionId()] -= Attribute::noisePerUnit;
	  oldNoise += Attribute::noisePerUnit;
	}
    }
  return oldNoise;
}

void DenseCrispTube::absentFixAbsentValuesAfterAbsentValuesMet(Attribute& currentAttribute, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const
{
  const vector<Value*>::iterator end = currentAttribute.absentEnd();
  for (vector<Value*>::iterator valueIt = currentAttribute.absentBegin(); valueIt != end; ++valueIt)
    {
      if (tube[(*valueIt)->getDataId()])
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

const unsigned int DenseCrispTube::countNoise(const vector<vector<unsigned int>>::const_iterator dimensionIt) const
{
  unsigned int noise = 0;
  for (const unsigned int id : *dimensionIt)
    {
      if (tube[id])
	{
	  noise += Attribute::noisePerUnit;
	}
    }
  return noise;
}

const bool DenseCrispTube::decreaseMembershipDownToThreshold(const double membershipThreshold, const vector<vector<unsigned int>>::const_iterator dimensionIt, const vector<vector<unsigned int>::const_iterator>::iterator tupleIt, double& membershipSum) const
{
  for (; *tupleIt != dimensionIt->end(); ++*tupleIt)
    {
      if (tube[**tupleIt])
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
const unsigned int DenseCrispTube::countNoiseOnPresent(const vector<Attribute*>::const_iterator valueAttributeIt, const Value& value, const vector<Attribute*>::const_iterator attributeIt) const
{
  if (attributeIt == valueAttributeIt)
    {
      if (tube[value.getDataId()])
	{
	  return Attribute::noisePerUnit;
	}
      return 0;
    }
  unsigned int noise = 0;
  const vector<Value*>::const_iterator end = (*attributeIt)->presentEnd();
  for (vector<Value*>::const_iterator valueIt = (*attributeIt)->presentBegin(); valueIt != end; ++valueIt)
    {
      if (tube[(*valueIt)->getDataId()])
	{
	  noise += Attribute::noisePerUnit;
	}
    }
  return noise;
}

const unsigned int DenseCrispTube::countNoiseOnPresentAndPotential(const vector<Attribute*>::const_iterator valueAttributeIt, const Value& value, const vector<Attribute*>::const_iterator attributeIt) const
{
  if (attributeIt == valueAttributeIt)
    {
      if (tube[value.getDataId()])
	{
	  return Attribute::noisePerUnit;
	}
      return 0;
    }
  unsigned int noise = 0;
  const vector<Value*>::const_iterator end = (*attributeIt)->irrelevantEnd();
  for (vector<Value*>::const_iterator valueIt = (*attributeIt)->presentBegin(); valueIt != end; ++valueIt)
    {
      if (tube[(*valueIt)->getDataId()])
	{
	  noise += Attribute::noisePerUnit;
	}
    }
  return noise;
}
#endif
