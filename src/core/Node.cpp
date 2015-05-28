// Copyright 2013,2014 Loïc Cerf (lcerf@dcc.ufmg.br)

// This file is part of multidupehack.

// multidupehack is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License version 3 as published by the Free Software Foundation

// multidupehack is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along with multidupehack; if not, write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

#include "Node.h"

double Node::maxMembershipMinusSimilarityShift;
unsigned int Node::maximalNbOfClosedNSets;
bool Node::isBelowMaximalNbOfClosedNSets = true;
double Node::smallestG = -numeric_limits<double>::infinity();

multiset<Node*, const bool(*)(const Node*, const Node*)> Node::leaves(lessPromising);
list<Node*> Node::dendrogram;
list<Node*> Node::dendrogramFrontier;
set<Node*, const bool(*)(const Node*, const Node*)> Node::candidates(morePromising);
unordered_map<vector<vector<unsigned int>>, Node*, vector_hash<vector<unsigned int>>> Node::candidateNSets;

Node::Node(const vector<Attribute*>& attributes): pattern(), membershipSum(0), area(1), g(0), gEstimation(0), nextTuple(), parents(), children()
{
  pattern.reserve(attributes.size());
  nextTuple.reserve(attributes.size());
  for (const Attribute* attribute : attributes)
    {
      pattern.push_back(attribute->getPresentDataIds());
      sort(pattern.back().begin(), pattern.back().end());
      area *= pattern.back().size();
      nextTuple.push_back(pattern.back().begin());
    }
  membershipSum = maxMembershipMinusSimilarityShift * area - attributes.front()->totalPresentAndPotentialNoise();
  computeG();
}

Node::Node(const vector<vector<unsigned int>>& nSet, const Trie* data): pattern(nSet), membershipSum(0), area(1), g(0), gEstimation(0), nextTuple(), parents(), children()
{
  nextTuple.reserve(nSet.size());
  for (vector<unsigned int>& patternDimension : pattern)
    {
      sort(patternDimension.begin(), patternDimension.end());
      area *= patternDimension.size();
      nextTuple.push_back(patternDimension.begin());
    }
  membershipSum = maxMembershipMinusSimilarityShift * area - data->countNoise(pattern);
  computeG();
}

Node::Node(const vector<vector<unsigned int>>& nSet, const list<Node*>::iterator child1It, const list<Node*>::iterator child2It): pattern(nSet), membershipSum(0), area(1), g(0), gEstimation(0), nextTuple(), parents(), children {child1It, child2It}
{
  nextTuple.reserve(nSet.size());
  for (const vector<unsigned int>& patternDimension : pattern)
    {
      area *= patternDimension.size();
      nextTuple.push_back(patternDimension.begin());
    }
  membershipSum = maxMembershipMinusSimilarityShift * area;
  computeG();
  gEstimation = gEstimationFromLastTwoChildren();
}

const vector<unsigned int>& Node::dimension(const unsigned int dimensionId) const
{
  return pattern[dimensionId];
}

const unsigned int Node::getArea() const
{
  return area;
}

void Node::constructCandidate(const list<Node*>::iterator child1It, const list<Node*>::iterator child2It)
{
  vector<vector<unsigned int>> unionNSet;
  unionNSet.reserve((*child1It)->pattern.size());
  vector<vector<unsigned int>>::const_iterator otherPatternIt = (*child2It)->pattern.begin();
  for (const vector<unsigned int>& patternDimension : (*child1It)->pattern)
    {
      unionNSet.push_back(idVectorUnion(*otherPatternIt, patternDimension));
      ++otherPatternIt;
    }
  Node* candidate;
  const unordered_map<vector<vector<unsigned int>>, Node*, vector_hash<vector<unsigned int>>>::iterator candidateNSetIt = candidateNSets.find(unionNSet);
  if (candidateNSetIt == candidateNSets.end())
    {
      candidate = new Node(unionNSet, child1It, child2It);
      candidateNSets[unionNSet] = candidate;
      candidates.insert(candidates.begin(), candidate);
      (*child1It)->parents.insert(candidate);
      (*child2It)->parents.insert(candidate);
      return;
    }
  candidate = candidateNSetIt->second;
  candidate->children.push_back(child1It);
  if (candidate != *child2It)
    {
      candidate->children.push_back(child2It);
      const double newGEstimation = candidate->gEstimationFromLastTwoChildren();
      if (newGEstimation > candidate->gEstimation)
	{
	  const set<Node*>::const_iterator hintIt = candidates.erase(candidates.find(candidate));
	  candidate->gEstimation = newGEstimation;
	  candidates.insert(hintIt, candidate);
	}
      (*child1It)->parents.insert(candidate);
      (*child2It)->parents.insert(candidate);
    }
}

