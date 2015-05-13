// Copyright 2013,2014 Loïc Cerf (lcerf@dcc.ufmg.br)

// This file is part of multidupehack.

// multidupehack is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License version 3 as published by the Free Software Foundation

// multidupehack is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along with multidupehack; if not, write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

#include "Node.h"

unsigned int Node::maximalNbOfClosedNSets;
bool Node::isBelowMaximalNbOfClosedNSets = true;
unsigned int Node::largestIntrinsicDistance = numeric_limits<unsigned int>::max();
int Node::smallestArea = numeric_limits<unsigned int>::max();

multiset<Node*, const bool(*)(const Node*, const Node*)> Node::leaves(lessPromising);
list<Node*> Node::dendrogram;
list<Node*> Node::dendrogramFrontier;
set<Node*, const bool(*)(const Node*, const Node*)> Node::candidates(morePromising);
unordered_map<vector<vector<unsigned int>>, Node*, vector_hash<vector<unsigned int>>> Node::candidateNSets;

///////////////////////////////////
// Node constructors
//	Remember:
//		- pattern -> is a vector of vectors of elements (id and noise), which means [dimension][element]
//		- intrincicDistance -> maior ruido relativo de um dos elementos do padrao (pior contagem de ruido)
//		- nextTuple -> guarda a ultima computacao do intrincicDistance
//		- parents -> All candidates for to build from this pattern
//		- children -> patterns that built this pattern
//		- leaves -> the very first patterns
//		- dendrogramFrontier -> Agglomerates on top of dendrogram. The head.
//		-  
///////////////////////////////////
// start with attributes (?)
Node::Node(const vector<Attribute*>& attributes): pattern(), intrinsicDistance(0), relevance(1), nextTuple(), parents(), children()
{
  pattern.reserve(attributes.size());
  nextTuple.reserve(attributes.size());
#ifdef DEBUG_HA
      cout << "Creating NODE ";
#endif
  for (vector<Attribute*>::const_iterator attributeIt = attributes.begin(); attributeIt != attributes.end(); ++attributeIt)
    {
      pattern.push_back((*attributeIt)->getElements());
      relevance *= pattern.back().size();
      nextTuple.push_back(pattern.back().begin());

#ifdef DEBUG_HA
      for(Element it : ((*attributeIt)->getElements()))
	{
	  unsigned int v1 = (*attributeIt)->getId();
	  unsigned int v2 = it.getId();
	  cout << Attribute::printLabelsById(v1, v2) << " ";
	}
      cout << " - ";
#endif
    }
#ifdef DEBUG_HA
      cout << "relevance " << relevance;
      cout << "\n\n";
#endif
}

// start with a vector of vectors of integers (nSet), and data
Node::Node(const vector<vector<unsigned int>>& nSet, const Trie* data): pattern(), intrinsicDistance(0), relevance(1), nextTuple(), parents(), children()
{
  pattern.reserve(nSet.size());	
  nextTuple.reserve(nSet.size());
  for (const vector<unsigned int>& dimension : nSet)
    {
      relevance *= dimension.size();
      vector<Element> patternDimension;
      patternDimension.reserve(dimension.size());
      for(unsigned int id : dimension)
	{
	  patternDimension.push_back(Element(id));
	}
      pattern.push_back(patternDimension);
      nextTuple.push_back(pattern.back().begin());	
    }
  data->countNoise(pattern);   
}

// start with a vector of vectors of integers (nSet), 2 children
// (protected constructor)
Node::Node(const vector<vector<unsigned int>>& nSet, const list<Node*>::iterator child1It, const list<Node*>::iterator child2It): pattern(), intrinsicDistance(0), relevance(1), nextTuple(), parents(), children {child1It, child2It}
{
  pattern.reserve(nSet.size());
  nextTuple.reserve(nSet.size());
  for (const vector<unsigned int>& nSetDimension : nSet)
    {
      relevance *= nSetDimension.size();
      vector<Element> patternDimension;
      patternDimension.reserve(nSetDimension.size());
      for (const unsigned int id : nSetDimension)
	{
	  patternDimension.push_back(Element(id));
	}
      pattern.push_back(patternDimension);
      nextTuple.push_back(pattern.back().begin());
    }
}
///////////////////////////////
// end of Node constructors
//////////////////////////////

