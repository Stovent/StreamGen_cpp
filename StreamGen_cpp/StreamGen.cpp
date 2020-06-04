#include "StreamGen.h"
#include "Utility.h"
#include <iostream>
#include <queue>
#include <stack>

extern uint32_t CET_NODE_ID;
extern uint32_t NBR_NODES;
extern uint32_t NBR_CLOSED_NODES;
extern uint32_t minsup;
extern CETNode ROOT;
extern std::map<uint32_t, CETNode*> CLOSED_ITEMSETS;

void Explore(CETNode* const _node) {
	std::map<uint32_t, CETNode*>* siblings = _node->parent->children;

	for (const std::pair<uint32_t, CETNode*>& sibling : *siblings) {
		if (sibling.second->type == GENERATOR_NODE && sibling.second->maxitem > _node->maxitem) {
			new_child(_node, sibling.second->maxitem, inter(_node->tidlist, sibling.second->tidlist));
		}
		else if (sibling.second->type == GENERATOR_NODE && sibling.second->maxitem < _node->maxitem) {
			if (!has_child(sibling.second, _node->maxitem))
				new_child(sibling.second, _node->maxitem, inter(sibling.second->tidlist, _node->tidlist));
		}
	}
};

//void Addition(const uint32_t _tid, std::vector<uint32_t>* _transaction, const uint32_t _minsupp, CETNode* const _node, std::map<long, std::vector<std::vector<CETNode*>*>*>* const _EQ_TABLE) {
void Addition(const uint32_t tid, std::vector<uint32_t>* transaction) {
	std::queue<CETNode*> queue;
	queue.push(&ROOT);

	while (!queue.empty()) {
		CETNode* node = queue.front();
		queue.pop();

		std::vector<uint32_t>* intersec = inter(node->itemset, transaction);
		if (node->itemset->size() == 0) { // special treatment for the root node
			for (const std::pair<uint32_t, CETNode*>& item : *node->children) {
				queue.push(item.second);
			}
		}
		else if (intersec->size() == node->itemset->size() - 1) {
			if (node->type == UNPROMISSING_NODE) {
				identify(node);
				if (node->type == UNPROMISSING_NODE)
					continue;

				Explore(node);
			}
			if (node->children) {
				for (std::map<uint32_t, CETNode*>::const_reverse_iterator child = node->children->rbegin(); child != node->children->rend(); child++) {
					if (child->second->type != INFREQUENT_NODE)
						queue.push(child->second);
				}
			}
		}
		else if (intersec->size() > node->itemset->size() - 1) {
			node->support++;
			node->tidlist->push_back(tid);
			node->tidsum += tid;
			if (node->type == UNPROMISSING_NODE)
				continue;

			if (node->type == INFREQUENT_NODE) {
				if (node->support < minsup)
					continue;

				identify(node);
				if (node->type == GENERATOR_NODE) {
					Explore(node);
				}
			}
			else {
				if (node->children) {
					for (std::map<uint32_t, CETNode*>::const_reverse_iterator child = node->children->rbegin(); child != node->children->rend(); child++) {
						queue.push(child->second);
					}
				}

				for (uint32_t item : *transaction) {
					if (item <= node->maxitem)
						continue;

					// create childs of node using his lexicographically right siblings
					std::map<uint32_t, CETNode*>::iterator child = node->parent->children->find(item);
					if (child != node->parent->children->end() && child->second->type == GENERATOR_NODE) {
						new_child(node, item, node->tidlist); // TODO: check which tidlist to give to the new child
					}
				}
			}
		}

		delete intersec;
	}
};

void Deletion(const uint32_t _tid, std::vector<uint32_t>* _transaction, const uint32_t _minsupp, CETNode* const _node, std::map<long, std::vector<std::vector<CETNode*>*>*>* const _EQ_TABLE) {

};

void identify(CETNode* node) {
	if (node->support < minsup) {
		node->type = INFREQUENT_NODE;
	}
	else if (itemset_is_a_generator(node->itemset, node->support)) {
		node->type = GENERATOR_NODE;
	}
	else {
		node->type = UNPROMISSING_NODE;
	}
}

void new_child(CETNode* node, uint32_t maxitem, std::vector<uint32_t>* tidlist) {
	if (!node->children) {
		node->children = new std::map<uint32_t, CETNode*>();
	}
	node->children->emplace(maxitem, create_node(node, maxitem, tidlist));
}

bool has_child(CETNode* node, uint32_t maxitem) {
	if (!node->children)
		return false;
	
	return node->children->find(maxitem) != node->children->end();
}


