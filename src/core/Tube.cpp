// Copyright 2010,2011,2012,2013,2014,2015 Loïc Cerf (lcerf@dcc.ufmg.br)

// This file is part of multidupehack.

// multidupehack is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License version 3 as published by the Free Software Foundation

// multidupehack is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along with multidupehack; if not, write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

#include "Tube.h"

Tube::~Tube()
{
}

const unsigned int Tube::setSelfLoopsAfterSymmetricAttributes(const vector<Attribute*>::iterator attributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts)
{
  // Return 0 to trigger a switch to dense storage
  for (unsigned int hyperplaneId = 0; hyperplaneId != (*attributeIt)->sizeOfPresentAndPotential(); ++hyperplaneId)
    {
      for (vector<vector<unsigned int>>::iterator intersectionIt : intersectionIts)
	{
	  (*intersectionIt)[hyperplaneId] -= Attribute::noisePerUnit;
	}
      (*attributeIt)->substractPotentialNoise(hyperplaneId, Attribute::noisePerUnit);
    }
  return 0;
}

const unsigned int Tube::setPresentAfterPresentValueMet(const vector<Attribute*>::iterator attributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const
{
  presentFixPotentialOrAbsentValuesAfterPresentValueMet(**attributeIt, intersectionIts);
  return presentFixPresentValuesAfterPresentValueMet(**attributeIt);
}

const unsigned int Tube::setSymmetricPresentAfterPresentValueMet(const vector<Attribute*>::iterator attributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const
{
  // *this necessarily relates to the second symmetric attribute
  presentFixPotentialOrAbsentValuesInSecondSymmetricAttribute(**attributeIt, intersectionIts);
  return presentFixPresentValuesAfterPresentValueMet(**attributeIt);
}

const unsigned int Tube::setPresentAfterPresentValueMetAndPotentialOrAbsentUsed(const vector<Attribute*>::iterator attributeIt, const vector<vector<unsigned int>>::iterator potentialOrAbsentValueIntersectionIt) const
{
  return presentFixPresentValuesAfterPresentValueMetAndPotentialOrAbsentUsed(**attributeIt, potentialOrAbsentValueIntersectionIt);
}

const unsigned int Tube::setAbsentAfterAbsentValuesMet(const vector<Attribute*>::iterator attributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const
{
  absentFixAbsentValuesAfterAbsentValuesMet(**attributeIt, intersectionIts);
  return absentFixPresentOrPotentialValuesAfterAbsentValuesMet(**attributeIt, intersectionIts);
}

const unsigned int Tube::setSymmetricAbsentAfterAbsentValueMet(const vector<Attribute*>::iterator attributeIt, vector<vector<vector<unsigned int>>::iterator>& intersectionIts) const
{
  absentFixAbsentValuesAfterAbsentValuesMet(**attributeIt, intersectionIts);
  return absentFixPresentOrPotentialValuesInSecondSymmetricAttribute(**attributeIt, intersectionIts);
}

const unsigned int Tube::setAbsentAfterAbsentValuesMetAndAbsentUsed(const vector<Attribute*>::iterator attributeIt, const vector<vector<unsigned int>>::iterator absentValueIntersectionIt) const
{
  return absentFixPresentOrPotentialValuesAfterAbsentValuesMetAndAbsentUsed(**attributeIt, absentValueIntersectionIt);
}

const unsigned int Tube::setSymmetricAbsentAfterAbsentValueMetAndAbsentUsed(const vector<Attribute*>::iterator attributeIt, const vector<vector<unsigned int>>::iterator absentValueIntersectionIt) const
{
  return absentFixPresentOrPotentialValuesInSecondSymmetricAttributeAfterAbsentUsed(**attributeIt, absentValueIntersectionIt);
}
