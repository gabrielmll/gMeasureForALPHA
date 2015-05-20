// Copyright 2010,2011,2012,2013,2014,2015 Loïc Cerf (lcerf@dcc.ufmg.br)

// This file is part of multidupehack.

// multidupehack is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License version 3 as published by the Free Software Foundation

// multidupehack is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along with multidupehack; if not, write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

#ifndef DENSE_CRISP_TUBE_H_
#define DENSE_CRISP_TUBE_H_

#include <boost/dynamic_bitset.hpp>

#include "SparseCrispTube.h"

class DenseCrispTube: public Tube
{
 public:
  DenseCrispTube(const SparseCrispTube& sparseCrispTube, const unsigned int nbOfHyperplanes);

  void print(vector<unsigned int>& prefix, ostream& out) const;
  const bool setTuple(const vector<unsigned int>& tuple, const unsigned int membership, vector<unsigned int>::const_iterator attributeIdIt, vector<vector<unsigned int>>::const_iterator oldIds2NewIdsIt, const vector<Attribute*>::iterator attributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts);
  const unsigned int setSelfLoopsInSymmetricAttribute(const unsigned int hyperplaneId, const unsigned int lastSymmetricAttributeId, const vector<Attribute*>::iterator attributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts, const unsigned int dimensionId);

  const unsigned int setPresent(const vector<Attribute*>::iterator presentAttributeIt, const vector<Attribute*>::iterator attributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const;
  const unsigned int setPresentAfterPotentialOrAbsentUsed(const vector<Attribute*>::iterator presentAttributeIt, const vector<Attribute*>::iterator attributeIt, const vector<vector<unsigned int>>::iterator potentialOrAbsentValueIntersectionIt) const;
  const unsigned int setAbsent(const vector<Attribute*>::iterator absentAttributeIt, const vector<unsigned int>& absentValueDataIds, const vector<Attribute*>::iterator attributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const;
  const unsigned int setAbsentAfterAbsentUsed(const vector<Attribute*>::iterator absentAttributeIt, const vector<unsigned int>& absentValueDataIds, const vector<Attribute*>::iterator attributeIt, const vector<vector<unsigned int>>::iterator absentValueIntersectionIt) const;

  const unsigned int countNoise(const vector<vector<unsigned int>>::const_iterator dimensionIt) const;
  const bool decreaseMembershipDownToThreshold(const double membershipThreshold, const vector<vector<unsigned int>>::const_iterator dimensionIt, const vector<vector<unsigned int>::const_iterator>::iterator tupleIt, double& membershipSum) const;

#ifdef ASSERT
  const unsigned int countNoiseOnPresent(const vector<Attribute*>::const_iterator valueAttributeIt, const Value& value, const vector<Attribute*>::const_iterator attributeIt) const;
  const unsigned int countNoiseOnPresentAndPotential(const vector<Attribute*>::const_iterator valueAttributeIt, const Value& value, const vector<Attribute*>::const_iterator attributeIt) const;
#endif

 protected:
  dynamic_bitset<> tube;

  DenseCrispTube* clone() const;

  const unsigned int noiseOnValues(const vector<Attribute*>::const_iterator attributeIt, const vector<unsigned int>& valueIds) const;

  const unsigned int presentFixPresentValuesAfterPresentValueMet(Attribute& currentAttribute) const;
  const unsigned int presentFixPresentValuesAfterPresentValueMetAndPotentialOrAbsentUsed(Attribute& currentAttribute, const vector<vector<unsigned int>>::iterator potentialOrAbsentValueIntersectionIt) const;
  void presentFixPotentialOrAbsentValuesAfterPresentValueMet(Attribute& currentAttribute, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const;
  void presentFixPotentialOrAbsentValuesInSecondSymmetricAttribute(Attribute& currentAttribute, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const;
  const unsigned int absentFixPresentOrPotentialValuesAfterAbsentValuesMet(Attribute& currentAttribute, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const;
  const unsigned int absentFixPresentOrPotentialValuesInSecondSymmetricAttribute(Attribute& currentAttribute, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const;
  const unsigned int absentFixPresentOrPotentialValuesAfterAbsentValuesMetAndAbsentUsed(Attribute& currentAttribute, const vector<vector<unsigned int>>::iterator absentValueIntersectionIt) const;
  const unsigned int absentFixPresentOrPotentialValuesInSecondSymmetricAttributeAfterAbsentUsed(Attribute& currentAttribute, const vector<vector<unsigned int>>::iterator absentValueIntersectionIt) const;
  void absentFixAbsentValuesAfterAbsentValuesMet(Attribute& currentAttribute, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const;
};

#endif /*DENSE_CRISP_TUBE_H_*/
