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
  static pair<list<Node*>::const_iterator, list<Node*>::const_iterator> agglomerateAndSelect(const Trie* data, const double maximalNbOfCandidateAgglomerates);

 protected:
  struct MorePromisingParent
  {
    const bool operator()(const Node* node1, const Node* node2)
    {
      return node1->gEstimation > node2->gEstimation || (node1->gEstimation == node2->gEstimation && node1 < node2);
    }
  };

  vector<vector<unsigned int>> pattern;
  double membershipSum;		/* only relevant for candidates and nodes in dendrogramFrontier to estimate g */
  unsigned int area;		/* only relevant for candidates and nodes in dendrogramFrontier to estimate g */
  double g;
  double gEstimation;					  /* only relevant for candidates */
  vector<vector<unsigned int>::const_iterator> nextTuple; /* only relevant for candidates */
  set<Node*, MorePromisingParent> parents; /* only relevant for nodes in dendrogramFrontier, empty otherwise */
  set<Node*>::iterator lastGoodParentIt; /* only relevant for nodes in dendrogramFrontier, all parents up to **lastGoodParentIt have this as a first generating child; all parents after lastGoodParentIt do not have this as a first generating child */
  vector<list<Node*>::iterator> children; /* if *this is candidate, the pairs of nodes in dendrogramFrontier that generate it and it is one of the good candidate for every first child of a pair; if inserted in dendrogram frontier, its subsets in the dendrogram */

  static double maxMembershipMinusSimilarityShift;
  static unsigned int nbOfGoodParents;

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

  /* static const bool morePromisingParent(const Node* node1, const Node* node2); */
  static const bool morePromising(const Node* node1, const Node* node2);
  static const bool moreRelevant(const Node* node1, const Node* node2);
  static void constructCandidate(const list<Node*>::iterator otherChildIt, const list<Node*>::iterator thisIt);
  static vector<unsigned int> idVectorUnion(const vector<unsigned int>& v1, const vector<unsigned int>& v2);

#ifdef DEBUG_HA
  void print(ostream& out) const;
#endif
};

#endif	/*NODE_H*/
