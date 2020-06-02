/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
Struct: CET

Description: the CET
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "misc.h"
#include "FP.h"
#include "CET.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
bool isSubset ()

Decription: a helper function to check if list2 is a sublist of list1 
assuming data are sorted and list2 is non-empty
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
bool isSubset (const vector<unsigned short>& list1, const vector<unsigned short>& list2 )
{
	if ( list2.size() > list1.size() ) return false;
	else if ( list2.size() == list1.size() ) return ( list2 == list1 );
	unsigned int idx1 = 0, idx2 = 0;
	while ( idx1 < list1.size() && idx2 < list2.size() ) {
		if ( list1[idx1] == list2[idx2] ) {
			idx1++;
			idx2++;
		}
		else if ( list1[idx1] < list2[idx2] ) {
			idx1++;
			while ( idx1 < list1.size() && list1[idx1] < list2[idx2] ) idx1++;
		}
		else
			return false;
	}

	if ( idx2 < list2.size() ) return false;
	else return true;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
bool isPrefix ()

Decription: a helper function to check if list1 is a prefix of list2 
assuming data are sorted and list2 is non-empty
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
bool isPrefix (const vector<unsigned short>& list1, const vector<unsigned short>& list2 )
{
	if ( list1.size() > list2.size() ) return false;
	else if ( list1.size() == list2.size() ) return ( list1 == list2 );
	int idx1 = 0, idx2 = 0;
	while ( idx1 < list1.size() && idx2 < list2.size() ) {
		if ( list1[idx1] == list2[idx2] ) {
			idx1++;
			idx2++;
		}
		else
			return false;
	}

	return true;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
void  initialize ()

Decription: initialize the CET from the FP-tree
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void CET::initialize(FP & FPTree)
{
	vector<bool> isFrequent(FPTree.headTable.size(), false);
	unsigned short begin = 0;
	unsigned short end = 0;

	//check if each item is frequent
	for ( unsigned short i = 0; i < FPTree.headTable.size(); i++ ) {
		CETRoot.myChildren[i].mySupport = FPTree.headCount[i]; //initialize
		CETRoot.myChildren[i].myTidSum = FPTree.headTidSum[i];
		if ( CETRoot.myChildren[i].mySupport < SUPPORT ) {
			CETRoot.myChildren[i].isInfrequent = true;
		}
		else {
			isFrequent[i] = true;
			end = i; //here end records the largest frequent item
		}
	}

	for ( unsigned short i = 0; i < isFrequent.size() && i <= end; i++ ) {
		if ( isFrequent[i] ) {
			map<FPNodePtr, pair<int,int> > occurrence;
			IdxNodePtr posIdx = FPTree.headTable[i].right;
			while ( posIdx != 0 ) {
				occurrence.insert(make_pair(posIdx->locInFP, 
					make_pair(posIdx->locInFP->count,posIdx->locInFP->tid_sum)));
				posIdx = posIdx->right;
			}

			bool stillClosed = true;

			currentPrefix.push_back(i);

			//second, check if it is promising
			pair<HashClosed::const_iterator, HashClosed::const_iterator> p =	closedItemsets.equal_range(CETRoot.myChildren[i].myTidSum);

			for ( HashClosed::const_iterator pos = p.first; pos != p.second; ++pos ) {
				if ( pos->second.first == CETRoot.myChildren[i].mySupport 
					&& isSubset(pos->second.second, currentPrefix ) ) {
						CETRoot.myChildren[i].isUnpromising = true;
						stillClosed = false;
						break;
					}
			}

			if ( !stillClosed ) {
				currentPrefix.pop_back();
				continue; //next i, please
			}

			CETRoot.myChildren[i].childrenSupport = initializeHelp(CETRoot.myChildren[i],	occurrence, i, end, isFrequent);

			//check if it is closed
			if ( CETRoot.myChildren[i].childrenSupport < CETRoot.myChildren[i].mySupport ) {
				// CETRoot.myChildren[i].isClosed = true;
				closedItemsets.insert(make_pair(CETRoot.myChildren[i].myTidSum, make_pair(CETRoot.myChildren[i].mySupport,currentPrefix)));
			}

			currentPrefix.pop_back();
		}
	}
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
int initializeHelp ()

Decription: a helper function for initialize() 
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int CET::initializeHelp(TreeNode & node,
						map<FPNodePtr,pair<int,int> > & occurrence,
						unsigned short begin,
						unsigned short end,
						vector<bool> & isFrequent)
{
	if ( begin >= end ) return 0; //end must > begin

	for ( unsigned short i = begin + 1; i <= end; i++ ) {
		if ( isFrequent[i] ) node.myChildren[i].isInfrequent = true; //initialize
	}

	unsigned short newend = begin;
	int bestSupport = 0;

	//step 1, compute the support and tid_sum for all children
	for ( map<FPNodePtr,pair<int,int> >::iterator pos = occurrence.begin(); 
		pos != occurrence.end(); ++pos) 
	{
		FPNodePtr  posFP = pos->first; //find the location in FP
		int tempCount = pos->second.first; //support
		int tempTidSum = pos->second.second; //tid sum
		posFP = posFP->parent;
		while ( posFP->parent != 0 && posFP->item <= end ) { //while not the root
			if ( isFrequent[posFP->item] ) {
				node.myChildren[posFP->item].mySupport += tempCount;
				node.myChildren[posFP->item].myTidSum += tempTidSum;
			}
			posFP = posFP->parent;
		}
	}

	//step 2, find the newly infrequent children
	for ( unsigned short i = begin + 1; i <= end; i++ ) {
		if ( isFrequent[i] ) { 
			if ( node.myChildren[i].mySupport > bestSupport )
				bestSupport = node.myChildren[i].mySupport;
			if ( node.myChildren[i].mySupport < SUPPORT ) {
				isFrequent[i] = false; //temporarily make it false
			}
			else {
				node.myChildren[i].isInfrequent = false;
				newend = i;
			}
		}
	}

	end = newend;

	if ( begin == end ) { //if no frequent children, stop here and return
		//recover the original indicator vector for frequent items of the above level
		for ( Family::iterator pos1 = node.myChildren.begin();
			pos1 != node.myChildren.end(); ++pos1 )
		{
			if ( pos1->second.isInfrequent == true ) {
				isFrequent[pos1->first] = true;
			}
		}
		return bestSupport;
	}

	//step 3, for the children remained frequent, create their occurrence list
	vector<map<FPNodePtr,pair<int,int> > > newOccurrence(end-begin);

	for ( map<FPNodePtr,pair<int,int> >::iterator pos = occurrence.begin(); pos != occurrence.end(); ++pos) {
		FPNodePtr  posFP = pos->first; //find the location in FP
		int tempCount = pos->second.first; //support
		int tempTidSum = pos->second.second; //tid sum
		posFP = posFP->parent;
		while ( posFP->parent != 0 && posFP->item <= end ) { //while not the root
			if ( isFrequent[posFP->item] ) {
				map<FPNodePtr,pair<int,int> >::iterator pos2 = 
					newOccurrence[posFP->item - begin - 1].find(posFP);
				if ( pos2 != newOccurrence[posFP->item - begin - 1].end() ) {
					pos2->second.first += tempCount;
					pos2->second.second += tempTidSum;
				}
				else {
					newOccurrence[posFP->item - begin - 1].insert(
						make_pair(posFP, make_pair(tempCount, tempTidSum)));
				}
			}
			posFP = posFP->parent;
		}
	}

	//step 4, for the children remained frequent, recursively explore
	for ( unsigned short i = begin + 1; i <= end; i++ ) {
		if ( isFrequent[i] ) { 

			//first, mark the node to be frequent
			node.myChildren[i].isInfrequent = false; //redundant

			bool stillClosed = true;

			currentPrefix.push_back(i);

			//second, check if it is promising
			pair<HashClosed::const_iterator, HashClosed::const_iterator> p =
				closedItemsets.equal_range(node.myChildren[i].myTidSum);

			for ( HashClosed::const_iterator pos = p.first; pos != p.second; ++pos ) {
				if ( pos->second.first == node.myChildren[i].mySupport 
					&& isSubset(pos->second.second, currentPrefix ) ) {
						node.myChildren[i].isUnpromising = true;
						stillClosed = false;
						break;
					}
			}

			if ( !stillClosed ) {
				currentPrefix.pop_back();
				continue; //next i, please
			}

			node.myChildren[i].childrenSupport = 
				initializeHelp(node.myChildren[i],
				newOccurrence[i-begin-1], i, end, isFrequent);

			//check if it is closed
			if ( node.myChildren[i].childrenSupport < node.myChildren[i].mySupport ) {
				// node.myChildren[i].isClosed = true;
				closedItemsets.insert(make_pair(node.myChildren[i].myTidSum, 
					make_pair(node.myChildren[i].mySupport,currentPrefix)));
			}

			currentPrefix.pop_back();
		}
	}

	//step 5, recover the original indicator vector for frequent items of the above level
	for ( Family::iterator pos1 = node.myChildren.begin();
		pos1 != node.myChildren.end(); ++pos1 )
	{
		if ( pos1->second.isInfrequent == true ) {
			isFrequent[pos1->first] = true;
		}
	}

	return bestSupport;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
void getOccurrence ()

Decription: grab the occurrence list to the current node
Note: assumed that previousPrefix < currentPrefix.size()
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void CET::getOccurrence(const size_t previousPrefix, 
						const map<FPNodePtr,pair<int,int> > & previousOccurrence, 
						const FP & FPTree,
						map<FPNodePtr,pair<int,int> > & myOccurrence)
{
	//case 1, called by CETRoot, when we actually don't use the returned value
	if ( previousPrefix == 0 && currentPrefix.size() == 0 ) {
		return;
	}
	//case 2, called by the children of CETRoot. We use FPTree directly
	else if ( previousPrefix == 0 && currentPrefix.size() == 1 ) { //the root node
		IdxNodePtr posIdx = FPTree.headTable[currentPrefix[0]].right;
		while ( posIdx != 0 ) {
			myOccurrence.insert(make_pair(posIdx->locInFP, 
				make_pair(posIdx->locInFP->count,posIdx->locInFP->tid_sum)));
			posIdx = posIdx->right;
		}
	}
	//case 3, called by descendants of CETRoot, but never called before
	else if ( previousPrefix == 0 && currentPrefix.size() > 1 ) {
		map<FPNodePtr,pair<int,int> > tempOccurrence;
		IdxNodePtr posIdx = FPTree.headTable[currentPrefix[0]].right;
		while ( posIdx != 0 ) {
			tempOccurrence.insert(make_pair(posIdx->locInFP, 
				make_pair(posIdx->locInFP->count,posIdx->locInFP->tid_sum)));
			posIdx = posIdx->right;
		}
		getOccurrence(1, tempOccurrence, FPTree, myOccurrence);
	}
	//case 4, they are the same. This should not happen at all.
	else if ( previousPrefix == currentPrefix.size() ) {
		myOccurrence = previousOccurrence; 
	}
	//case 5, the general case, i.e., previousPrefix >= 1, so previousOccurrence non-empty 
	else {
		for ( map<FPNodePtr,pair<int,int> >::const_iterator pos = previousOccurrence.begin(); 
			pos != previousOccurrence.end(); ++pos) 
		{
			FPNodePtr  posFP = pos->first; //find the location in FP
			int tempCount = pos->second.first; //support
			int tempTidSum = pos->second.second; //tid sum
			bool isFound = false;
			size_t start = previousPrefix;
			posFP = posFP->parent;
			while ( posFP->parent != 0 && !isFound ) {
				if ( posFP->item > currentPrefix[start] ) { //failed
					break;
				}
				else if ( posFP->item == currentPrefix[start] ) {
					start++;
					if ( start == currentPrefix.size() ) {
						isFound = true;
						break;
					}
				}
				posFP = posFP->parent;
			} //end of while

			if ( isFound ) {
				map<FPNodePtr,pair<int,int> >::iterator pos2 = 
					myOccurrence.find(posFP);
				if ( pos2 != myOccurrence.end() ) {
					pos2->second.first += tempCount;
					pos2->second.second += tempTidSum;
				}
				else {
					myOccurrence.insert(make_pair(posFP, make_pair(tempCount, tempTidSum)));
				}
			} //end of if

		} //end of for
	}
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
void addition ()

Decription: add an itemset to the CET 
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void CET::addition(const int tid, const vector<unsigned short> & itemset, const FP & FPTree)
{
	map<FPNodePtr,pair<int,int> > previousOccurrence;
	vector<bool> parentIsNew(itemset.size(), false);

	////debug
	//cout << "To add itemset: ";
	//for ( int i = 0; i < itemset.size(); i++ )
	//	cout << itemset[i] << " ";
	//cout << endl;

	addHelp(tid, 0, previousOccurrence, itemset, parentIsNew,	0, FPTree, CETRoot);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
int addHelp ()

Decription: a helper function for addition() 
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int CET::addHelp(const int tid,
				 const size_t previousPrefix,
				 const map<FPNodePtr,pair<int,int> > & previousOccurrence,
				 const vector<unsigned short> & parentItemset,
				 const vector<bool> & parentIsNew,
				 const unsigned short startPosition,
				 const FP & FPTree,
				 TreeNode & node)
{
	return 0;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
void  cleanNode ()

Decription: a helper function for recursively removing closed itemsets
in a subtree from the Hashtable
Note: assuming currentPrefix contains the item at "node"
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void CET::cleanNode(TreeNode & node)
{

}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
void deletion ()

Decription: delete an itemset from the CET 
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void CET::deletion(const int tid, const vector<unsigned short> & itemset, const FP & FPTree)
{
	vector<bool> parentIsNew(itemset.size(), false);
	deleteHelp(tid, itemset, parentIsNew, 0, FPTree, CETRoot);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
void deleteHelp ()

Decription: a helper function for deletion() 
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void CET::deleteHelp(const int tid,
					const vector<unsigned short> & parentItemset,
					const vector<bool> & parentIsNew,
					const unsigned short startPosition,
					const FP & FPTree,
					TreeNode & node)
{

}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
void  printMe ()

Decription: debugging function 
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void CET::printMe(TreeNode & node, unsigned short level)
{
  for (unsigned short s = 0; s < currentPrefix.size(); s++)
    cout << currentPrefix[s];
  cout << ": ";

  if (node.myChildren.size() != 0) {
    Family::iterator pos;
    for (pos = node.myChildren.begin(); pos != node.myChildren.end(); ++pos) {
		cout << "\"" << pos->first << ' ' << pos->second.mySupport << ' ' << pos->second.myTidSum << ' '
			<< (pos->second.isInfrequent ? 't' : 'f') << ' '
			<< (pos->second.isUnpromising ? 't' : 'f') << ' ';
    }

    cout << endl;

    for (Family::iterator pos1 = node.myChildren.begin(); pos1 != node.myChildren.end(); ++pos1) {
      currentPrefix.push_back(pos1->first);
      printMe(pos1->second, level + 1);
      currentPrefix.pop_back();
    }
  }
  else
    cout << endl;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
void  printHash ()

Decription: debugging function 
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void CET::printHash()
{
  for (HashClosed::iterator pos = closedItemsets.begin(); pos != closedItemsets.end(); ++pos) {
    cout << pos->first << " ";
    cout << pos->second.first << ":";
    for (int i = 0; i < pos->second.second.size(); i++) {
      cout << " " << pos->second.second[i];
    }
    cout << endl;
  }
}

///* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
//void  checkMe ()
//
//Decription: check if the closed itemsets are correct
//* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
//void CET::checkMe()
//{
//	HashClosed::iterator pos1;
//	for ( pos1 = closedItemsets.begin(); pos1 != closedItemsets.end(); ++pos1 ) {
//		pair<HashClosed::iterator, HashClosed::iterator> p = 
//			closedItemsets.equal_range(pos1->first);
//
//		for ( HashClosed::const_iterator pos = p.first; pos != p.second; ++pos ) {
//			if ( pos == pos1 ) continue;
//
//			else if ( pos->second.first == pos1->second.first 
//				&& isSubset(pos->second.second, pos1->second.second) )
//			{
//				cout << "error: " << endl;
//				cout << pos1->first << " " << pos1->second.first << " ";
//				for ( int i = 0; i < pos1->second.second.size(); i++ )
//					cout << pos1->second.second[i] << "-";
//				cout << endl;
//				cout << pos->first << " " << pos->second.first << " ";
//				for ( int i = 0; i < pos->second.second.size(); i++ )
//					cout << pos->second.second[i] << "-";
//				cout << endl;
//			}
//		}
//	}
//}
