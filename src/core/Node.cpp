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
  for (vector<Attribute*>::const_iterator attributeIt = attributes.begin(); attributeIt != attributes.end(); ++attributeIt)
    {
      pattern.push_back((*attributeIt)->getPresentDataIds());
      sort(pattern.back().begin(), pattern.back().end());
      area *= pattern.back().size();
      nextTuple.push_back(pattern.back().begin());
    }
#ifdef DEBUG_HA
  cout << endl << "maxMembershipMinusSimilarityShift: " << maxMembershipMinusSimilarityShift / Attribute::noisePerUnit;
  cout << endl << "totalPresentAndPotentialNoise: " << attributes.front()->totalPresentAndPotentialNoise() / Attribute::noisePerUnit;
#endif
  membershipSum = maxMembershipMinusSimilarityShift * area - attributes.front()->totalPresentAndPotentialNoise();
  g = membershipSum * membershipSum / area;

#ifdef DEBUG_HA
  print(cout);
#endif
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
  g = membershipSum * membershipSum / area;
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
  g = membershipSum * membershipSum / area;
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
    }
  else
    {
      candidate = candidateNSetIt->second;
      candidate->children.push_back(child1It);
      candidate->children.push_back(child2It);
      const double newGEstimation = candidate->gEstimationFromLastTwoChildren();
      if (newGEstimation > candidate->gEstimation)
	{
	  const set<Node*>::const_iterator hintIt = candidates.erase(candidates.find(candidate));
	  candidate->gEstimation = newGEstimation;
	  candidates.insert(hintIt, candidate);
	}
    }
  (*child1It)->parents.insert(candidate);
  (*child2It)->parents.insert(candidate);
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
      return membershipSumEstimation * membershipSumEstimation / area;
    }
  double membershipSumEstimation = child1.membershipSum;
  const double estimatedSumAtIntersection = child1Density * nbOfTuplesInIntersection;
  if (estimatedSumAtIntersection < child2.membershipSum)
    {
      membershipSumEstimation += child2.membershipSum - estimatedSumAtIntersection;
    }
  return membershipSumEstimation * membershipSumEstimation / area;
}

const unsigned int Node::countLeaves() const
{
  if (children.empty())
    {
      return 1;
    }
  unsigned int nbOfCoveredLeaves = 0;
  for (const list<Node*>::iterator childIt : children)
    {
      nbOfCoveredLeaves += (*childIt)->countLeaves();
    }
  return nbOfCoveredLeaves;
}
/*
 * Should it be countLeaves with G above?
const unsigned int Node::countLeavesWithRelevanceAbove(const int ancestorRelevance) const
{
  if (children.empty())
    {
      return 0;
    }
  unsigned int nbOfCoveredLeaves = 0;
  for (const list<Node*>::iterator childIt : children)
    {
      if ((*childIt)->relevance > ancestorRelevance)
	{
	  nbOfCoveredLeaves += (*childIt)->countLeaves();
	}
      else
	{
	  nbOfCoveredLeaves += (*childIt)->countLeavesWithRelevanceAbove(ancestorRelevance);
	}
    }
  return nbOfCoveredLeaves;
}
*/
/*
 * setG (although G is never recalculated as relevance)
 * So this method should be deleted (?)
 *
const unsigned int Node::setRelevance(const int distanceToParent)
{
  relevance = distanceToParent - intrinsicDistance;
  const unsigned int nbOfLeaves = countLeavesWithRelevanceAbove(relevance);
  if (nbOfLeaves == 0)
    {
      return 1;
    }
  return nbOfLeaves;
}
*/

void Node::deleteOffspringWithSmallerG(const int ancestorG, vector<list<Node*>::iterator>& ancestorChildren)
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
  for (vector<list<Node*>::iterator>::reverse_iterator childItIt = children.rbegin(); childItIt != children.rend(); )
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
  // Store the generating children (to not make their union with *this)
  unordered_set<list<Node*>::iterator, list_iterator_hash> coveredChildren;
  for (const list<Node*>::iterator childIt : children)
    {
      coveredChildren.insert(childIt);
    }
  children.clear();
  // This candidate goes to dendrogramFrontier
  dendrogramFrontier.push_back(this);
  // Construct the new candidates (or add children to it if already constructed)
  const list<Node*>::iterator thisIt = --dendrogramFrontier.end();
  for (list<Node*>::iterator otherChildIt = dendrogramFrontier.begin(); otherChildIt != thisIt; ++otherChildIt)
    {
      if (coveredChildren.find(otherChildIt) == coveredChildren.end())
	{
	  constructCandidate(otherChildIt, thisIt);
	}
     }
  // Remove this n-set from candidateNSets
  candidateNSets.erase(pattern);
  if (!children.empty())
    {
      // subsets of *this were found during candidate construction: they are at the odd positions of children (this at the even positions) and *this became a parent of *this
      parents.erase(this);
      for (vector<list<Node*>::iterator>::iterator childItIt = children.begin(); childItIt != children.end(); childItIt = childItIt + 2)
	{
	  coveredChildren.insert(*childItIt);
	}
      children.clear();
    }
  // Unlink covered children from their parents
  for (const list<Node*>::iterator coveredChildIt : coveredChildren)
    {
      (*coveredChildIt)->parents.erase(this);
      for (Node* parent : (*coveredChildIt)->parents)
	{
	  parent->unlinkGeneratingPairsInvolving(*coveredChildIt);
	  if (parent->children.empty())
	    {
	      candidateNSets.erase(parent->pattern);
	      candidates.erase(parent);
	      delete parent;
	    }
	}
    }

 /*
  * This must be deleted.
  * There's no distance to calculate anymore.
  * But there is G measure to be considered on dedrogram frontier
  *
  // Compute the distance to the covered child with the highest intrinsic distance
  unsigned int maximalChildIntrinsicDistance = 0;
  for (const list<Node*>::iterator coveredChildIt : coveredChildren)
    {
      if ((*coveredChildIt)->intrinsicDistance > maximalChildIntrinsicDistance)
	{
	  maximalChildIntrinsicDistance = (*coveredChildIt)->intrinsicDistance;
	}
    }
  int distanceToParent = intrinsicDistance - maximalChildIntrinsicDistance;
  // Set the relevance of the covered children
  unsigned int nbOfCoveredLeaves = 1; // + 1 because, in deleteIrrelevantOffspring, new children are inserted before removing the one that became irrelevant
  for (const list<Node*>::iterator coveredChildIt : coveredChildren)
    {
      nbOfCoveredLeaves += (*coveredChildIt)->setRelevance(distanceToParent);
    }
  // Compute the children (removing the nodes that became irrelevant)
  children.reserve(nbOfCoveredLeaves);
  for (list<Node*>::iterator coveredChildIt : coveredChildren)
    {
      const vector<list<Node*>::iterator> newChildren = (*coveredChildIt)->getParentChildren();
      if (newChildren.empty())
	{
	  dendrogram.push_back(*coveredChildIt);
	  children.push_back(--dendrogram.end());
	}
      else
	{
	  delete *coveredChildIt;
	  children.insert(children.end(), newChildren.begin(), newChildren.end());
	}
      dendrogramFrontier.erase(coveredChildIt);
    }
    */
}