void Node::unlinkGeneratingPairsInvolving(const Node* child)
{
  unordered_set<Node*> otherComponentsOfErasedPairs;
  for (vector<list<Node*>::iterator>::iterator otherChildItIt = children.begin(); otherChildItIt != children.end(); )
    {
      if (**otherChildItIt == child)
	{
	  // child is the first component of the generating pair
	  if (otherChildItIt + 2 == children.end())
	    {
	      otherComponentsOfErasedPairs.insert(*children.back());
	      children.pop_back();
	      children.pop_back();
	      break;
	    }
	  *otherChildItIt = children.back();
	  children.pop_back();
	  vector<list<Node*>::iterator>::iterator otherComponentOfErasedPairsIt = otherChildItIt + 1;
	  otherComponentsOfErasedPairs.insert(**otherComponentOfErasedPairsIt);
	  *otherComponentOfErasedPairsIt = children.back();
	  children.pop_back();
	}
      else
	{
	  if (**++otherChildItIt == child)
	    {
	      // child is the second component of the generating pair
	      if (otherChildItIt + 1 == children.end())
		{
		  children.pop_back();
		  otherComponentsOfErasedPairs.insert(*children.back());
		  children.pop_back();
		  break;
		}
	      *otherChildItIt = children.back();
	      children.pop_back();
	      otherComponentsOfErasedPairs.insert(**--otherChildItIt);
	      *otherChildItIt = children.back();
	      children.pop_back();
	    }
	  else
	    {
	      // child is not a component of the generating pair
	      ++otherChildItIt;
	    }
	}
    }
  // Do not unlink a child if still in a valid generating pair
  for (const list<Node*>::iterator childIt : children)
    {
      otherComponentsOfErasedPairs.erase(*childIt);
    }
  // Unlink the rest of them
  for (Node* otherChild : otherComponentsOfErasedPairs)
    {
      otherChild->parents.erase(this);
    }
}

void Node::computeG()
{
  if (membershipSum < 0)
    {
      g = -membershipSum * membershipSum / area;
      return;
    }
  g = membershipSum * membershipSum / area;
}

const double Node::gEstimationFromLastTwoChildren() const
{
  const Node& child1 = ***(children.end() - 2);
  const Node& child2 = **children.back();
  unsigned int nbOfTuplesInIntersection = 1;
  vector<vector<unsigned int>>::const_iterator child1DimensionIt = child1.pattern.begin();
  vector<vector<unsigned int>>::const_iterator child2DimensionIt = child2.pattern.begin();
  for (const vector<unsigned int>& patternDimension : pattern)
    {
      nbOfTuplesInIntersection *= child1DimensionIt->size() + child2DimensionIt->size() - patternDimension.size();
    }
  const double child1Density = child1.membershipSum / child1.area;
  const double child2Density = child2.membershipSum / child2.area;
  if (child1Density < child2Density)
    {
      double membershipSumEstimation = child2.membershipSum;
      const double estimatedSumAtIntersection = child2Density * nbOfTuplesInIntersection;
      if (estimatedSumAtIntersection < child1.membershipSum)
	{
	  membershipSumEstimation += child1.membershipSum - estimatedSumAtIntersection;
	}
      if (membershipSumEstimation < 0)
	{
	  return -membershipSumEstimation * membershipSumEstimation / area;
	}
      return membershipSumEstimation * membershipSumEstimation / area;
    }
  double membershipSumEstimation = child1.membershipSum;
  const double estimatedSumAtIntersection = child1Density * nbOfTuplesInIntersection;
  if (estimatedSumAtIntersection < child2.membershipSum)
    {
      membershipSumEstimation += child2.membershipSum - estimatedSumAtIntersection;
    }
  if (membershipSumEstimation < 0)
    {
      return -membershipSumEstimation * membershipSumEstimation / area;
    }
  return membershipSumEstimation * membershipSumEstimation / area;
}

