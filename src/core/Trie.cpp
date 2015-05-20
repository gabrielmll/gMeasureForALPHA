// Copyright 2007,2008,2009,2010,2011,2012,2013,2014,2015 Loïc (lcerf@dcc.ufmg.br)

// This file is part of multidupehack.

// multidupehack is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License version 3 as published by the Free Software Foundation

// multidupehack is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along with multidupehack; if not, write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

#include "Trie.h"

bool Trie::isCrisp = false;
NoNoiseTube Trie::noNoiseTube = NoNoiseTube();

Trie::Trie(const Trie& otherTrie): hyperplanes()
{
  copy(otherTrie);
}

Trie::Trie(Trie&& otherTrie): hyperplanes(std::move(otherTrie.hyperplanes))
{
}

Trie::Trie(const vector<unsigned int>::const_iterator cardinalityIt, const vector<unsigned int>::const_iterator cardinalityEnd): hyperplanes()
{
  const unsigned int cardinality = *cardinalityIt;
  hyperplanes.reserve(cardinality);
  if (cardinalityIt + 2 == cardinalityEnd)
    {
      if (isCrisp)
	{
	  for (unsigned int hyperplaneId = 0; hyperplaneId != cardinality; ++hyperplaneId)
	    {
	      hyperplanes.push_back(new SparseCrispTube());
	    }
	}
      else
	{
	  for (unsigned int hyperplaneId = 0; hyperplaneId != cardinality; ++hyperplaneId)
	    {
	      hyperplanes.push_back(new SparseFuzzyTube());
	    }
	}
    }
  else
    {
      const vector<unsigned int>::const_iterator nbOfElementsInNextDiIt = cardinalityIt + 1;
      for (unsigned int hyperplaneId = 0; hyperplaneId != cardinality; ++hyperplaneId)
	{
	  hyperplanes.push_back(new Trie(nbOfElementsInNextDiIt, cardinalityEnd));
	}
    }
}

Trie::~Trie()
{
  for (AbstractData* hyperplane : hyperplanes)
    {
      if (hyperplane != &noNoiseTube)
	{
	  delete hyperplane;
	}
    }
}

Trie* Trie::clone() const
{
  return new Trie(*this);
}

Trie& Trie::operator=(const Trie& otherTrie)
{
  copy(otherTrie);
  return *this;
}

Trie& Trie::operator=(Trie&& otherTrie)
{
  hyperplanes = std::move(otherTrie.hyperplanes);
  return *this;
}

ostream& operator<<(ostream& out, const Trie& trie)
{
  vector<unsigned int> prefix;
  prefix.reserve(trie.depth());
  trie.print(prefix, out);
  return out;
}

void Trie::copy(const Trie& otherTrie)
{
  hyperplanes.reserve(otherTrie.hyperplanes.size());
  for (const AbstractData* hyperplane : otherTrie.hyperplanes)
    {
      hyperplanes.push_back(hyperplane->clone());
    }
}

const unsigned int Trie::depth() const
{
  if (hyperplanes.empty())
    {
      return 0;
    }
  return 1 + hyperplanes.front()->depth();
}

void Trie::print(vector<unsigned int>& prefix, ostream& out) const
{
  unsigned int hyperplaneId = 0;
  for (AbstractData* hyperplane : hyperplanes)
    {
      prefix.push_back(hyperplaneId++);
      hyperplane->print(prefix, out);
      prefix.pop_back();
    }
}

void Trie::setHyperplane(const unsigned int hyperplaneOldId, const unordered_map<vector<unsigned int>, double, vector_hash<unsigned int>>::const_iterator begin, const unordered_map<vector<unsigned int>, double, vector_hash<unsigned int>>::const_iterator end, const vector<unsigned int>& attributeOrder, const vector<vector<unsigned int>>& oldIds2NewIds, vector<Attribute*>& attributes)
{
  const vector<Attribute*>::iterator nextAttributeIt = attributes.begin() + 1;
  const unsigned int hyperplaneId = oldIds2NewIds.front().at(hyperplaneOldId);
  AbstractData* hyperplane = hyperplanes[hyperplaneId];
  for (unordered_map<vector<unsigned int>, double, vector_hash<unsigned int>>::const_iterator tupleIt = begin; tupleIt != end; ++tupleIt)
    {
      vector<vector<vector<unsigned int>>::iterator> intersectionIts;
      intersectionIts.reserve(attributeOrder.size());
      intersectionIts.push_back(attributes.front()->getIntersectionsBeginWithPotentialValues(hyperplaneId));
      const unsigned int membership = ceil(tupleIt->second * Attribute::noisePerUnit); // ceil to guarantee that every pattern to be returned is returned
      if (hyperplane->setTuple(tupleIt->first, membership, attributeOrder.begin(), ++oldIds2NewIds.begin(), nextAttributeIt, intersectionIts))
	{
	  AbstractData* newHyperplane;
	  if (isCrisp)
	    {
	      newHyperplane = new DenseCrispTube(static_cast<SparseCrispTube&>(*hyperplane), (*nextAttributeIt)->sizeOfPresentAndPotential());
	    }
	  else
	    {
	      newHyperplane = new DenseFuzzyTube(static_cast<SparseFuzzyTube&>(*hyperplane), (*nextAttributeIt)->sizeOfPresentAndPotential());
	    }
	  delete hyperplane;
	  hyperplane = newHyperplane;
	  hyperplanes[hyperplaneId] = hyperplane;
	}
      attributes.front()->substractPotentialNoise(hyperplaneId, membership);
    }
}

void Trie::setSelfLoops(const unsigned int firstSymmetricAttributeId, const unsigned int lastSymmetricAttributeId, vector<Attribute*>& attributes)
{
  vector<vector<vector<unsigned int>>::iterator> intersectionIts;
  intersectionIts.reserve(Attribute::lastAttributeId());
  setSelfLoopsBeforeSymmetricAttributes(firstSymmetricAttributeId, lastSymmetricAttributeId, attributes.begin(), intersectionIts, 0);
}

void Trie::setPresent(const vector<Attribute*>::iterator presentAttributeIt, const vector<Attribute*>::iterator attributeBegin) const
{
  vector<vector<vector<unsigned int>>::iterator> intersectionIts;
  intersectionIts.reserve(Attribute::lastAttributeId());
  setPresent(presentAttributeIt, attributeBegin, intersectionIts);
}

void Trie::setSymmetricPresent(const vector<Attribute*>::iterator presentAttributeIt, const vector<Attribute*>::iterator attributeBegin) const
{
  vector<vector<vector<unsigned int>>::iterator> intersectionIts;
  intersectionIts.reserve(Attribute::lastAttributeId());
  setSymmetricPresent(presentAttributeIt, attributeBegin, intersectionIts);
}

void Trie::setAbsent(const vector<Attribute*>::iterator absentAttributeIt, const vector<unsigned int>& absentValueDataIds, const vector<Attribute*>::iterator attributeBegin) const
{
  vector<vector<vector<unsigned int>>::iterator> intersectionIts;
  intersectionIts.reserve(Attribute::lastAttributeId());
  setAbsent(absentAttributeIt, absentValueDataIds, attributeBegin, intersectionIts);
}

void Trie::setSymmetricAbsent(const vector<Attribute*>::iterator absentAttributeIt, const vector<Attribute*>::iterator attributeBegin) const
{
  vector<vector<vector<unsigned int>>::iterator> intersectionIts;
  intersectionIts.reserve(Attribute::lastAttributeId());
  setSymmetricAbsent(absentAttributeIt, attributeBegin, intersectionIts);
}