// Return a pattern vector
const vector<Element>& Node::dimension(const unsigned int dimensionId) const
{
  return pattern[dimensionId];
}

// Return the "squared" area of this pattern
const unsigned int Node::area() const
{
  unsigned int area = 1;
  for (const vector<Element>& patternDimension : pattern)
    {
      area *= patternDimension.size();
    }
  return area;
}

// Remove this nSet (pattern) from candidateNSets
void Node::eraseNSet() const
{
  vector<vector<unsigned int>> nSet;
  nSet.reserve(pattern.size());
  for (const vector<Element>& patternDimension : pattern)
    {
      vector<unsigned int> nSetDimension;
      nSetDimension.reserve(patternDimension.size());
      for (const Element& element : patternDimension)
	{
	  nSetDimension.push_back(element.getId());
	}
      nSet.push_back(nSetDimension);		
    }
  candidateNSets.erase(nSet);
}

// A candidate is the combination of every pattern in dedrogramFrontier.
// To build it, give 2 patterns (list of Node) to this constructCandidate.
// The union of this 2 patterns will be a candidate.
void Node::constructCandidate(const list<Node*>::iterator child1It, const list<Node*>::iterator child2It)
{
  vector<vector<unsigned int>> unionNSet;
  unionNSet.reserve((*child1It)->pattern.size());
  vector<vector<Element>>::const_iterator otherPatternIt = (*child2It)->pattern.begin();

  // This loop is adding together the Element vectors of child1 and child2 to the unionNSet
  for (const vector<Element>& patternDimension : (*child1It)->pattern)
    {
      unionNSet.push_back(Element::idVectorUnion(*otherPatternIt, patternDimension));
      ++otherPatternIt;
    }

  // Seek for unionSet in candidates to avoid redundancy
  Node* candidate;
  const unordered_map<vector<vector<unsigned int>>, Node*, vector_hash<vector<unsigned int>>>::iterator candidateNSetIt = candidateNSets.find(unionNSet);	// Search for the nSet child1+child2 (unionNSet)

  // if doesn't find it
  if (candidateNSetIt == candidateNSets.end())
    {
      // create a new candidate
      candidate = new Node(unionNSet, child1It, child2It);
      candidateNSets[unionNSet] = candidate;
      candidates.insert(candidates.begin(), candidate);
    }
  else
    {
      // candidate already exist, just add new children to it
      candidate = candidateNSetIt->second;
      candidate->children.push_back(child1It);
      candidate->children.push_back(child2It);
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

// computeIntrinsicDistance of (this) pattern.
//
void Node::computeIntrinsicDistance()
{
  // Find the highest noise
  for (const vector<Element>& patternDimension : pattern)
    {
      const unsigned int maxNoiseInDimension = max_element(patternDimension.begin(), patternDimension.end())->getNoise() / (relevance / patternDimension.size()); // relevance is, here, the area
      if (maxNoiseInDimension > intrinsicDistance)
				{
					intrinsicDistance = maxNoiseInDimension;
				}
    }
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

// TODO: returning negatives!!!
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

void Node::deleteIrrelevantOffspring(const int ancestorRelevance, vector<list<Node*>::iterator>& ancestorChildren)
{
  for (list<Node*>::iterator childIt : children)
    {
      if ((*childIt)->relevance > ancestorRelevance)
				{
					ancestorChildren.push_back(childIt);
				}
      else
			{
				(*childIt)->deleteIrrelevantOffspring(ancestorRelevance, ancestorChildren);
				delete *childIt;
				dendrogram.erase(childIt);
			}
    }
}

vector<list<Node*>::iterator> Node::getParentChildren()
{
  cout << "Relevance: " << relevance << "\n";
  bool isIrrelevant = true;
  for (vector<list<Node*>::iterator>::reverse_iterator childItIt = children.rbegin(); childItIt != children.rend(); )
    {
#ifdef DEBUG_HA
      cout << "Children:\n";
      printANode(**childItIt);
#endif
      if ((**childItIt)->relevance > relevance)
	{
	  cout << "This will always be true!\n";
	  ++childItIt;
	}
// TODO: This else never happen!!!!
      else
	{
	  isIrrelevant = false;
	  (**childItIt)->deleteIrrelevantOffspring(relevance, children);
	  delete **childItIt;
	  dendrogram.erase(*childItIt);
	  *childItIt++ = children.back();
	  children.pop_back();
	}
    }
  if (isIrrelevant)
    {
      return move(children);
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
  eraseNSet();
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
	      parent->eraseNSet();
	      candidates.erase(parent);
	      delete parent;
	    }
	}
    }
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
}

const bool Node::lessPromising(const Node* node1, const Node* node2)
{
  return node1->intrinsicDistance > node2->intrinsicDistance || (node1->intrinsicDistance == node2->intrinsicDistance && node1->relevance < node2->relevance);
}

const bool Node::morePromising(const Node* node1, const Node* node2)
{
  if (node1->intrinsicDistance < node2->intrinsicDistance)
    {
      return true;
    }
  if (node1->intrinsicDistance == node2->intrinsicDistance)
    {
      return node1->relevance > node2->relevance || (node1->relevance == node2->relevance && node1 < node2); // relevance is, here, the area
    }
  return false;
}

const bool Node::moreRelevant(const Node* node1, const Node* node2)
{
  return node2->relevance < node1->relevance;
}

void Node::setMaximalNbOfClosedNSetsForAgglomeration(const unsigned int maximalNbOfClosedNSetsForAgglomeration)
{
  maximalNbOfClosedNSets = maximalNbOfClosedNSetsForAgglomeration;
}

// Main function
void Node::insertOrDelete(Node* leaf)
{
#ifdef DEBUG_HA
  cout << "\tInsert or Delet to leaves?\n";
#endif
  leaf->computeIntrinsicDistance();
#ifdef DEBUG_HA
  cout << "\t\t-Intrinsic Distance: " << leaf->intrinsicDistance << "\n";
#endif

  // Delete the less promising leaves, in case --HA set is smaller than the number of leaves
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
      largestIntrinsicDistance = (*leaves.begin())->intrinsicDistance;
      smallestArea = (*leaves.begin())->relevance;
      for (multiset<Node*>::iterator leafIt = leaves.begin(); leafIt != leaves.end() && (*leafIt)->intrinsicDistance == largestIntrinsicDistance && (*leafIt)->relevance == smallestArea; leafIt = leaves.erase(leafIt))
	{
	  delete *leafIt;
	}
#ifdef DEBUG_HA
      // it doesn't mean this one will be deleted. Someone is.
      cout << "\tDelete!\n\n";
#endif
    }
  if (leaf->intrinsicDistance < largestIntrinsicDistance || (leaf->intrinsicDistance == largestIntrinsicDistance && leaf->relevance > smallestArea))
    {
#ifdef DEBUG_HA
      cout << "\tInsert!\n\n";
#endif
      leaves.insert(leaf);
      return;
    }
  delete leaf;
}