const unsigned int Node::countFutureChildren(const double ancestorG) const
{
  if (children.empty())
    {
      return 0;
    }
  unsigned int nbOfCoveredLeaves = 0;
  for (const list<Node*>::iterator childIt : children)
    {
      if ((*childIt)->g > ancestorG)
	{
	  ++nbOfCoveredLeaves;
	}
      else
	{
	  nbOfCoveredLeaves += (*childIt)->countFutureChildren(ancestorG);
	}
    }
  return nbOfCoveredLeaves;
}

void Node::deleteOffspringWithSmallerG(const double ancestorG, vector<list<Node*>::iterator>& ancestorChildren)
{
  for (list<Node*>::iterator childIt : children)
    {
      if ((*childIt)->g > ancestorG)
	{
	  ancestorChildren.push_back(childIt);
	}
      else
	{
	  (*childIt)->deleteOffspringWithSmallerG(ancestorG, ancestorChildren);
	  delete *childIt;
	  dendrogram.erase(childIt);
	}
    }
}

vector<list<Node*>::iterator> Node::getParentChildren()
{
  bool isIrrelevant = true;
  const vector<list<Node*>::iterator>::reverse_iterator rend = children.rend();
  for (vector<list<Node*>::iterator>::reverse_iterator childItIt = children.rbegin(); childItIt != rend; )
    {
      if ((**childItIt)->g > g)
	{
	  ++childItIt;
	}
      else
	{
	  isIrrelevant = false;
	  (**childItIt)->deleteOffspringWithSmallerG(g, children);
	  delete **childItIt;
	  dendrogram.erase(*childItIt);
	  *childItIt++ = children.back();
	  children.pop_back();
	}
    }
  if (isIrrelevant)
    {
      return std::move(children);
    }
  return vector<list<Node*>::iterator>();
}

void Node::insertInDendrogramFrontier()
{
  children.clear();
  // *this goes to dendrogramFrontier
  dendrogramFrontier.push_back(this);
  // Construct the new candidates or add children to it if already constructed
  const list<Node*>::iterator thisIt = --dendrogramFrontier.end();
  for (list<Node*>::iterator otherChildIt = dendrogramFrontier.begin(); otherChildIt != thisIt; ++otherChildIt)
    {
      // NB: subsets of *this are found during candidate construction and added to children
      constructCandidate(otherChildIt, thisIt);
    }
#ifdef DEBUG_HA
  cout << "  " << children.size() << " children" << endl;
#endif
  // Remove this n-set from candidateNSets
  candidateNSets.erase(pattern);
  unsigned int nbOfFutureChildren = 1; // + 1 because, in deleteOffspringWithSmallerG, new children are inserted before removing the one that became irrelevant
  vector<list<Node*>::iterator> newChildren;
  for (const list<Node*>::iterator childIt : children)
    {
      // Unlink **childIt from *this and its other parents
      (*childIt)->parents.erase(this);
      for (Node* parent : (*childIt)->parents)
	{
	  parent->unlinkGeneratingPairsInvolving(*childIt);
	  if (parent->children.empty())
	    {
	      candidateNSets.erase(parent->pattern);
	      candidates.erase(parent);
	      delete parent;
	    }
	}
      // Compute the number of future children to **childIt allocate enough space for this children (essential to guarantee no reallocation in getParentChildren)
      const unsigned int nbOfFutureChildrenBelowChild = (*childIt)->countFutureChildren(g);
      if (nbOfFutureChildrenBelowChild == 0)
	{
	  ++nbOfFutureChildren;
	}
      else
	{
	  nbOfFutureChildren += nbOfFutureChildrenBelowChild;
	}
      // Compute the new children, which are not the future children yet because, when this isIrrelevant will be computed in getParentChildren, there must be at least one child per set of covered leaves
      const vector<list<Node*>::iterator> childrenOfOneChildWithAtLeastItsG = (*childIt)->getParentChildren();
      if (childrenOfOneChildWithAtLeastItsG.empty())
	{
	  dendrogram.push_back(*childIt);
	  newChildren.push_back(--dendrogram.end());
	}
      else
	{
	  delete *childIt;
	  newChildren.insert(newChildren.end(), childrenOfOneChildWithAtLeastItsG.begin(), childrenOfOneChildWithAtLeastItsG.end());
	}
      dendrogramFrontier.erase(childIt);
    }
  children = std::move(newChildren);
  children.reserve(nbOfFutureChildren);
}

