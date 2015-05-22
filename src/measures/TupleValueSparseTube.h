// Copyright 2014 Loïc Cerf (lcerf@dcc.ufmg.br)

// This file is part of multidupehack.

// multidupehack is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License version 3 as published by the Free Software Foundation

// multidupehack is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along with multidupehack; if not, write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

#ifndef TUPLE_VALUE_SPARSE_TUBE_H
#define TUPLE_VALUE_SPARSE_TUBE_H

#include <unordered_map>

#include "TupleValueTube.h"

class TupleValueSparseTube : public TupleValueTube
{
 public:
  TupleValueSparseTube();

  unordered_map<unsigned int, double>::const_iterator begin() const;
  unordered_map<unsigned int, double>::const_iterator end() const;
  const bool setTupleValues(const vector<vector<unsigned int>>::const_iterator dimensionIt, const double value, const unsigned int sizeThreshold, const unsigned int lastDimensionCardinality);
  void setSum(double& sum) const;
  void decreaseSum(const vector<unsigned int>& dimension, double& sum) const;
 protected:
  unordered_map<unsigned int, double> tube;

  TupleValueSparseTube* clone() const;
};
#endif /*TUPLE_VALUE_SPARSE_TUBE_H*/