pair<list<Node*>::const_iterator, list<Node*>::const_iterator> Node::agglomerateAndSelect(const Trie* data)
{
#ifdef DEBUG_HA
  cout << "-=-Agglomerate and Select-=-\n";
  cout << "leaves are:\n";
  printLeaves();
  cout << "\n\n";
#endif

  if (leaves.empty())
    {
      return pair<list<Node*>::const_iterator, list<Node*>::const_iterator>(dendrogram.begin(), dendrogram.begin());
    }

  // Candidate construction - Create every possible united pair on Dendrogram Frontier
  // 1st, Iterate through leaves
  for (multiset<Node*>::iterator child1It = leaves.begin(); child1It != leaves.end(); child1It = leaves.erase(child1It))
    {
      // Add the 1st leaf to the begining of Dendrogram Frontier

      dendrogramFrontier.push_front(*child1It);

      // Iterate through Dendrogram Frontier
      // Note that equal pairs will never be passed together (loop final condition has ++)
      for (list<Node*>::iterator child2It = dendrogramFrontier.begin(); ++child2It != dendrogramFrontier.end(); )
	{
	  // Construct every possible pair combination on the front of Dendrogram
	  constructCandidate(dendrogramFrontier.begin(), child2It);
	}
    }

#ifdef DEBUG_HA
	  cout << "+ Candidate construction +\n";
	  printCandidates();
	  cout << "\nDedrogram Frontier:\n";
	  printDendrogramFrontier();
	  cout << "\n\n";
#endif

  // Hierarchical agglomeration
  while (!candidates.empty())
    {
#ifdef DEBUG_HA
      cout << "+ Hierarchical Agglomeration +\n";
#endif
      // Searching for the candidates with the smallest intrinsic distance and the largest area (in case of equality according to both criteria, the one with the smallest memory address is retained)
      unsigned int smallestIntrinsicDistance = numeric_limits<unsigned int>::max();

      // Finding the worse relative noise at this hiperplan
      while ((*candidates.begin())->nextTuple.front() != (*candidates.begin())->pattern.front().end())
    	{
    	  Node* candidate = *candidates.begin();
    	  // candidate->intrinsicDistance is partial
    	  candidates.erase(candidates.begin());

    	  vector<unsigned int> noiseThresholds(candidate->pattern.size(), candidate->relevance); // candidate->relevance is, here, its area
    	  vector<unsigned int>::iterator noiseThresholdIt = noiseThresholds.begin();

    	  for (const vector<Element>& patternDimension : candidate->pattern)
    	    {
    	      *noiseThresholdIt /= patternDimension.size();
    	      *noiseThresholdIt++ *= smallestIntrinsicDistance;
    	    }

    	  const bool isLessNoisy = data->lessNoisyNSet(noiseThresholds, candidate->pattern, candidate->nextTuple);
    	  candidate->computeIntrinsicDistance();

#ifdef DEBUG_HA
    	  cout << "Current Candidate (updtd intrinsicDistance):\n";
    	  printANode(candidate);
#endif

    	  if (isLessNoisy)
    	    {
    	      smallestIntrinsicDistance = candidate->intrinsicDistance;
    	    }
    	  candidates.insert(candidate);
    	}

      Node* candidateToInsert = *candidates.begin();
      candidates.erase(candidates.begin());
      candidateToInsert->insertInDendrogramFrontier();

#ifdef DEBUG_HA
      cout << "Updated dendrogram frontier:\n";
      printDendrogramFrontier();
#endif
    }

  // Those which are not at the frontier will be part of the body
  Node* root = dendrogramFrontier.front();
// TODO: These prints must be deleted
  cout << "root: \n";
  printANode(root);

  root->setRelevance(0);

  cout << "root2: \n";
  printANode(root);

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

#ifdef DEBUG_HA
  cout << "Dendrogram: \n";
  printDedrogram();
#endif
  return pair<list<Node*>::const_iterator, list<Node*>::const_iterator>(dendrogram.begin(), dendrogram.end());
}

