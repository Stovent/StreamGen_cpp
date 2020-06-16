#include "StreamGen.h"
#include "Utility.h"
#include <iostream>
#include <queue>
#include <stack>

extern uint32_t CET_NODE_ID;
extern uint32_t NBR_GENERATOR_NODES;
extern uint32_t minsup;
extern CETNode ROOT;
extern std::map<uint32_t, std::map<uint32_t, std::vector<CETNode*>>> GENERATORS; // <itemset size, <itemsum, nodes>>

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
	else if (node_is_a_generator(node)) {
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

void new_child(CETNode* parent, uint32_t maxitem, std::vector<uint32_t>* tidlist, bool _identify) {
	if (!parent->children) {
		parent->children = new std::map<uint32_t, CETNode*>();
	}
	parent->children->emplace(maxitem, create_node(parent, maxitem, tidlist, _identify));
}

bool has_child(const CETNode* node, const uint32_t maxitem) {
	if (!node->children)
		return false;
	
	return node->children->find(maxitem) != node->children->end();
}

void clean_children(CETNode* node) {
	if (node->children) {
		for (const std::pair<uint32_t, CETNode*>& child : *node->children) {
			if (child.second->type == GENERATOR_NODE) {
				remove_generator(child.second);
			}
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
	node->itemsum = get_itemsum(node->itemset);

	if (_identify) {
		identify(node);
	}
	else if(node->support > minsup) {
		node->type = UNPROMISSING_NODE;
	}
	return node;
}

#ifndef USE_MAP_FOR_GEN
bool node_is_a_generator(const CETNode* node) {
	uint32_t itemsum = node->itemsum;

	std::map<uint32_t, std::map<uint32_t, std::vector<CETNode*>>>::iterator git = GENERATORS.find(node->itemset->size() - 1);
	if (git == GENERATORS.end())
		return false;

	std::map<uint32_t, std::vector<CETNode*>>& generators = git->second;

	for (std::vector<uint32_t>::iterator it = node->itemset->begin(); it != node->itemset->end(); it++) {
		itemsum -= *it;

		std::map<uint32_t, std::vector<CETNode*>>::const_iterator git = generators.find(itemsum);
		if (git == generators.end())
			return false;

		for (const CETNode* gen : generators[itemsum]) {
			if (contains(node->itemset, gen->itemset, true)) { // can this be removed ?
				if (gen->support == node->support) {
					return false;
				}
			}
		}
		itemsum += *it;
	}
	
	return true;
}
#else
bool itemset_is_a_generator(const std::vector<uint32_t>* itemset, const uint32_t refsup) {
	std::queue<CETNode*> queue;
	queue.push(&ROOT);

	while (!queue.empty()) {
		CETNode* node = queue.front();
		queue.pop();

		if (node->itemset->size() > 0) {
			//if(!contains(itemset, node->itemset, true))
			if (!is_contained_strict(node->itemset, itemset))
				continue;
		}

		if (node->type != GENERATOR_NODE) {
			return false;
		}
		if (node->support == refsup) {
			return false;
		}

		if (node->children) {
			for (const std::pair<uint32_t, CETNode*>& child : *node->children) {
				queue.push(child.second);
			}
		}
	}
	return true;
}

#endif


bool is_contained_strict(const std::vector<uint32_t>* contained, const std::vector<uint32_t>* container) {
	uint32_t match = 0;

	for (const uint32_t cmp : *contained) {
		bool any = false;
		for (const uint32_t ref : *container) {
			if (ref == cmp) {
				match++;
				any = true;
			}
		}
		if (!any)
			return false;
	}

	if (match > 0 && match < container->size())
		return true;
	
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
	GENERATORS[node->itemset->size()][node->itemsum].push_back(node);
}

void remove_generator(CETNode* node) {
	std::vector<CETNode*>& v = GENERATORS[node->itemset->size()][node->itemsum];
	v.erase(std::find(v.begin(), v.end(), node));
}

uint32_t get_itemsum(const std::vector<uint32_t>* itemset) {
	uint32_t sum = 0;
	for (const uint32_t item : *itemset) {
		sum += item;
	}
	return sum;
}