const bool Node::lessPromising(const Node* node1, const Node* node2)
{
  return node1->g < node2->g;
}

const bool Node::morePromising(const Node* node1, const Node* node2)
{
  return node1->g > node2->g || (node1->g == node2->g && (node1->gEstimation > node2->gEstimation || (node1->gEstimation == node2->gEstimation && node1 < node2)));
}

const bool Node::moreRelevant(const Node* node1, const Node* node2)
{
  return node2->g < node1->g;
}

void Node::setSimilarityShift(const double similarityShift)
{
  maxMembershipMinusSimilarityShift = similarityShift + Attribute::noisePerUnit;
}

void Node::setMaximalNbOfClosedNSetsForAgglomeration(const unsigned int maximalNbOfClosedNSetsForAgglomeration)
{
  maximalNbOfClosedNSets = maximalNbOfClosedNSetsForAgglomeration;
}

void Node::insertOrDelete(Node* leaf)
{
  if (leaves.size() == maximalNbOfClosedNSets)
    {
      if (isBelowMaximalNbOfClosedNSets)
	{
	  isBelowMaximalNbOfClosedNSets = false;
	  cerr << "Warning: more than " << maximalNbOfClosedNSets << " closed ET-" << leaf->pattern.size() << "-sets" << endl << "Only retaining those with the smallest relative noises" << endl;
	}
      if (lessPromising(leaf, *leaves.begin()))
	{
	  delete leaf;
	  return;
	}
      smallestG = (*leaves.begin())->g;
      for (multiset<Node*>::iterator leafIt = leaves.begin(); leafIt != leaves.end() && (*leafIt)->g == smallestG; leafIt = leaves.erase(leafIt))
	{
	  delete *leafIt;
	}
    }
  if (leaf->g > smallestG)
    {
      leaves.insert(leaf);
      return;
    }
  delete leaf;
}