const bool Trie::setTuple(const vector<unsigned int>& tuple, const unsigned int membership, vector<unsigned int>::const_iterator attributeIdIt, vector<vector<unsigned int>>::const_iterator oldIds2NewIdsIt, const vector<Attribute*>::iterator attributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts)
{
  const unsigned int element = oldIds2NewIdsIt->at(tuple[*attributeIdIt]);
  (*attributeIt)->substractPotentialNoise(element, membership);
  for (vector<vector<unsigned int>>::iterator& intersectionIt : intersectionIts)
    {
      (*intersectionIt)[element] -= membership;
      ++intersectionIt;
    }
  intersectionIts.push_back((*attributeIt)->getIntersectionsBeginWithPotentialValues(element));
  AbstractData*& hyperplane = hyperplanes[element];
  const vector<Attribute*>::iterator nextAttributeIt = attributeIt + 1;
  if (hyperplane->setTuple(tuple, membership, ++attributeIdIt, ++oldIds2NewIdsIt, nextAttributeIt, intersectionIts))
    {
      AbstractData* newHyperplane;
      if (isCrisp)
	{
	  newHyperplane = new DenseCrispTube(static_cast<SparseCrispTube&>(*hyperplane), (*nextAttributeIt)->sizeOfPresentAndPotential());
	}
      else
	{
	  newHyperplane = new DenseFuzzyTube(static_cast<SparseFuzzyTube&>(*hyperplane), (*nextAttributeIt)->sizeOfPresentAndPotential());
	}
      delete hyperplane;
      hyperplane = newHyperplane;
    }
  return false;
}

void Trie::setCrisp()
{
  isCrisp = true;
}

// PERF: The amount of noise in every counter could be set at the construction of the attributes (but the time to insert the self loops is negligible)
const unsigned int Trie::setSelfLoopsBeforeSymmetricAttributes(const unsigned int firstSymmetricAttributeId, const unsigned int lastSymmetricAttributeId, const vector<Attribute*>::iterator attributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts, const unsigned int dimensionId)
{
  unsigned int noiseInSelfLoops = 0;
  const vector<Attribute*>::iterator nextAttributeIt = attributeIt + 1;
  vector<vector<vector<unsigned int>>::iterator> nextIntersectionIts(incrementIterators(intersectionIts));
  vector<AbstractData*>::iterator hyperplaneIt = hyperplanes.begin();
  for (unsigned int hyperplaneId = 0; hyperplaneId != hyperplanes.size(); ++hyperplaneId)
    {
      nextIntersectionIts.push_back((*attributeIt)->getIntersectionsBeginWithPotentialValues(hyperplaneId));
      unsigned int noiseInSelfLoopsInHyperplane;
      if (dimensionId == firstSymmetricAttributeId)
	{
	  noiseInSelfLoopsInHyperplane = (*hyperplaneIt)->setSelfLoopsInSymmetricAttribute(hyperplaneId, lastSymmetricAttributeId, nextAttributeIt, nextIntersectionIts, dimensionId + 1);
	}
      else
	{
	  noiseInSelfLoopsInHyperplane = static_cast<Trie*>(*hyperplaneIt)->setSelfLoopsBeforeSymmetricAttributes(firstSymmetricAttributeId, lastSymmetricAttributeId, nextAttributeIt, nextIntersectionIts, dimensionId + 1);
	}
      ++hyperplaneIt;
      for (vector<vector<unsigned int>>::iterator intersectionIt : intersectionIts)
	{
	  (*intersectionIt)[hyperplaneId] -= noiseInSelfLoopsInHyperplane;
	}
      (*attributeIt)->substractPotentialNoise(hyperplaneId, noiseInSelfLoopsInHyperplane);
      noiseInSelfLoops += noiseInSelfLoopsInHyperplane;
      nextIntersectionIts.pop_back();
    }
  return noiseInSelfLoops;
}

// PERF: The amount of noise in every counter could be set at the construction of the attributes (but the time to insert the self loops is negligible)
const unsigned int Trie::setSelfLoopsInSymmetricAttribute(const unsigned int hyperplaneId, const unsigned int lastSymmetricAttributeId, const vector<Attribute*>::iterator attributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts, const unsigned int dimensionId)
{
  const vector<Attribute*>::iterator nextAttributeIt = attributeIt + 1;
  vector<vector<vector<unsigned int>>::iterator> nextIntersectionIts(incrementIterators(intersectionIts));
  AbstractData*& hyperplane = hyperplanes[hyperplaneId];
  nextIntersectionIts.push_back((*attributeIt)->getIntersectionsBeginWithPotentialValues(hyperplaneId));
  unsigned int noiseInSelfLoopsInHyperplane;
  if (dimensionId == lastSymmetricAttributeId)
    {
      noiseInSelfLoopsInHyperplane = hyperplane->setSelfLoopsAfterSymmetricAttributes(nextAttributeIt, nextIntersectionIts);
      if (noiseInSelfLoopsInHyperplane == 0)
	{
	  // *hyperplane is a tube that would be full of 0-noise values: turn it into a NoNoiseTube
	  delete hyperplane;
	  hyperplane = &noNoiseTube;
	  noiseInSelfLoopsInHyperplane = Attribute::noisePerUnit * (*nextAttributeIt)->sizeOfPresentAndPotential();
	}
    }
  else
    {
      noiseInSelfLoopsInHyperplane = hyperplane->setSelfLoopsInSymmetricAttribute(hyperplaneId, lastSymmetricAttributeId, nextAttributeIt, nextIntersectionIts, dimensionId + 1);
    }
  for (vector<vector<unsigned int>>::iterator intersectionIt : intersectionIts)
    {
      (*intersectionIt)[hyperplaneId] -= noiseInSelfLoopsInHyperplane;
    }
  (*attributeIt)->substractPotentialNoise(hyperplaneId, noiseInSelfLoopsInHyperplane);
  nextIntersectionIts.pop_back();
  return noiseInSelfLoopsInHyperplane;
}

// PERF: The amount of noise in every counter could be set at the construction of the attributes (but the time to insert the self loops is negligible)
const unsigned int Trie::setSelfLoopsAfterSymmetricAttributes(const vector<Attribute*>::iterator attributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts)
{
  unsigned int noiseInSelfLoops = 0;
  const vector<Attribute*>::iterator nextAttributeIt = attributeIt + 1;
  vector<vector<vector<unsigned int>>::iterator> nextIntersectionIts(incrementIterators(intersectionIts));
  vector<AbstractData*>::iterator hyperplaneIt = hyperplanes.begin();
  for (unsigned int hyperplaneId = 0; hyperplaneId != hyperplanes.size(); ++hyperplaneId)
    {
      nextIntersectionIts.push_back((*attributeIt)->getIntersectionsBeginWithPotentialValues(hyperplaneId));
      unsigned int noiseInSelfLoopsInHyperplane = (*hyperplaneIt)->setSelfLoopsAfterSymmetricAttributes(nextAttributeIt, nextIntersectionIts);
      if (noiseInSelfLoopsInHyperplane == 0)
	{
	  // **hyperplaneIt is a tube that would be full of 0-noise values: turn it into a NoNoiseTube
	  delete *hyperplaneIt;
	  hyperplanes[hyperplaneId] = &noNoiseTube;
	  noiseInSelfLoopsInHyperplane = Attribute::noisePerUnit * (*nextAttributeIt)->sizeOfPresentAndPotential();
	}
      ++hyperplaneIt;
      for (vector<vector<unsigned int>>::iterator intersectionIt : intersectionIts)
	{
	  (*intersectionIt)[hyperplaneId] -= noiseInSelfLoopsInHyperplane;
	}
      (*attributeIt)->substractPotentialNoise(hyperplaneId, noiseInSelfLoopsInHyperplane);
      noiseInSelfLoops += noiseInSelfLoopsInHyperplane;
      nextIntersectionIts.pop_back();
    }
  return noiseInSelfLoops;
}

