#include "StreamGen.h"
#include "Utility.h"
#include <iostream>
#include <queue>
#include <stack>

extern uint32_t CET_NODE_ID;
extern uint32_t NBR_GENERATOR_NODES;
extern uint32_t minsup;
extern CETNode ROOT;
extern std::map<uint32_t, std::vector<CETNode*>> GENERATORS; // <itemset size, nodes>

void Explore(CETNode* const node) {
	std::map<uint32_t, CETNode*>* siblings = node->parent->children;

	for (const std::pair<uint32_t, CETNode*>& sibling : *siblings) {
		if (sibling.second->type == GENERATOR_NODE && sibling.second->maxitem > node->maxitem) {
			new_child(node, sibling.second->maxitem, inter(node->tidlist, sibling.second->tidlist), true);
		}
		else if (sibling.second->type == GENERATOR_NODE && sibling.second->maxitem < node->maxitem) {
			if (!has_child(sibling.second, node->maxitem)) {
				new_child(sibling.second, node->maxitem, inter(sibling.second->tidlist, node->tidlist), true);
			}
		}
	}
};

void Addition(const uint32_t tid, std::vector<uint32_t>* transaction) {
	std::queue<CETNode*> queue;
	queue.push(&ROOT);

	while (!queue.empty()) {
		CETNode* node = queue.front();
		queue.pop();

		std::vector<uint32_t>* intersec = inter(node->itemset, transaction);
		if (intersec->size() == node->itemset->size() - 1) {
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
		else if (intersec->size() == node->itemset->size()) {
			node->support++;
			if (node->tidlist)
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
					if (node->parent) {
						std::map<uint32_t, CETNode*>::iterator child = node->parent->children->find(item);
						if (child != node->parent->children->end() && child->second->type == GENERATOR_NODE) {
							new_child(node, item, inter(node->tidlist, child->second->tidlist), false);
						}
					}
				}
			}
		}

		delete intersec;
	}
};

void Deletion(const uint32_t tid, std::vector<uint32_t>* transaction) {
	std::queue<CETNode*> queue;
	queue.push(&ROOT);

	while (!queue.empty()) {
		CETNode* node = queue.front();
		queue.pop();

		std::vector<uint32_t>* intersec = inter(node->itemset, transaction);
		if (intersec->size() == node->itemset->size() - 1) {
			if (node->type == GENERATOR_NODE) {
				identify(node);
				if (node->type == UNPROMISSING_NODE) {
					clean(node);
				}
				else {
					if (node->children) {
						for (std::map<uint32_t, CETNode*>::const_reverse_iterator child = node->children->crbegin(); child != node->children->crend(); child++) {
							if (child->second->type == GENERATOR_NODE) {
								queue.push(child->second);
							}
						}
					}
				}
			}
		}
		else if (intersec->size() == node->itemset->size()) {
			node->support--;
			if (node->tidlist)
				node->tidlist->erase(std::find(node->tidlist->begin(), node->tidlist->end(), tid));
			node->tidsum -= tid;

			if (node->type == INFREQUENT_NODE && node->support == 0) {
				remove_child(node->parent, node->maxitem);
			}
			else {
				if (node->support < minsup) {
					if (node->type == GENERATOR_NODE) {
						clean(node);
					}
					node->type = INFREQUENT_NODE;
				}
				else {
					if (node->type == GENERATOR_NODE && node->children) {
						for (std::map<uint32_t, CETNode*>::const_reverse_iterator child = node->children->crbegin(); child != node->children->crend(); child++) {
							queue.push(child->second);
						}
					}
				}
			}
		}

		delete intersec;
	}
};

void identify(CETNode* node) {
	if (node->support < minsup) {
		node->type = INFREQUENT_NODE;
	}
	else if (itemset_is_a_generator(node->itemset, node->support)) {
		if (node->type != GENERATOR_NODE) {
			NBR_GENERATOR_NODES++;
			add_generator(node);
		}
		node->type = GENERATOR_NODE;
	}
	else {
		node->type = UNPROMISSING_NODE;
	}
}