#ifdef DEBUG_HA
void Node::printCandidates()
{
  set<Node*, const bool(*)(const Node*, const Node*)>::iterator candIt;
  int counter = 0;
  for(candIt = candidates.begin(); candIt != candidates.end(); candIt++)
    {
      cout << counter << ": ";
      printANode(*candIt);
      counter++;
    }
}

void Node::printDendrogramFrontier()
{
  list<Node*>::iterator dendroIt;
  unsigned int counter = 0;
  for(dendroIt = dendrogramFrontier.begin(); dendroIt != dendrogramFrontier.end(); dendroIt++)
    {
      cout << counter << ": ";
      printANode(*dendroIt);
      counter++;
    }
}

void Node::printDedrogram()
{
  list<Node*>::iterator dendroIt;
  unsigned int counter = 0;
  for(dendroIt = dendrogram.begin(); dendroIt != dendrogram.end(); dendroIt++)
    {
      cout << counter << ": ";
      printANode(*dendroIt);
      counter++;
    }
}

void Node::printLeaves()
{
  multiset<Node*, const bool(*)(const Node*, const Node*)>::iterator leavesIt;
  unsigned int counter = 0;
  for(leavesIt = leaves.begin(); leavesIt != leaves.end(); leavesIt++)
    {
      cout << counter << ": ";
      printANode(*leavesIt);
      counter++;
    }
}

void Node::printANode(Node* thisNode)
{
  for(unsigned int v1 = 0; v1 < (thisNode)->pattern.size(); v1++)
  	{
  	  for(unsigned int v2 = 0; v2 < (thisNode)->pattern[v1].size(); v2++)
  	    {
  	      cout << Attribute::printLabelsById(v1, (thisNode)->pattern[v1][v2].getId()) << " ";
  	    }
  	  cout << " - ";
  	}
        cout << "\n\tIntrinsic Distance: " << (thisNode)->intrinsicDistance << "\n";
        cout << "\tRelevance (Area): " << (thisNode)->relevance;
        cout << "\n\n";
}
#endif