const bool Node::lessPromising(const Node* node1, const Node* node2)
{
  return node1->g < node2->g;
}

const bool Node::morePromising(const Node* node1, const Node* node2)
{
  if (node1->g > node2->g)
    {
      return true;
    }
  if (node1->g == node2->g)
    {
      return node1->gEstimation > node2->gEstimation || (node1->gEstimation == node2->gEstimation && node1 < node2);
    }
  return false;
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
  // Candidate construction
  for (multiset<Node*>::iterator child1It = leaves.begin(); child1It != leaves.end(); child1It = leaves.erase(child1It))
    {
      dendrogramFrontier.push_front(*child1It);
      for (list<Node*>::iterator child2It = dendrogramFrontier.begin(); ++child2It != dendrogramFrontier.end(); )
	{
	  constructCandidate(dendrogramFrontier.begin(), child2It);
	}
    }

#ifdef DEBUG_HA
  printCadidates();
#endif


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
    	  candidate->g = candidate->membershipSum * candidate->membershipSum / candidate->area;
    	  if (isBetter)
    	    {
    	      highestG = candidate->g;
    	    }
    	  candidates.insert(candidate);
    	}
      Node* candidateToInsert = *candidates.begin();
      candidates.erase(candidates.begin());

// insertInDendrogramFrontier seems to NOT be cleaning the children
      candidateToInsert->insertInDendrogramFrontier();

      cerr << "\n\tend of while";
      printCadidates();
      cerr << "DendrogramFrontier: ";
      printNodeList(dendrogramFrontier);
    }
  // TODO: implement the second part of the selection of the relevant agglomerates
  // Node* root = dendrogramFrontier.front();
  // root->setRelevance(0);
  // if (root->getParentChildren().empty())
  //   {
  //     dendrogram.push_back(root);
  //   }
  // else
  //   {
  //     delete root;
  //   }
  // Order the nodes, more relevant first
  dendrogram.sort(moreRelevant);

#ifdef DEBUG_HA
  cout << "Dendrogram:\n";
  printNodeList(dendrogram);
#endif
  return pair<list<Node*>::const_iterator, list<Node*>::const_iterator>(dendrogram.begin(), dendrogram.end());
}

vector<unsigned int> Node::idVectorUnion(const vector<unsigned int>& v1,const vector<unsigned int>& v2)
{
  vector<unsigned int> unionVector;
  unionVector.reserve(v1.size() + v2.size());
  vector<unsigned int>::const_iterator v1It = v1.begin();
  vector<unsigned int>::const_iterator v2It = v2.begin();
  while (true)
    {
      if (*v1It < *v2It)
	{
	  unionVector.push_back(*v1It);
	  if (++v1It == v1.end())
	    {
	      for (; v2It != v2.end(); ++v2It)
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
	      if (++v1It == v1.end())
		{
		  for (; v2It != v2.end(); ++v2It)
		    {
		      unionVector.push_back(*v2It);
		    }
		  return unionVector;
		}
	      unionVector.push_back(*v2It);
	      if (++v2It == v2.end())
		{
		  for (; v1It != v1.end(); ++v1It)
		    {
		      unionVector.push_back(*v1It);
		    }
		  return unionVector;
		}
	    }
	  else
	    {
	      unionVector.push_back(*v2It);
	      if (++v2It == v2.end())
		{
		  for (; v1It != v1.end(); ++v1It)
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
  out << endl << "  area: " << area << endl << "  membershipSum: " << membershipSum / Attribute::noisePerUnit << endl << "  g: " << g / Attribute::noisePerUnit / Attribute::noisePerUnit << endl;
}

void Node::printCadidates()
{
  cout << "Candidates:\n";
  for (const Node* candidate : candidates)
    {
      candidate->print(cout);
    }
}

void Node::printNodeList(const list<Node*>& nodeList)
{
  for(const Node* node : nodeList)
    {
      node->print(cout);
    }
}
#endif
