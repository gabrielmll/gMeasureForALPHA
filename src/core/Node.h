// Copyright 2013,2014 Loïc Cerf (lcerf@dcc.ufmg.br)

// This file is part of multidupehack.

// multidupehack is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License version 3 as published by the Free Software Foundation

// multidupehack is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along with multidupehack; if not, write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

#ifndef NODE_H_
#define NODE_H_

#include <set>
#include <iostream>

#include "../utilities/list_iterator_hash.h"
#include "Trie.h"

class Node
{
 public:
  Node(const vector<Attribute*>& attributes);
  Node(const vector<vector<unsigned int>>& nSet, const Trie* data);

  const vector<unsigned int>& dimension(const unsigned int dimensionId) const;
  const unsigned int getArea() const;

  static void setSimilarityShift(const double similarityShift);
  static void setMaximalNbOfClosedNSetsForAgglomeration(const unsigned int maximalNbOfClosedNSetsForAgglomeration);
  static void insertOrDelete(Node* leaf);
  static pair<list<Node*>::const_iterator, list<Node*>::const_iterator> agglomerateAndSelect(const Trie* data);

 protected:
  vector<vector<unsigned int>> pattern;
  double membershipSum;
  unsigned int area;
  double g;
  double gEstimation;  
  vector<vector<unsigned int>::const_iterator> nextTuple;
  unordered_set<Node*> parents;
  vector<list<Node*>::iterator> children;

  static double maxMembershipMinusSimilarityShift;
  static unsigned int maximalNbOfClosedNSets;
  static bool isBelowMaximalNbOfClosedNSets;
  static double smallestG;

  static multiset<Node*, const bool(*)(const Node*, const Node*)> leaves;
  static list<Node*> dendrogram;
  static list<Node*> dendrogramFrontier;
  static set<Node*, const bool(*)(const Node*, const Node*)> candidates;
  static unordered_map<vector<vector<unsigned int>>, Node*, vector_hash<vector<unsigned int>>> candidateNSets;

  Node(const vector<vector<unsigned int>>& nSet, const list<Node*>::iterator child1, const list<Node*>::iterator child2);

  void computeG();
  const double gEstimationFromLastTwoChildren() const;
  const unsigned int countFutureChildren(const double ancestorG) const;

  void unlinkGeneratingPairsInvolving(const Node* child);
  void deleteOffspringWithSmallerG(const double ancestorG, vector<list<Node*>::iterator>& ancestorChildren);
  vector<list<Node*>::iterator> getParentChildren();
  void insertInDendrogramFrontier();

  static const bool lessPromising(const Node* node1, const Node* node2);
  static const bool morePromising(const Node* node1, const Node* node2);
  static const bool moreRelevant(const Node* node1, const Node* node2);
  static void constructCandidate(const list<Node*>::iterator otherChildIt, const list<Node*>::iterator thisIt);
  static vector<unsigned int> idVectorUnion(const vector<unsigned int>& v1, const vector<unsigned int>& v2);

#ifdef DEBUG_HA
  void print(ostream& out) const;
#endif
};

#endif	/*NODE_H*/