void new_child(CETNode* node, uint32_t maxitem, std::vector<uint32_t>* tidlist, bool _identify) {
	if (!node->children) {
		node->children = new std::map<uint32_t, CETNode*>();
	}
	node->children->emplace(maxitem, create_node(node, maxitem, tidlist, _identify));
}

bool has_child(const CETNode* node, const uint32_t maxitem) {
	if (!node->children)
		return false;
	
	return node->children->find(maxitem) != node->children->end();
}

void clean_children(CETNode* node) {
	if (node->children) {
		for (const std::pair<uint32_t, CETNode*>& child : *node->children) {
			delete child.second;
		}
		node->children->clear();
	}
}

void remove_child(CETNode* node, uint32_t item) {
	std::map<uint32_t, CETNode*>::iterator it = node->children->find(item);
	if (it != node->children->end()) {
		delete it->second;
		node->children->erase(it);
	}
}

void clean(CETNode* node) {
	NBR_GENERATOR_NODES--;
	remove_generator(node);

	clean_children(node);

	for (const std::pair<uint32_t, CETNode*>& sibling : *node->parent->children) {
		if (sibling.second->type == GENERATOR_NODE && sibling.second->maxitem < node->maxitem) {
			if (has_child(sibling.second, node->maxitem)) {

				CETNode* child = (*sibling.second->children)[node->maxitem];
				if (child->type == GENERATOR_NODE) {
					clean(child);
				}
				remove_child(child->parent, child->maxitem);
			}
		}
	}
}

//Below are utility functions for StreamGen (non in algorithm).

CETNode* create_node(CETNode* parent, uint32_t maxitem, std::vector<uint32_t>* tidlist, bool _identify) {
	CETNode* node = new CETNode;
	node->id = ++CET_NODE_ID;
	node->maxitem = maxitem;
	node->parent = parent;
	node->tidlist = tidlist;
	node->support = node->tidlist->size();
	node->children = nullptr;
	node->type = INFREQUENT_NODE;

	node->tidsum = 0;
	for (const uint32_t tid : *node->tidlist) {
		node->tidsum += tid;
	}

	node->itemset = new std::vector<uint32_t>(parent->itemset->begin(), parent->itemset->end());
	node->itemset->push_back(maxitem);
	if (_identify) {
		identify(node);
	}
	else if(node->support > minsup) {
		node->type = UNPROMISSING_NODE;
	}
	return node;
}

bool itemset_is_a_generator(const std::vector<uint32_t>* itemset, const uint32_t refsup) {
	for (const CETNode* node : GENERATORS[itemset->size() - 1]) {
		if (contains(itemset, node->itemset, true)) {
			if (node->support == refsup)
				return false;
		}
	}
	
	return true;
}

bool is_contained_strict(const std::vector<uint32_t>* compared, const std::vector<uint32_t>* reference) {
	uint32_t match = 0;

	for (const uint32_t cmp : *compared) {
		bool any = false;
		for (const uint32_t ref : *reference) {
			if (ref == cmp) {
				match++;
				any = true;
			}
		}
		if (!any)
			return false;
	}

	if (match > 0 && match < reference->size()) {
		/*std::cout << " compared ";
		for (const uint32_t i : *compared)
			std::cout << i << " ";
		std::cout << " is in reference ";
		for (const uint32_t i : *reference)
			std::cout << i << " ";
		std::cout << std::endl << std::endl;*/
		return true;
	}
	
	return false;
}

std::string itemset_to_string(const std::vector<uint32_t>* itemset) {
	std::string str;
	for (const uint32_t item : *itemset) {
		str += std::to_string(item) + " ";
	}
	return str;
}

void add_generator(CETNode* node) {
	GENERATORS[node->itemset->size()].push_back(node);
}

void remove_generator(CETNode* node) {
	std::vector<CETNode*>& v = GENERATORS[node->itemset->size()];
	v.erase(std::find(v.begin(), v.end(), node));
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
