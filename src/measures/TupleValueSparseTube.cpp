// Copyright 2014 Loïc Cerf (lcerf@dcc.ufmg.br)

// This file is part of multidupehack.

// multidupehack is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License version 3 as published by the Free Software Foundation

// multidupehack is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along with multidupehack; if not, write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

#include "TupleValueSparseTube.h"

TupleValueSparseTube::TupleValueSparseTube() : tube()
{
}

TupleValueSparseTube* TupleValueSparseTube::clone() const
{
  return new TupleValueSparseTube(*this);
}

const bool TupleValueSparseTube::setTupleValues(const vector<vector<unsigned int>>::const_iterator dimensionIt, const double value, const unsigned int sizeThreshold, const unsigned int lastDimensionCardinality)
{
  if (value == 0)
    {
      return false;
    }
  for (const unsigned int hyperplaneId : *dimensionIt)
    {
      tube[hyperplaneId] = value;
    }
  return tube.bucket_count() * sizeof(unsigned int) + tube.size() * (sizeof(double) + 2 * sizeof(double*)) > sizeThreshold; // In the worst case (all values in the same bucket), the unordered_map<unsigned int, double> takes more space than a vector<double> * densityThreshold
}

unordered_map<unsigned int, double>::const_iterator TupleValueSparseTube::begin() const
{
  return tube.begin();
}

unordered_map<unsigned int, double>::const_iterator TupleValueSparseTube::end() const
{
  return tube.end();
}

void TupleValueSparseTube::setSum(double& sum) const
{
  for (const pair<unsigned int, double>& keyValue : tube)
    {
      sum += keyValue.second;
    }
}

void TupleValueSparseTube::decreaseSum(const vector<unsigned int>& dimension, double& sum) const
{
  for (const unsigned int element : dimension)
    {
      const unordered_map<unsigned int, double>::const_iterator keyValue = tube.find(element);
      if (keyValue != tube.end())
	{
	  sum -= keyValue->second;
	}
    }
}