const unsigned int Trie::setPresent(const vector<Attribute*>::iterator presentAttributeIt, const vector<Attribute*>::iterator attributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const
{
  const vector<Attribute*>::iterator nextAttributeIt = attributeIt + 1;
  vector<vector<vector<unsigned int>>::iterator> nextIntersectionIts(incrementIterators(intersectionIts));
  if (attributeIt == presentAttributeIt)
    {
      Value& presentValue = (*presentAttributeIt)->getChosenValue();
      nextIntersectionIts.push_back(presentValue.getIntersectionsBeginWithPresentValues());
      return hyperplanes[presentValue.getDataId()]->setPresentAfterPresentValueMet(nextAttributeIt, nextIntersectionIts);
    }
  presentFixPotentialOrAbsentValues(**attributeIt, presentAttributeIt, nextAttributeIt, intersectionIts);
  return presentFixPresentValues(**attributeIt, presentAttributeIt, nextAttributeIt, nextIntersectionIts);
}

const unsigned int Trie::setSymmetricPresent(const vector<Attribute*>::iterator presentAttributeIt, const vector<Attribute*>::iterator attributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const
{
  const vector<Attribute*>::iterator nextAttributeIt = attributeIt + 1;
  vector<vector<vector<unsigned int>>::iterator> nextIntersectionIts(incrementIterators(intersectionIts));
  if (attributeIt == presentAttributeIt)
    {
      // *this necessarily relates to the first symmetric attribute
      presentFixPotentialOrAbsentValuesInFirstSymmetricAttribute(**attributeIt, nextAttributeIt, intersectionIts);
      const unsigned int newNoise = presentFixPresentValues(**attributeIt, nextAttributeIt, nextAttributeIt, nextIntersectionIts);
      Value& presentValue = (*presentAttributeIt)->getChosenValue();
      nextIntersectionIts.push_back(presentValue.getIntersectionsBeginWithPresentValues());
      return newNoise + hyperplanes[presentValue.getDataId()]->setSymmetricPresentAfterPresentValueMet(nextAttributeIt, nextIntersectionIts);
    }
  presentFixPotentialOrAbsentValuesBeforeSymmetricAttributes(**attributeIt, presentAttributeIt, nextAttributeIt, intersectionIts);
  return presentFixPresentValuesBeforeSymmetricAttributes(**attributeIt, presentAttributeIt, nextAttributeIt, nextIntersectionIts);
}

const unsigned int Trie::setPresentAfterPresentValueMet(const vector<Attribute*>::iterator attributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const
{
  const vector<Attribute*>::iterator nextAttributeIt = attributeIt + 1;
  vector<vector<vector<unsigned int>>::iterator> nextIntersectionIts(incrementIterators(intersectionIts));
  presentFixPotentialOrAbsentValuesAfterPresentValueMet(**attributeIt, nextAttributeIt, intersectionIts);
  return presentFixPresentValuesAfterPresentValueMet(**attributeIt, nextAttributeIt, nextIntersectionIts);
}

const unsigned int Trie::setSymmetricPresentAfterPresentValueMet(const vector<Attribute*>::iterator attributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const
{
  // *this necessarily relates to the second symmetric attribute
  const vector<Attribute*>::iterator nextAttributeIt = attributeIt + 1;
  vector<vector<vector<unsigned int>>::iterator> nextIntersectionIts(incrementIterators(intersectionIts));
  presentFixPotentialOrAbsentValuesInSecondSymmetricAttribute(**attributeIt, nextAttributeIt, intersectionIts);
  return presentFixPresentValuesAfterPresentValueMet(**attributeIt, nextAttributeIt, nextIntersectionIts);
}

const unsigned int Trie::setPresentAfterPotentialOrAbsentUsed(const vector<Attribute*>::iterator presentAttributeIt, const vector<Attribute*>::iterator attributeIt, const vector<vector<unsigned int>>::iterator potentialOrAbsentValueIntersectionIt) const
{
  if (attributeIt == presentAttributeIt)
    {
      const Value& presentValue = (*presentAttributeIt)->getChosenValue();
      const unsigned int newNoiseInHyperplane = hyperplanes[presentValue.getDataId()]->setPresentAfterPresentValueMetAndPotentialOrAbsentUsed(attributeIt + 1, potentialOrAbsentValueIntersectionIt + 1);
      (*potentialOrAbsentValueIntersectionIt)[presentValue.getIntersectionId()] += newNoiseInHyperplane;
      return newNoiseInHyperplane;
    }
  return presentFixPresentValuesAfterPotentialOrAbsentUsed(**attributeIt, presentAttributeIt, attributeIt + 1, potentialOrAbsentValueIntersectionIt, potentialOrAbsentValueIntersectionIt + 1);
}

const unsigned int Trie::setSymmetricPresentAfterPotentialOrAbsentUsed(const vector<Attribute*>::iterator presentAttributeIt, const vector<Attribute*>::iterator attributeIt, const vector<vector<unsigned int>>::iterator potentialOrAbsentValueIntersectionIt) const
{
  if (attributeIt == presentAttributeIt)
    {
      // *this necessarily relates to the first symmetric attribute
      const Value& presentValue = (*presentAttributeIt)->getChosenValue();
      const unsigned int newNoiseInHyperplane = hyperplanes[presentValue.getDataId()]->setPresentAfterPresentValueMetAndPotentialOrAbsentUsed(attributeIt + 1, potentialOrAbsentValueIntersectionIt + 1);
      (*potentialOrAbsentValueIntersectionIt)[presentValue.getIntersectionId()] += newNoiseInHyperplane;
      return newNoiseInHyperplane + presentFixPresentValuesAfterPotentialOrAbsentUsed(**attributeIt, presentAttributeIt + 1, attributeIt + 1, potentialOrAbsentValueIntersectionIt, potentialOrAbsentValueIntersectionIt + 1);
    }
  return presentFixPresentValuesBeforeSymmetricAttributesAfterPotentialOrAbsentUsed(**attributeIt, presentAttributeIt, attributeIt + 1, potentialOrAbsentValueIntersectionIt, potentialOrAbsentValueIntersectionIt + 1);
}

const unsigned int Trie::setPresentAfterPresentValueMetAndPotentialOrAbsentUsed(const vector<Attribute*>::iterator attributeIt, const vector<vector<unsigned int>>::iterator potentialOrAbsentValueIntersectionIt) const
{
  return presentFixPresentValuesAfterPresentValueMetAndPotentialOrAbsentUsed(**attributeIt, attributeIt + 1, potentialOrAbsentValueIntersectionIt, potentialOrAbsentValueIntersectionIt + 1);
}

const unsigned int Trie::presentFixPresentValues(Attribute& currentAttribute, const vector<Attribute*>::iterator presentAttributeIt, const vector<Attribute*>::iterator nextAttributeIt, vector<vector<vector<unsigned int>>::iterator>& nextIntersectionIts) const
{
  unsigned int newNoise = 0;
  const vector<Value*>::iterator end = currentAttribute.presentEnd();
  for (vector<Value*>::iterator valueIt = currentAttribute.presentBegin(); valueIt != end; ++valueIt)
    {
      nextIntersectionIts.push_back((*valueIt)->getIntersectionsBeginWithPresentValues());
      const unsigned int newNoiseInHyperplane = hyperplanes[(*valueIt)->getDataId()]->setPresent(presentAttributeIt, nextAttributeIt, nextIntersectionIts);
      nextIntersectionIts.pop_back();
      (*valueIt)->addPresentNoise(newNoiseInHyperplane);
      newNoise += newNoiseInHyperplane;
    }
  return newNoise;
}

const unsigned int Trie::presentFixPresentValuesBeforeSymmetricAttributes(Attribute& currentAttribute, const vector<Attribute*>::iterator presentAttributeIt, const vector<Attribute*>::iterator nextAttributeIt, vector<vector<vector<unsigned int>>::iterator>& nextIntersectionIts) const
{
  unsigned int newNoise = 0;
  const vector<Value*>::iterator end = currentAttribute.presentEnd();
  for (vector<Value*>::iterator valueIt = currentAttribute.presentBegin(); valueIt != end; ++valueIt)
    {
      nextIntersectionIts.push_back((*valueIt)->getIntersectionsBeginWithPresentValues());
      // Since this is before the symmetric attributes, hyperplanes necessarily are tries
      const unsigned int newNoiseInHyperplane = static_cast<Trie*>(hyperplanes[(*valueIt)->getDataId()])->setSymmetricPresent(presentAttributeIt, nextAttributeIt, nextIntersectionIts);
      nextIntersectionIts.pop_back();
      (*valueIt)->addPresentNoise(newNoiseInHyperplane);
      newNoise += newNoiseInHyperplane;
    }
  return newNoise;
}

const unsigned int Trie::presentFixPresentValuesAfterPresentValueMet(Attribute& currentAttribute, const vector<Attribute*>::iterator nextAttributeIt, vector<vector<vector<unsigned int>>::iterator>& nextIntersectionIts) const
{
  unsigned int newNoise = 0;
  const vector<Value*>::iterator end = currentAttribute.presentEnd();
  for (vector<Value*>::iterator valueIt = currentAttribute.presentBegin(); valueIt != end; ++valueIt)
    {
      nextIntersectionIts.push_back((*valueIt)->getIntersectionsBeginWithPresentValues());
      const unsigned int newNoiseInHyperplane = hyperplanes[(*valueIt)->getDataId()]->setPresentAfterPresentValueMet(nextAttributeIt, nextIntersectionIts);
      nextIntersectionIts.pop_back();
      (*valueIt)->addPresentNoise(newNoiseInHyperplane);
      newNoise += newNoiseInHyperplane;
    }
  return newNoise;
}

const unsigned int Trie::presentFixPresentValuesAfterPotentialOrAbsentUsed(Attribute& currentAttribute, const vector<Attribute*>::iterator presentAttributeIt, const vector<Attribute*>::iterator nextAttributeIt, const vector<vector<unsigned int>>::iterator potentialOrAbsentValueIntersectionIt, const vector<vector<unsigned int>>::iterator nextPotentialOrAbsentValueIntersectionIt) const
{
  unsigned int newNoise = 0;
  const vector<Value*>::iterator end = currentAttribute.presentEnd();
  for (vector<Value*>::iterator valueIt = currentAttribute.presentBegin(); valueIt != end; ++valueIt)
    {
      const unsigned int newNoiseInHyperplane = hyperplanes[(*valueIt)->getDataId()]->setPresentAfterPotentialOrAbsentUsed(presentAttributeIt, nextAttributeIt, nextPotentialOrAbsentValueIntersectionIt);
      (*potentialOrAbsentValueIntersectionIt)[(*valueIt)->getIntersectionId()] += newNoiseInHyperplane;
      newNoise += newNoiseInHyperplane;
    }
  return newNoise;
}

const unsigned int Trie::presentFixPresentValuesBeforeSymmetricAttributesAfterPotentialOrAbsentUsed(Attribute& currentAttribute, const vector<Attribute*>::iterator presentAttributeIt, const vector<Attribute*>::iterator nextAttributeIt, const vector<vector<unsigned int>>::iterator potentialOrAbsentValueIntersectionIt, const vector<vector<unsigned int>>::iterator nextPotentialOrAbsentValueIntersectionIt) const
{
  unsigned int newNoise = 0;
  const vector<Value*>::iterator end = currentAttribute.presentEnd();
  for (vector<Value*>::iterator valueIt = currentAttribute.presentBegin(); valueIt != end; ++valueIt)
    {
      // Since this is before the symmetric attributes, hyperplanes necessarily are tries
      const unsigned int newNoiseInHyperplane = static_cast<Trie*>(hyperplanes[(*valueIt)->getDataId()])->setSymmetricPresentAfterPotentialOrAbsentUsed(presentAttributeIt, nextAttributeIt, nextPotentialOrAbsentValueIntersectionIt);
      (*potentialOrAbsentValueIntersectionIt)[(*valueIt)->getIntersectionId()] += newNoiseInHyperplane;
      newNoise += newNoiseInHyperplane;
    }
  return newNoise;
}

const unsigned int Trie::presentFixPresentValuesAfterPresentValueMetAndPotentialOrAbsentUsed(Attribute& currentAttribute, const vector<Attribute*>::iterator nextAttributeIt, const vector<vector<unsigned int>>::iterator potentialOrAbsentValueIntersectionIt, const vector<vector<unsigned int>>::iterator nextPotentialOrAbsentValueIntersectionIt) const
{
  unsigned int newNoise = 0;
  const vector<Value*>::iterator end = currentAttribute.presentEnd();
  for (vector<Value*>::iterator valueIt = currentAttribute.presentBegin(); valueIt != end; ++valueIt)
    {
      const unsigned int newNoiseInHyperplane = hyperplanes[(*valueIt)->getDataId()]->setPresentAfterPresentValueMetAndPotentialOrAbsentUsed(nextAttributeIt, nextPotentialOrAbsentValueIntersectionIt);
      (*potentialOrAbsentValueIntersectionIt)[(*valueIt)->getIntersectionId()] += newNoiseInHyperplane;
      newNoise += newNoiseInHyperplane;
    }
  return newNoise;
}

void Trie::presentFixPotentialOrAbsentValues(Attribute& currentAttribute, const vector<Attribute*>::iterator presentAttributeIt, const vector<Attribute*>::iterator nextAttributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const
{
  const vector<Value*>::iterator end = currentAttribute.absentEnd();
  for (vector<Value*>::iterator valueIt = currentAttribute.potentialBegin(); valueIt != end; ++valueIt)
    {
      const unsigned int newNoiseInHyperplane = hyperplanes[(*valueIt)->getDataId()]->setPresentAfterPotentialOrAbsentUsed(presentAttributeIt, nextAttributeIt, (*valueIt)->getIntersectionsBeginWithPresentValues());
      (*valueIt)->addPresentNoise(newNoiseInHyperplane);
      const unsigned int valueId = (*valueIt)->getIntersectionId();
      for (vector<vector<unsigned int>>::iterator intersectionIt : intersectionIts)
	{
	  (*intersectionIt)[valueId] += newNoiseInHyperplane;
	}
    }
}

void Trie::presentFixPotentialOrAbsentValuesInFirstSymmetricAttribute(Attribute& currentAttribute, const vector<Attribute*>::iterator nextAttributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const
{
  const vector<Value*>::iterator end = currentAttribute.absentEnd();
  // The first potential value actually is the value set present and there is no noise to be found at the intersection of a vertex (seen as an outgoing vertex) and itself (seen as an ingoing vertex)
  for (vector<Value*>::iterator valueIt = currentAttribute.potentialBegin(); ++valueIt != end; )
    {
      const unsigned int newNoiseInHyperplane = hyperplanes[(*valueIt)->getDataId()]->setPresentAfterPotentialOrAbsentUsed(nextAttributeIt, nextAttributeIt, (*valueIt)->getIntersectionsBeginWithPresentValues());
      (*valueIt)->addPresentNoise(newNoiseInHyperplane);
      const unsigned int valueId = (*valueIt)->getIntersectionId();
      for (vector<vector<unsigned int>>::iterator intersectionIt : intersectionIts)
	{
	  (*intersectionIt)[valueId] += newNoiseInHyperplane;
	}
    }
}

void Trie::presentFixPotentialOrAbsentValuesBeforeSymmetricAttributes(Attribute& currentAttribute, const vector<Attribute*>::iterator presentAttributeIt, const vector<Attribute*>::iterator nextAttributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const
{
  const vector<Value*>::iterator end = currentAttribute.absentEnd();
  for (vector<Value*>::iterator valueIt = currentAttribute.potentialBegin(); valueIt != end; ++valueIt)
    {
      // Since this is before the symmetric attributes, hyperplanes necessarily are tries
      const unsigned int newNoiseInHyperplane = static_cast<Trie*>(hyperplanes[(*valueIt)->getDataId()])->setSymmetricPresentAfterPotentialOrAbsentUsed(presentAttributeIt, nextAttributeIt, (*valueIt)->getIntersectionsBeginWithPresentValues());
      (*valueIt)->addPresentNoise(newNoiseInHyperplane);
      const unsigned int valueId = (*valueIt)->getIntersectionId();
      for (vector<vector<unsigned int>>::iterator intersectionIt : intersectionIts)
	{
	  (*intersectionIt)[valueId] += newNoiseInHyperplane;
	}
    }
}

void Trie::presentFixPotentialOrAbsentValuesAfterPresentValueMet(Attribute& currentAttribute, const vector<Attribute*>::iterator nextAttributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const
{
  const vector<Value*>::iterator end = currentAttribute.absentEnd();
  for (vector<Value*>::iterator valueIt = currentAttribute.potentialBegin(); valueIt != end; ++valueIt)
    {
      const unsigned int newNoiseInHyperplane = hyperplanes[(*valueIt)->getDataId()]->setPresentAfterPresentValueMetAndPotentialOrAbsentUsed(nextAttributeIt, (*valueIt)->getIntersectionsBeginWithPresentValues());
      (*valueIt)->addPresentNoise(newNoiseInHyperplane);
      const unsigned int valueId = (*valueIt)->getIntersectionId();
      for (vector<vector<unsigned int>>::iterator intersectionIt : intersectionIts)
	{
	  (*intersectionIt)[valueId] += newNoiseInHyperplane;
	}
    }
}

void Trie::presentFixPotentialOrAbsentValuesInSecondSymmetricAttribute(Attribute& currentAttribute, const vector<Attribute*>::iterator nextAttributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const
{
  const vector<Value*>::iterator end = currentAttribute.absentEnd();
  // The first potential value actually is the value set present and there is no noise to be found at the intersection of a vertex (seen as an outgoing vertex) and itself (seen as an ingoing vertex)
  for (vector<Value*>::iterator valueIt = currentAttribute.potentialBegin(); ++valueIt != end; )
    {
      const unsigned int newNoiseInHyperplane = hyperplanes[(*valueIt)->getDataId()]->setPresentAfterPresentValueMetAndPotentialOrAbsentUsed(nextAttributeIt, (*valueIt)->getIntersectionsBeginWithPresentValues());
      (*valueIt)->addPresentNoise(newNoiseInHyperplane);
      const unsigned int valueId = (*valueIt)->getIntersectionId();
      for (vector<vector<unsigned int>>::iterator intersectionIt : intersectionIts)
	{
	  (*intersectionIt)[valueId] += newNoiseInHyperplane;
	}
    }
}

const unsigned int Trie::setAbsent(const vector<Attribute*>::iterator absentAttributeIt, const vector<unsigned int>& absentValueDataIds, const vector<Attribute*>::iterator attributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const
{
  const vector<Attribute*>::iterator nextAttributeIt = attributeIt + 1;
  vector<vector<vector<unsigned int>>::iterator> nextIntersectionIts(incrementIterators(intersectionIts));
  if (attributeIt == absentAttributeIt)
    {
      unsigned int oldNoise = 0;
      for (const unsigned int absentValueDataId : absentValueDataIds)
	{
	  oldNoise += hyperplanes[absentValueDataId]->setAbsentAfterAbsentValuesMet(nextAttributeIt, nextIntersectionIts);
	}
      return oldNoise;
    }
  absentFixAbsentValues(**attributeIt, absentAttributeIt, absentValueDataIds, nextAttributeIt, intersectionIts);
  return absentFixPresentOrPotentialValues(**attributeIt, absentAttributeIt, absentValueDataIds, nextAttributeIt, intersectionIts, nextIntersectionIts);
}

const unsigned int Trie::setSymmetricAbsent(const vector<Attribute*>::iterator absentAttributeIt, const vector<Attribute*>::iterator attributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const
{
  const vector<Attribute*>::iterator nextAttributeIt = attributeIt + 1;
  vector<vector<vector<unsigned int>>::iterator> nextIntersectionIts(incrementIterators(intersectionIts));
  if (attributeIt == absentAttributeIt)
    {
      // *this necessarily relates to the first symmetric attribute
      const unsigned int absentValueDataId = (*absentAttributeIt)->getChosenValue().getDataId();
      const vector<unsigned int> absentValueDataIds {absentValueDataId};
      absentFixAbsentValues(**attributeIt, nextAttributeIt, absentValueDataIds, nextAttributeIt, intersectionIts);
      return hyperplanes[absentValueDataId]->setSymmetricAbsentAfterAbsentValueMet(nextAttributeIt, nextIntersectionIts) + absentFixPresentOrPotentialValuesInFirstSymmetricAttribute(**attributeIt, absentValueDataIds, nextAttributeIt, intersectionIts, nextIntersectionIts);
    }
  absentFixAbsentValuesBeforeSymmetricAttributes(**attributeIt, absentAttributeIt, nextAttributeIt, intersectionIts);
  return absentFixPresentOrPotentialValuesBeforeSymmetricAttributes(**attributeIt, absentAttributeIt, nextAttributeIt, intersectionIts, nextIntersectionIts);
}

const unsigned int Trie::setAbsentAfterAbsentValuesMet(const vector<Attribute*>::iterator attributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const
{
  const vector<Attribute*>::iterator nextAttributeIt = attributeIt + 1;
  vector<vector<vector<unsigned int>>::iterator> nextIntersectionIts(incrementIterators(intersectionIts));
  absentFixAbsentValuesAfterAbsentValuesMet(**attributeIt, nextAttributeIt, intersectionIts);
  return absentFixPresentOrPotentialValuesAfterAbsentValuesMet(**attributeIt, nextAttributeIt, intersectionIts, nextIntersectionIts);
}

const unsigned int Trie::setSymmetricAbsentAfterAbsentValueMet(const vector<Attribute*>::iterator attributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const
{
  // *this necessarily relates to the second symmetric attribute
  const vector<Attribute*>::iterator nextAttributeIt = attributeIt + 1;
  vector<vector<vector<unsigned int>>::iterator> nextIntersectionIts(incrementIterators(intersectionIts));
  absentFixAbsentValuesAfterAbsentValuesMet(**attributeIt, nextAttributeIt, intersectionIts);
  return absentFixPresentOrPotentialValuesInSecondSymmetricAttribute(**attributeIt, nextAttributeIt, intersectionIts, nextIntersectionIts);
}

const unsigned int Trie::setAbsentAfterAbsentUsed(const vector<Attribute*>::iterator absentAttributeIt, const vector<unsigned int>& absentValueDataIds, const vector<Attribute*>::iterator attributeIt, const vector<vector<unsigned int>>::iterator absentValueIntersectionIt) const
{
  const vector<Attribute*>::iterator nextAttributeIt = attributeIt + 1;
  const vector<vector<unsigned int>>::iterator nextAbsentValueIntersectionIt = absentValueIntersectionIt + 1;
  if (attributeIt == absentAttributeIt)
    {
      unsigned int oldNoise = 0;
      for (const unsigned int absentValueDataId : absentValueDataIds)
	{
	  oldNoise += hyperplanes[absentValueDataId]->setAbsentAfterAbsentValuesMetAndAbsentUsed(nextAttributeIt, nextAbsentValueIntersectionIt);
	}
      return oldNoise;
    }
  return absentFixPresentOrPotentialValuesAfterAbsentUsed(**attributeIt, absentAttributeIt, absentValueDataIds, nextAttributeIt, absentValueIntersectionIt, nextAbsentValueIntersectionIt);
}

const unsigned int Trie::setSymmetricAbsentAfterAbsentUsed(const vector<Attribute*>::iterator absentAttributeIt, const vector<Attribute*>::iterator attributeIt, const vector<vector<unsigned int>>::iterator absentValueIntersectionIt) const
{
  const vector<Attribute*>::iterator nextAttributeIt = attributeIt + 1;
  const vector<vector<unsigned int>>::iterator nextAbsentValueIntersectionIt = absentValueIntersectionIt + 1;
  if (attributeIt == absentAttributeIt)
    {
      // *this necessarily relates to the first symmetric attribute
      const unsigned int absentValueDataId = (*absentAttributeIt)->getChosenValue().getDataId();
      const vector<unsigned int> absentValueDataIds {absentValueDataId};
      return hyperplanes[absentValueDataId]->setSymmetricAbsentAfterAbsentValueMetAndAbsentUsed(nextAttributeIt, nextAbsentValueIntersectionIt) + absentFixPresentOrPotentialValuesInFirstSymmetricAttributeAfterAbsentUsed(**attributeIt, absentValueDataIds, nextAttributeIt, absentValueIntersectionIt, nextAbsentValueIntersectionIt);
    }
  return absentFixPresentOrPotentialValuesBeforeSymmetricAttributesAfterAbsentUsed(**attributeIt, absentAttributeIt, nextAttributeIt, absentValueIntersectionIt, nextAbsentValueIntersectionIt);
}

const unsigned int Trie::setAbsentAfterAbsentValuesMetAndAbsentUsed(const vector<Attribute*>::iterator attributeIt, const vector<vector<unsigned int>>::iterator absentValueIntersectionIt) const
{
  return absentFixPresentOrPotentialValuesAfterAbsentValuesMetAndAbsentUsed(**attributeIt, attributeIt + 1, absentValueIntersectionIt, absentValueIntersectionIt + 1);
}

const unsigned int Trie::setSymmetricAbsentAfterAbsentValueMetAndAbsentUsed(const vector<Attribute*>::iterator attributeIt, const vector<vector<unsigned int>>::iterator absentValueIntersectionIt) const
{
  // *this necessarily relates to the second symmetric attribute
  return absentFixPresentOrPotentialValuesInSecondSymmetricAttributeAfterAbsentUsed(**attributeIt, attributeIt + 1, absentValueIntersectionIt, absentValueIntersectionIt + 1);
}

const unsigned int Trie::absentFixPresentOrPotentialValues(Attribute& currentAttribute, const vector<Attribute*>::iterator absentAttributeIt, const vector<unsigned int>& absentValueDataIds, const vector<Attribute*>::iterator nextAttributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts, vector<vector<vector<unsigned int>>::iterator>& nextIntersectionIts) const
{
  unsigned int oldNoise = 0;
  const vector<Value*>::iterator end = currentAttribute.irrelevantEnd();
  for (vector<Value*>::iterator valueIt = currentAttribute.presentBegin(); valueIt != end; ++valueIt)
    {
      nextIntersectionIts.push_back((*valueIt)->getIntersectionsBeginWithPresentAndPotentialValues());
      const unsigned int oldNoiseInHyperplane = hyperplanes[(*valueIt)->getDataId()]->setAbsent(absentAttributeIt, absentValueDataIds, nextAttributeIt, nextIntersectionIts);
      nextIntersectionIts.pop_back();
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

const unsigned int Trie::absentFixPresentOrPotentialValuesInFirstSymmetricAttribute(Attribute& currentAttribute, const vector<unsigned int>& absentValueDataIds, const vector<Attribute*>::iterator nextAttributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts, vector<vector<vector<unsigned int>>::iterator>& nextIntersectionIts) const
{
  unsigned int oldNoise = 0;
  vector<Value*>::iterator end = currentAttribute.presentEnd();
  vector<Value*>::iterator valueIt = currentAttribute.presentBegin();
  for (; valueIt != end; ++valueIt)
    {
      nextIntersectionIts.push_back((*valueIt)->getIntersectionsBeginWithPresentAndPotentialValues());
      const unsigned int oldNoiseInHyperplane = hyperplanes[(*valueIt)->getDataId()]->setAbsent(nextAttributeIt, absentValueDataIds, nextAttributeIt, nextIntersectionIts);
      nextIntersectionIts.pop_back();
      (*valueIt)->substractPotentialNoise(oldNoiseInHyperplane);
      const unsigned int valueId = (*valueIt)->getIntersectionId();
      for (vector<vector<unsigned int>>::iterator intersectionIt : intersectionIts)
	{
	  (*intersectionIt)[valueId] -= oldNoiseInHyperplane;
	}
      oldNoise += oldNoiseInHyperplane;
    }
  end = currentAttribute.absentEnd();
  // The first potential value actually is the value set absent and there is no noise to be found at the intersection of a vertex (seen as an outgoing vertex) and itself (seen as an ingoing vertex)
  while (++valueIt != end)
    {
      nextIntersectionIts.push_back((*valueIt)->getIntersectionsBeginWithPresentAndPotentialValues());
      const unsigned int oldNoiseInHyperplane = hyperplanes[(*valueIt)->getDataId()]->setAbsent(nextAttributeIt, absentValueDataIds, nextAttributeIt, nextIntersectionIts);
      nextIntersectionIts.pop_back();
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

const unsigned int Trie::absentFixPresentOrPotentialValuesBeforeSymmetricAttributes(Attribute& currentAttribute, const vector<Attribute*>::iterator absentAttributeIt, const vector<Attribute*>::iterator nextAttributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts, vector<vector<vector<unsigned int>>::iterator>& nextIntersectionIts) const
{
  unsigned int oldNoise = 0;
  const vector<Value*>::iterator end = currentAttribute.irrelevantEnd();
  for (vector<Value*>::iterator valueIt = currentAttribute.presentBegin(); valueIt != end; ++valueIt)
    {
      nextIntersectionIts.push_back((*valueIt)->getIntersectionsBeginWithPresentAndPotentialValues());
      // Since this is before the symmetric attributes, hyperplanes necessarily are tries
      const unsigned int oldNoiseInHyperplane = static_cast<Trie*>(hyperplanes[(*valueIt)->getDataId()])->setSymmetricAbsent(absentAttributeIt, nextAttributeIt, nextIntersectionIts);
      nextIntersectionIts.pop_back();
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

const unsigned int Trie::absentFixPresentOrPotentialValuesAfterAbsentValuesMet(Attribute& currentAttribute, const vector<Attribute*>::iterator nextAttributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts, vector<vector<vector<unsigned int>>::iterator>& nextIntersectionIts) const
{
  unsigned int oldNoise = 0;
  const vector<Value*>::iterator end = currentAttribute.irrelevantEnd();
  for (vector<Value*>::iterator valueIt = currentAttribute.presentBegin(); valueIt != end; ++valueIt)
    {
      nextIntersectionIts.push_back((*valueIt)->getIntersectionsBeginWithPresentAndPotentialValues());
      const unsigned int oldNoiseInHyperplane = hyperplanes[(*valueIt)->getDataId()]->setAbsentAfterAbsentValuesMet(nextAttributeIt, nextIntersectionIts);
      nextIntersectionIts.pop_back();
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

const unsigned int Trie::absentFixPresentOrPotentialValuesInSecondSymmetricAttribute(Attribute& currentAttribute, const vector<Attribute*>::iterator nextAttributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts, vector<vector<vector<unsigned int>>::iterator>& nextIntersectionIts) const
{
  unsigned int oldNoise = 0;
  vector<Value*>::iterator end = currentAttribute.presentEnd();
  vector<Value*>::iterator valueIt = currentAttribute.presentBegin();
  for (; valueIt != end; ++valueIt)
    {
      nextIntersectionIts.push_back((*valueIt)->getIntersectionsBeginWithPresentAndPotentialValues());
      const unsigned int oldNoiseInHyperplane = hyperplanes[(*valueIt)->getDataId()]->setAbsentAfterAbsentValuesMet(nextAttributeIt, nextIntersectionIts);
      nextIntersectionIts.pop_back();
      (*valueIt)->substractPotentialNoise(oldNoiseInHyperplane);
      const unsigned int valueId = (*valueIt)->getIntersectionId();
      for (vector<vector<unsigned int>>::iterator intersectionIt : intersectionIts)
	{
	  (*intersectionIt)[valueId] -= oldNoiseInHyperplane;
	}
      oldNoise += oldNoiseInHyperplane;
    }
  end = currentAttribute.irrelevantEnd();
  // The first potential value actually is the value set absent and there is no noise to be found at the intersection of a vertex (seen as an outgoing vertex) and itself (seen as an ingoing vertex)
  while (++valueIt != end)
    {
      nextIntersectionIts.push_back((*valueIt)->getIntersectionsBeginWithPresentAndPotentialValues());
      const unsigned int oldNoiseInHyperplane = hyperplanes[(*valueIt)->getDataId()]->setAbsentAfterAbsentValuesMet(nextAttributeIt, nextIntersectionIts);
      nextIntersectionIts.pop_back();
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

const unsigned int Trie::absentFixPresentOrPotentialValuesAfterAbsentUsed(Attribute& currentAttribute, const vector<Attribute*>::iterator absentAttributeIt, const vector<unsigned int>& absentValueDataIds, const vector<Attribute*>::iterator nextAttributeIt, const vector<vector<unsigned int>>::iterator absentValueIntersectionIt, const vector<vector<unsigned int>>::iterator nextAbsentValueIntersectionIt) const
{
  unsigned int oldNoise = 0;
  const vector<Value*>::iterator end = currentAttribute.irrelevantEnd();
  for (vector<Value*>::iterator valueIt = currentAttribute.presentBegin(); valueIt != end; ++valueIt)
    {
      const unsigned int oldNoiseInHyperplane = hyperplanes[(*valueIt)->getDataId()]->setAbsentAfterAbsentUsed(absentAttributeIt, absentValueDataIds, nextAttributeIt, nextAbsentValueIntersectionIt);
      (*absentValueIntersectionIt)[(*valueIt)->getIntersectionId()] -= oldNoiseInHyperplane;
      oldNoise += oldNoiseInHyperplane;
    }
  return oldNoise;
}

const unsigned int Trie::absentFixPresentOrPotentialValuesInFirstSymmetricAttributeAfterAbsentUsed(Attribute& currentAttribute, const vector<unsigned int>& absentValueDataIds, const vector<Attribute*>::iterator nextAttributeIt, const vector<vector<unsigned int>>::iterator absentValueIntersectionIt, const vector<vector<unsigned int>>::iterator nextAbsentValueIntersectionIt) const
{
  unsigned int oldNoise = 0;
  vector<Value*>::iterator end = currentAttribute.presentEnd();
  vector<Value*>::iterator valueIt = currentAttribute.presentBegin();
  for (; valueIt != end; ++valueIt)
    {
      const unsigned int oldNoiseInHyperplane = hyperplanes[(*valueIt)->getDataId()]->setAbsentAfterAbsentUsed(nextAttributeIt, absentValueDataIds, nextAttributeIt, nextAbsentValueIntersectionIt);
      (*absentValueIntersectionIt)[(*valueIt)->getIntersectionId()] -= oldNoiseInHyperplane;
      oldNoise += oldNoiseInHyperplane;
    }
  end = currentAttribute.irrelevantEnd();
  // The first potential value actually is the value set absent and there is no noise to be found at the intersection of a vertex (seen as an outgoing vertex) and itself (seen as an ingoing vertex)
  while (++valueIt != end)
    {
      const unsigned int oldNoiseInHyperplane = hyperplanes[(*valueIt)->getDataId()]->setAbsentAfterAbsentUsed(nextAttributeIt, absentValueDataIds, nextAttributeIt, nextAbsentValueIntersectionIt);
      (*absentValueIntersectionIt)[(*valueIt)->getIntersectionId()] -= oldNoiseInHyperplane;
      oldNoise += oldNoiseInHyperplane;
    }  
  return oldNoise;
}

const unsigned int Trie::absentFixPresentOrPotentialValuesBeforeSymmetricAttributesAfterAbsentUsed(Attribute& currentAttribute, const vector<Attribute*>::iterator absentAttributeIt, const vector<Attribute*>::iterator nextAttributeIt, const vector<vector<unsigned int>>::iterator absentValueIntersectionIt, const vector<vector<unsigned int>>::iterator nextAbsentValueIntersectionIt) const
{
  unsigned int oldNoise = 0;
  const vector<Value*>::iterator end = currentAttribute.presentEnd();
  for (vector<Value*>::iterator valueIt = currentAttribute.presentBegin(); valueIt != end; ++valueIt)
    {
      // Since this is before the symmetric attributes, hyperplanes necessarily are tries
      const unsigned int oldNoiseInHyperplane = static_cast<Trie*>(hyperplanes[(*valueIt)->getDataId()])->setSymmetricAbsentAfterAbsentUsed(absentAttributeIt, nextAttributeIt, nextAbsentValueIntersectionIt);
      (*absentValueIntersectionIt)[(*valueIt)->getIntersectionId()] -= oldNoiseInHyperplane;
      oldNoise += oldNoiseInHyperplane;
    }
  return oldNoise;
}

const unsigned int Trie::absentFixPresentOrPotentialValuesAfterAbsentValuesMetAndAbsentUsed(Attribute& currentAttribute, const vector<Attribute*>::iterator nextAttributeIt, const vector<vector<unsigned int>>::iterator absentValueIntersectionIt, const vector<vector<unsigned int>>::iterator nextAbsentValueIntersectionIt) const
{
  unsigned int oldNoise = 0;
  const vector<Value*>::iterator end = currentAttribute.irrelevantEnd();
  for (vector<Value*>::iterator valueIt = currentAttribute.presentBegin(); valueIt != end; ++valueIt)
    {
      const unsigned int oldNoiseInHyperplane = hyperplanes[(*valueIt)->getDataId()]->setAbsentAfterAbsentValuesMetAndAbsentUsed(nextAttributeIt, nextAbsentValueIntersectionIt);
      (*absentValueIntersectionIt)[(*valueIt)->getIntersectionId()] -= oldNoiseInHyperplane;
      oldNoise += oldNoiseInHyperplane;
    }
  return oldNoise;
}

const unsigned int Trie::absentFixPresentOrPotentialValuesInSecondSymmetricAttributeAfterAbsentUsed(Attribute& currentAttribute, const vector<Attribute*>::iterator nextAttributeIt, const vector<vector<unsigned int>>::iterator absentValueIntersectionIt, const vector<vector<unsigned int>>::iterator nextAbsentValueIntersectionIt) const
{
  unsigned int oldNoise = 0;
  vector<Value*>::iterator end = currentAttribute.presentEnd();
  vector<Value*>::iterator valueIt = currentAttribute.presentBegin();
  for (; valueIt != end; ++valueIt)
    {
      const unsigned int oldNoiseInHyperplane = hyperplanes[(*valueIt)->getDataId()]->setAbsentAfterAbsentValuesMetAndAbsentUsed(nextAttributeIt, nextAbsentValueIntersectionIt);
      (*absentValueIntersectionIt)[(*valueIt)->getIntersectionId()] -= oldNoiseInHyperplane;
      oldNoise += oldNoiseInHyperplane;
    }
  end = currentAttribute.irrelevantEnd();
  // The first potential value actually is the value set absent and there is no noise to be found at the intersection of a vertex (seen as an outgoing vertex) and itself (seen as an ingoing vertex)
  while (++valueIt != end)
    {
      const unsigned int oldNoiseInHyperplane = hyperplanes[(*valueIt)->getDataId()]->setAbsentAfterAbsentValuesMetAndAbsentUsed(nextAttributeIt, nextAbsentValueIntersectionIt);
      (*absentValueIntersectionIt)[(*valueIt)->getIntersectionId()] -= oldNoiseInHyperplane;
      oldNoise += oldNoiseInHyperplane;
    }
  return oldNoise;
}

void Trie::absentFixAbsentValues(Attribute& currentAttribute, const vector<Attribute*>::iterator absentAttributeIt, const vector<unsigned int>& absentValueDataIds, const vector<Attribute*>::iterator nextAttributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const
{
  const vector<Value*>::iterator end = currentAttribute.absentEnd();
  for (vector<Value*>::iterator valueIt = currentAttribute.absentBegin(); valueIt != end; ++valueIt)
    {
      const unsigned int oldNoiseInHyperplane = hyperplanes[(*valueIt)->getDataId()]->setAbsentAfterAbsentUsed(absentAttributeIt, absentValueDataIds, nextAttributeIt, (*valueIt)->getIntersectionsBeginWithPresentAndPotentialValues());
      (*valueIt)->substractPotentialNoise(oldNoiseInHyperplane);
      const unsigned int valueId = (*valueIt)->getIntersectionId();
      for (vector<vector<unsigned int>>::iterator intersectionIt : intersectionIts)
	{
	  (*intersectionIt)[valueId] -= oldNoiseInHyperplane;
	}
    }
}

void Trie::absentFixAbsentValuesBeforeSymmetricAttributes(Attribute& currentAttribute, const vector<Attribute*>::iterator absentAttributeIt, const vector<Attribute*>::iterator nextAttributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const
{
  const vector<Value*>::iterator end = currentAttribute.absentEnd();
  for (vector<Value*>::iterator valueIt = currentAttribute.absentBegin(); valueIt != end; ++valueIt)
    {
      // Since this is before the symmetric attributes, hyperplanes necessarily are tries
      const unsigned int oldNoiseInHyperplane = static_cast<Trie*>(hyperplanes[(*valueIt)->getDataId()])->setSymmetricAbsentAfterAbsentUsed(absentAttributeIt, nextAttributeIt, (*valueIt)->getIntersectionsBeginWithPresentAndPotentialValues());
      (*valueIt)->substractPotentialNoise(oldNoiseInHyperplane);
      const unsigned int valueId = (*valueIt)->getIntersectionId();
      for (vector<vector<unsigned int>>::iterator intersectionIt : intersectionIts)
	{
	  (*intersectionIt)[valueId] -= oldNoiseInHyperplane;
	}
    }
}

void Trie::absentFixAbsentValuesAfterAbsentValuesMet(Attribute& currentAttribute, const vector<Attribute*>::iterator nextAttributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const
{
  const vector<Value*>::iterator end = currentAttribute.absentEnd();
  for (vector<Value*>::iterator valueIt = currentAttribute.absentBegin(); valueIt != end; ++valueIt)
    {
      const unsigned int oldNoiseInHyperplane = hyperplanes[(*valueIt)->getDataId()]->setAbsentAfterAbsentValuesMetAndAbsentUsed(nextAttributeIt, (*valueIt)->getIntersectionsBeginWithPresentAndPotentialValues());
      (*valueIt)->substractPotentialNoise(oldNoiseInHyperplane);
      const unsigned int valueId = (*valueIt)->getIntersectionId();
      for (vector<vector<unsigned int>>::iterator intersectionIt : intersectionIts)
	{
	  (*intersectionIt)[valueId] -= oldNoiseInHyperplane;
	}
    }
}

const double Trie::countNoise(const vector<vector<unsigned int>>& nSet) const
{
  double noise = 0;
  const vector<vector<unsigned int>>::const_iterator nextDimensionIt = nSet.begin() + 1;
  for (const unsigned int id : nSet.front())
    {
      noise += static_cast<double>(hyperplanes[id]->countNoise(nextDimensionIt));
    }
  return noise;
}

const unsigned int Trie::countNoise(const vector<vector<unsigned int>>::const_iterator dimensionIt) const
{
  unsigned int noise = 0;
  const vector<vector<unsigned int>>::const_iterator nextDimensionIt = dimensionIt + 1;
  for (const unsigned int id : *dimensionIt)
    {
      noise += hyperplanes[id]->countNoise(nextDimensionIt);
    }
  return noise;
}

const bool Trie::isBetterNSet(const double membershipThreshold, const vector<vector<unsigned int>>& nSet, vector<vector<unsigned int>::const_iterator>& tuple, double& membershipSum) const
{
  const vector<vector<unsigned int>>::const_iterator dimensionIt = nSet.begin();
  const vector<vector<unsigned int>::const_iterator>::iterator tupleIt = tuple.begin();
  const vector<vector<unsigned int>>::const_iterator nextDimensionIt = dimensionIt + 1;
  const vector<vector<unsigned int>::const_iterator>::iterator nextTupleIt = tupleIt + 1;
  for (; *tupleIt != dimensionIt->end(); ++*tupleIt)
    {
      if (hyperplanes[**tupleIt]->decreaseMembershipDownToThreshold(membershipThreshold, nextDimensionIt, nextTupleIt, membershipSum))
	{
	  if (*nextTupleIt == nextDimensionIt->end())
	    {
	      *nextTupleIt = nextDimensionIt->begin();
	      ++*tupleIt;
	    }
	  return false;
	}
    }
  return true;
}

const bool Trie::decreaseMembershipUpToThreshold(const double membershipThreshold, const vector<vector<unsigned int>>::const_iterator dimensionIt, const vector<vector<unsigned int>::const_iterator>::iterator tupleIt, double& membershipSum) const
{
  const vector<vector<unsigned int>>::const_iterator nextDimensionIt = dimensionIt + 1;
  const vector<vector<unsigned int>::const_iterator>::iterator nextTupleIt = tupleIt + 1;
  for (; *tupleIt != dimensionIt->end(); ++*tupleIt)
    {
      if (hyperplanes[**tupleIt]->decreaseMembershipDownToThreshold(membershipThreshold, nextDimensionIt, nextTupleIt, membershipSum))
	{
	  if (*nextTupleIt == nextDimensionIt->end())
	    {
	      *nextTupleIt = nextDimensionIt->begin();
	      ++*tupleIt;
	    }
	  return true;
	}
    }
  *tupleIt = dimensionIt->begin();
  return false;
}

#ifdef ASSERT
const unsigned int Trie::countNoiseOnPresent(const vector<Attribute*>::const_iterator valueAttributeIt, const Value& value, const vector<Attribute*>::const_iterator attributeIt) const
{
  const vector<Attribute*>::const_iterator nextAttributeIt = attributeIt + 1;
  if (attributeIt == valueAttributeIt)
    {
      return hyperplanes[value.getDataId()]->countNoiseOnPresent(valueAttributeIt, value, nextAttributeIt);
    }
  unsigned int noise = 0;
  const vector<Value*>::const_iterator end = (*attributeIt)->presentEnd();
  for (vector<Value*>::const_iterator valueIt = (*attributeIt)->presentBegin(); valueIt != end; ++valueIt)
    {
      noise += hyperplanes[(*valueIt)->getDataId()]->countNoiseOnPresent(valueAttributeIt, value, nextAttributeIt);
    }
  return noise;
}

const unsigned int Trie::countNoiseOnPresentAndPotential(const vector<Attribute*>::const_iterator valueAttributeIt, const Value& value, const vector<Attribute*>::const_iterator attributeIt) const
{
  const vector<Attribute*>::const_iterator nextAttributeIt = attributeIt + 1;
  if (attributeIt == valueAttributeIt)
    {
      return hyperplanes[value.getDataId()]->countNoiseOnPresentAndPotential(valueAttributeIt, value, nextAttributeIt);
    }
  unsigned int noise = 0;
  const vector<Value*>::const_iterator end = (*attributeIt)->irrelevantEnd();
  for (vector<Value*>::const_iterator valueIt = (*attributeIt)->presentBegin(); valueIt != end; ++valueIt)
    {
      noise += hyperplanes[(*valueIt)->getDataId()]->countNoiseOnPresentAndPotential(valueAttributeIt, value, nextAttributeIt);
    }
  return noise;
}
#endif

vector<vector<vector<unsigned int>>::iterator> Trie::incrementIterators(const vector<vector<vector<unsigned int>>::iterator>& iterators)
{
  vector<vector<vector<unsigned int>>::iterator> nextIterators;
  nextIterators.reserve(iterators.size());
  for (const vector<vector<unsigned int>>::iterator it : iterators)
    {
      nextIterators.push_back(it + 1);
    }
  return nextIterators;
}
