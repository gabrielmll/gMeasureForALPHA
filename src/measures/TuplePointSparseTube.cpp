// Copyright 2014 Loïc Cerf (lcerf@dcc.ufmg.br)

// This file is part of multidupehack.

// multidupehack is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License version 3 as published by the Free Software Foundation

// multidupehack is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along with multidupehack; if not, write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

#include "TuplePointSparseTube.h"

TuplePointSparseTube::TuplePointSparseTube() : tube()
{
}

TuplePointSparseTube* TuplePointSparseTube::clone() const
{
  return new TuplePointSparseTube(*this);
}

pair<double, double> TuplePointSparseTube::getPoint(const unsigned int hyperplaneId) const
{
  const unordered_map<unsigned int, pair<double, double>>::const_iterator entryIt = tube.find(hyperplaneId);
  if (entryIt == tube.end())
    {
      return pair<double, double>(nan(""), nan(""));
    }
  return entryIt->second;
}

const bool TuplePointSparseTube::setTuplePoints(const vector<vector<unsigned int>>::const_iterator dimensionIt, const pair<double, double>& point, const unsigned int sizeThreshold, const unsigned int lastDimensionCardinality)
{
  for (const unsigned int hyperplaneId : *dimensionIt)
    {
      tube[hyperplaneId] = point;
    }
  return tube.bucket_count() * sizeof(unsigned int) + tube.size() * (sizeof(pair<double, double>) + 2 * sizeof(pair<double, double>*)) > sizeThreshold; // In the worst case (all values in th same bucket), the unordered_map<unsigned int, pair<double, double>> takes more space than a vector<pair<double, double>> * densityThreshold
}

void TuplePointSparseTube::minCoordinates(double& minX, double& minY) const
{
  for (const pair<unsigned int, pair<double, double>>& entry : tube)
    {
      if (entry.second.first < minX)
	{
	  minX = entry.second.first;
	}
      if (entry.second.second < minY)
	{
	  minY = entry.second.second;
	}
    }
}

void TuplePointSparseTube::translate(const double deltaX, const double deltaY)
{
  for (pair<const unsigned int, pair<double, double>>& entry : tube)
    {
      entry.second.first += deltaX;
      entry.second.second += deltaY;
    }
}

void TuplePointSparseTube::setSlopeSums(SlopeSums& slopeSums) const
{
  slopeSums.nbOfPoints += tube.size();
  for (const pair<unsigned int, pair<double, double>>& entry : tube)
    {
      slopeSums.sumX += entry.second.first;
      slopeSums.sumXSquared += entry.second.first * entry.second.first;
      slopeSums.sumY += entry.second.second;
      slopeSums.sumXY += entry.second.first * entry.second.second;
    }
}

void TuplePointSparseTube::increaseSlopeSums(const vector<unsigned int>& dimension, SlopeSums& slopeSums) const
{
  for (const unsigned int element : dimension)
    {
      const unordered_map<unsigned int, pair<double, double>>::const_iterator entryIt = tube.find(element);
      if (entryIt != tube.end())
	{
	  ++slopeSums.nbOfPoints;
	  slopeSums.sumX += entryIt->second.first;
	  slopeSums.sumXSquared += entryIt->second.first * entryIt->second.first;
	  slopeSums.sumY += entryIt->second.second;
	  slopeSums.sumXY += entryIt->second.first * entryIt->second.second;
	}
    }
}

void TuplePointSparseTube::decreaseSlopeSums(const vector<unsigned int>& dimension, SlopeSums& slopeSums) const
{
  for (const unsigned int element : dimension)
    {
      const unordered_map<unsigned int, pair<double, double>>::const_iterator entryIt = tube.find(element);
      if (entryIt != tube.end())
	{
	  --slopeSums.nbOfPoints;
	  slopeSums.sumX -= entryIt->second.first;
	  slopeSums.sumXSquared -= entryIt->second.first * entryIt->second.first;
	  slopeSums.sumY -= entryIt->second.second;
	  slopeSums.sumXY -= entryIt->second.first * entryIt->second.second;
	}
    }
}