//Below are utility functions for Moment (non in algorithm). TODO: Still need to add exit and error codes

CETNode* create_node(CETNode* parent, uint32_t maxitem, std::vector<uint32_t>* tidlist) {
	CETNode* node = new CETNode;
	node->id = ++CET_NODE_ID;
	node->maxitem = maxitem;
	node->parent = parent;
	node->tidlist = tidlist;
	node->support = node->tidlist->size();
	node->children = nullptr;

	node->tidsum = 0;
	for (const uint32_t tid : *node->tidlist) {
		node->tidsum += tid;
	}

	node->itemset = new std::vector<uint32_t>(parent->itemset->begin(), parent->itemset->end());
	node->itemset->push_back(maxitem);
	identify(node);
	return node;
}

/**
 * node: the parent of the node to check
**/
bool itemset_is_a_generator(const std::vector<uint32_t>* itemset, const uint32_t refsup) {
	std::stack<CETNode*> stack;
	stack.push(&ROOT);

	while (!stack.empty()) {
		CETNode* node = stack.top();
		stack.pop();
		if (!node->children)
			continue;
		
		for (const std::pair<uint32_t, CETNode*>& child : *node->children) {
			if (contains(child.second->itemset, itemset, true)) {
				if (child.second->type != GENERATOR_NODE)
					return false;
				if (child.second->support == refsup)
					return false;
				stack.push(child.second);
			}
		}
	}
	
	return true;
}