pair<list<Node*>::const_iterator, list<Node*>::const_iterator> Node::agglomerateAndSelect(const Trie* data)
{
  if (leaves.empty())
    {
      return pair<list<Node*>::const_iterator, list<Node*>::const_iterator>(dendrogram.begin(), dendrogram.begin());
    }
#ifdef DEBUG_HA
  cout << endl << "Dendrogram:" << endl << endl;
#endif
  // Candidate construction
  for (multiset<Node*>::iterator child1It = leaves.begin(); child1It != leaves.end(); child1It = leaves.erase(child1It))
    {
#ifdef DEBUG_HA
      (*child1It)->print(cout);
#endif
      dendrogramFrontier.push_front(*child1It);
      for (list<Node*>::iterator child2It = dendrogramFrontier.begin(); ++child2It != dendrogramFrontier.end(); )
	{
	  constructCandidate(dendrogramFrontier.begin(), child2It);
	}
    }
  // Hierarchical agglomeration
  while (!candidates.empty())
    {
      // Searching for the candidates with the smallest intrinsic distance and the largest area (in case of equality according to both criteria, the one with the smallest memory address is retained)
      double highestG = -numeric_limits<double>::infinity();
      while ((*candidates.begin())->nextTuple.front() != (*candidates.begin())->pattern.front().end())
    	{
    	  Node* candidate = *candidates.begin();
    	  // candidate->g is partial
    	  candidates.erase(candidates.begin());
    	  const bool isBetter = data->isBetterNSet(sqrt(highestG * candidate->area), candidate->pattern, candidate->nextTuple, candidate->membershipSum);
    	  candidate->computeG();
    	  if (isBetter)
    	    {
    	      highestG = candidate->g;
    	    }
    	  candidates.insert(candidate);
    	}
      Node* candidateToInsert = *candidates.begin();
#ifdef DEBUG_HA
      (*candidateToInsert).print(cout);
#endif
      candidates.erase(candidates.begin());
      candidateToInsert->insertInDendrogramFrontier();
    }
  Node* root = dendrogramFrontier.front();
  if (root->getParentChildren().empty())
    {
      dendrogram.push_back(root);
    }
  else
    {
      delete root;
    }
  // Order the nodes, more relevant first
  dendrogram.sort(moreRelevant);
  return pair<list<Node*>::const_iterator, list<Node*>::const_iterator>(dendrogram.begin(), dendrogram.end());
}

vector<unsigned int> Node::idVectorUnion(const vector<unsigned int>& v1, const vector<unsigned int>& v2)
{
  vector<unsigned int> unionVector;
  unionVector.reserve(v1.size() + v2.size());
  const vector<unsigned int>::const_iterator v1End = v1.end();
  const vector<unsigned int>::const_iterator v2End = v2.end();
  vector<unsigned int>::const_iterator v1It = v1.begin();
  vector<unsigned int>::const_iterator v2It = v2.begin();
  while (true)
    {
      if (*v1It < *v2It)
	{
	  unionVector.push_back(*v1It);
	  if (++v1It == v1End)
	    {
	      for (; v2It != v2End; ++v2It)
		{
		  unionVector.push_back(*v2It);
		}
	      return unionVector;
	    }
	}
      else
	{
	  if (*v1It == *v2It)
	    {
	      if (++v1It == v1End)
		{
		  for (; v2It != v2End; ++v2It)
		    {
		      unionVector.push_back(*v2It);
		    }
		  return unionVector;
		}
	      unionVector.push_back(*v2It);
	      if (++v2It == v2End)
		{
		  for (; v1It != v1End; ++v1It)
		    {
		      unionVector.push_back(*v1It);
		    }
		  return unionVector;
		}
	    }
	  else
	    {
	      unionVector.push_back(*v2It);
	      if (++v2It == v2End)
		{
		  for (; v1It != v1End; ++v1It)
		    {
		      unionVector.push_back(*v1It);
		    }
		  return unionVector;
		}
	    }
	}
    }
  return unionVector;
}

#ifdef DEBUG_HA
void Node::print(ostream& out) const
{
  unsigned int dimensionId = 0;
  bool isFirst = true;
  for (const vector<unsigned int>& dimension : pattern)
    {
      if (isFirst)
	{
	  isFirst = false;
	}
      else
	{
	  out << ' ';
	}
      Attribute::printValuesFromDataIds(dimension, dimensionId++, out);
    }
  const double noisePerUnitSquared = static_cast<double>(Attribute::noisePerUnit) * Attribute::noisePerUnit;
  out << endl << "  area: " << area << endl << "  membershipSum: " << membershipSum / Attribute::noisePerUnit << endl << "  g: " << g / noisePerUnitSquared << endl << "  gEstimation: " << gEstimation / noisePerUnitSquared << endl;
}
#endif