/*
void add_ci(CETNode* const _node, std::map<long, std::vector<std::vector<CETNode*>*>*>* const _EQ_TABLE) {
	//std::cout << "Added new CI of size " << _node->itemset->size() << " ";
	//print_cet_node(_node);

	if (NBR_CLOSED_NODES != CLOSED_ITEMSETS.size()) {
		//System.out.println("before add erreur d'integrite " + NBR_CLOSED_NODES + " vs " + CLOSED_ITEMSETS.size());
		exit(ERROR_NBR_CLOSED_NODES_DOES_NOT_MATCH_CI_SET_SIZE);
	}

	if (CLOSED_ITEMSETS.end() != CLOSED_ITEMSETS.find(_node->id)) {
		//System.out.println("on a deja cet id #" + _node.id + " " + Arrays.toString(CLOSED_ITEMSETS.get(_node.id).itemset) + " vs " + Arrays.toString(_node.itemset));
		exit(ERROR_ID_NEW_CI_ALREADY_REGISTERED);
	}

	CLOSED_ITEMSETS.emplace(_node->id, _node);
	NBR_CLOSED_NODES += 1;

	if (NBR_CLOSED_NODES != CLOSED_ITEMSETS.size()) {
		//System.out.println("after add erreur d'integrite " + NBR_CLOSED_NODES + " vs " + CLOSED_ITEMSETS.size());
		exit(ERROR_NBR_CLOSED_NODES_DOES_NOT_MATCH_CI_SET_SIZE);
	}

};

void delete_ci(CETNode* const _node, std::map<long, std::vector<std::vector<CETNode*>*>*>* const _EQ_TABLE) {
	//System.out.println("deleting "+Arrays.toString(_node.itemset)+" w/ "+_node.support);
	if (NBR_CLOSED_NODES != CLOSED_ITEMSETS.size()) {
		//System.out.println("before erreur d'integrite " + NBR_CLOSED_NODES + " vs " + CLOSED_ITEMSETS.size());
		exit(ERROR_NBR_CLOSED_NODES_DOES_NOT_MATCH_CI_SET_SIZE);
	}

	NBR_CLOSED_NODES -= 1;
	if (CLOSED_ITEMSETS.find(_node->id) == CLOSED_ITEMSETS.end()) {
		//System.out.println("(delete) oh shit, " + _node.id + " should " + Arrays.toString(_node.itemset) + " be here..." + _node.support);
		//new Throwable().printStackTrace(System.out);
		exit(ERROR_CANNOT_DELETE_UNREGISTRED_CI);
	}

	CLOSED_ITEMSETS.erase(_node->id);
	//_node.oldHash = _node.hash;
	remove_from_class(_node, _EQ_TABLE);

	//_node.parent.children.remove(_node.item);

	if (NBR_CLOSED_NODES != CLOSED_ITEMSETS.size()) {
		//System.out.println("erreur d'integrite " + NBR_CLOSED_NODES + " vs " + CLOSED_ITEMSETS.size());
		exit(ERROR_NBR_CLOSED_NODES_DOES_NOT_MATCH_CI_SET_SIZE);
	}
};

void prune_children(CETNode* const _node, const uint32_t _tid, std::map<long, std::vector<std::vector<CETNode*>*>*>* const _EQ_TABLE) {
	if (_node->children) {
		//pour chaque fils call prune_children 
		//check les fils, si closed remove dans la table
		//aussi remvove de CLOSED_ITEMSETS
		std::map<uint32_t, CETNode*>::iterator it = _node->children->begin();
		for (; it != _node->children->end(); ++it) {
			CETNode* const node = it->second;
			if (node->type == CLOSED_NODE) {
				delete_ci(node, _EQ_TABLE);
			}
			prune_children(node, _tid, _EQ_TABLE);
			NBR_NODES -= 1;
			delete node->tidlist;
			delete node->itemset;
			delete node->children;
			delete node;
		}
		_node->children->clear();
	}
};

void remove_from_class(CETNode* const _node, std::map<long, std::vector<std::vector<CETNode*>*>*>* const _EQ_TABLE) {
	{
		if (_EQ_TABLE->find(_node->oldHash) == _EQ_TABLE->end() || !_EQ_TABLE->find(_node->oldHash)->second) {
			_node->oldHash = _node->hash;
		}
		std::vector<CETNode*>* const ancienneClasse = _EQ_TABLE->find(_node->oldHash)->second->at(_node->itemset->size());
		uint32_t positionMatch = ancienneClasse->size();
		uint32_t cursor = 0;

		std::vector<CETNode*>::iterator it = ancienneClasse->begin();
		for (; it != ancienneClasse->end(); ++it) {
			CETNode* const n = *it;
			if (exactMatch(n->itemset, _node->itemset)) {
				positionMatch = cursor;//ok trouve, on doit le supprimer
				break;
			}
			cursor += 1;
		}

		if (positionMatch != ancienneClasse->size()) {
			ancienneClasse->erase(ancienneClasse->begin() + positionMatch);
		}
		else {
			//erreur
			std::vector<CETNode*>* const ancienneClasse2 = _EQ_TABLE->find(_node->hash)->second->at(_node->itemset->size());
			uint32_t positionMatch2 = ancienneClasse2->size();
			uint32_t cursor2 = 0;
			std::vector<CETNode*>::iterator it2 = ancienneClasse2->begin();
			for (; it2 != ancienneClasse2->end(); ++it2) {
				CETNode* const n = *it2;
				if (exactMatch(n->itemset, _node->itemset)) {
					positionMatch2 = cursor2;//ok trouve, on doit le supprimer
					break;
				}
				cursor2 += 1;
			}
			if (positionMatch2 != ancienneClasse2->size()) {
				//System.out.println("but was found in "+_node.hash+" !!!!");
				ancienneClasse2->erase(ancienneClasse2->begin() + positionMatch2);
			}
			else {
				//System.out.println("could not be found...");
				//System.out.println((_called_from ? "(rem)" : "(add)") + " could not find CI to transfer between classes..." + Arrays.toString(_node.itemset) + " has a support of s=" + _node.support + " hash=" + _node.hash + " oldHash=" + _node.oldHash + " tidset=" + Arrays.toString(_node.tidlist));
				//System.exit(1);
			}
			//System.exit(1);
		}
	}
};

void update_cetnode_in_hashmap(CETNode* const _node, std::map<long, std::vector<std::vector<CETNode*>*>*>* const _EQ_TABLE) {
	remove_from_class(_node, _EQ_TABLE);
	//add into new class
	//on ajoute le noeud dans la bonne classe d'equivalence (tidsum + support)
	{
		if (_EQ_TABLE->end() == _EQ_TABLE->find(_node->hash)) {
			_EQ_TABLE->emplace(_node->hash, new std::vector<std::vector<CETNode*>*>());
		}
		if (_EQ_TABLE->find(_node->hash)->second->size() <= _node->itemset->size()) {
			while (_EQ_TABLE->find(_node->hash)->second->size() <= _node->itemset->size()) {
				_EQ_TABLE->find(_node->hash)->second->push_back(new std::vector<CETNode*>());
			}
		}
		_EQ_TABLE->find(_node->hash)->second->at(_node->itemset->size())->push_back(_node);
	}
};

void print_cet_node(CETNode* const _node) {
	std::vector<uint32_t>::iterator it = _node->itemset->begin();
	for (; it != _node->itemset->end(); ++it) {
		std::cout << *it << ", ";
	}
	std::cout << std::endl;
}*/
