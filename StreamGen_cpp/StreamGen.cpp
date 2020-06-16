#include "StreamGen.h"
#include "Utility.h"
#include <iostream>
#include <queue>
#include <stack>

extern uint32_t CET_NODE_ID;
extern uint32_t NBR_GENERATOR_NODES;
extern uint32_t minsup;
extern std::shared_ptr<CETNode> ROOT;
extern std::map<uint32_t, std::map<uint32_t, std::vector<std::shared_ptr<CETNode>>>> GENERATORS; // <itemset size, <itemsum, nodes>>

void Explore(std::shared_ptr<CETNode> node) {
	std::map<uint32_t, std::shared_ptr<CETNode>>& siblings = node->parent->children;

	for (const std::pair<uint32_t, std::shared_ptr<CETNode>>& sibling : siblings) {
		if (sibling.second->type == GENERATOR_NODE && sibling.second->maxitem > node->maxitem) {
			new_child(node, sibling.second->maxitem, inter(&node->tidlist, &sibling.second->tidlist), true);
		}
		else if (sibling.second->type == GENERATOR_NODE && sibling.second->maxitem < node->maxitem) {
			if (!has_child(sibling.second.get(), node->maxitem)) {
				new_child(sibling.second, node->maxitem, inter(&sibling.second->tidlist, &node->tidlist), true);
			}
		}
	}
};

void Addition(const uint32_t tid, std::vector<uint32_t>* transaction) {
	std::queue<std::shared_ptr<CETNode>> queue;
	queue.push(ROOT);

	while (!queue.empty()) {
		std::shared_ptr<CETNode> node = queue.front();
		queue.pop();

		std::vector<uint32_t>* intersec = inter(&node->itemset, transaction);
		if (intersec->size() == node->itemset.size() - 1) {
			if (node->type == UNPROMISSING_NODE) {
				identify(node);
				if (node->type == UNPROMISSING_NODE)
					continue;

				Explore(node);
			}
			for (std::map<uint32_t, std::shared_ptr<CETNode>>::const_reverse_iterator child = node->children.crbegin(); child != node->children.crend(); child++) {
				if (child->second->type != INFREQUENT_NODE)
					queue.push(child->second);
			}
		}
		else if (intersec->size() == node->itemset.size()) {
			node->support++;
			node->tidlist.push_back(tid);
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
				for (std::map<uint32_t, std::shared_ptr<CETNode>>::const_reverse_iterator child = node->children.crbegin(); child != node->children.crend(); child++) {
					queue.push(child->second);
				}

				for (uint32_t item : *transaction) {
					if (item <= node->maxitem)
						continue;

					// create childs of node using his lexicographically right siblings
					if (node->parent) {
						std::map<uint32_t, std::shared_ptr<CETNode>>::iterator child = node->parent->children.find(item);
						if (child != node->parent->children.end() && child->second->type == GENERATOR_NODE) {
							new_child(node, item, inter(&node->tidlist, &child->second->tidlist), false);
						}
					}
				}
			}
		}

		delete intersec;
	}
};

void Deletion(const uint32_t tid, std::vector<uint32_t>* transaction) {
	std::queue<std::shared_ptr<CETNode>> queue;
	queue.push(ROOT);

	while (!queue.empty()) {
		std::shared_ptr<CETNode> node = queue.front();
		queue.pop();

		std::vector<uint32_t>* intersec = inter(&node->itemset, transaction);
		if (intersec->size() == node->itemset.size() - 1) {
			if (node->type == GENERATOR_NODE) {
				identify(node);
				if (node->type == UNPROMISSING_NODE) {
					clean(node);
				}
				else {
					for (std::map<uint32_t, std::shared_ptr<CETNode>>::const_reverse_iterator child = node->children.crbegin(); child != node->children.crend(); child++) {
						if (child->second->type == GENERATOR_NODE) {
							queue.push(child->second);
						}
					}
				}
			}
		}
		else if (intersec->size() == node->itemset.size()) {
			node->support--;
			node->tidlist.erase(std::find(node->tidlist.begin(), node->tidlist.end(), tid));
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
					if (node->type == GENERATOR_NODE) {
						for (std::map<uint32_t, std::shared_ptr<CETNode>>::const_reverse_iterator child = node->children.crbegin(); child != node->children.crend(); child++) {
							queue.push(child->second);
						}
					}
				}
			}
		}

		delete intersec;
	}
};

void identify(std::shared_ptr<CETNode> node) {
	if (node->support < minsup) {
		node->type = INFREQUENT_NODE;
	}
	else if (node_is_a_generator(node.get())) {
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

void new_child(std::shared_ptr<CETNode> parent, uint32_t maxitem, std::vector<uint32_t>* tidlist, bool _identify) {
	std::shared_ptr<CETNode> node = std::make_shared<CETNode>();
	node->id = ++CET_NODE_ID;
	node->maxitem = maxitem;
	node->parent = parent;
	node->tidlist.assign(tidlist->cbegin(), tidlist->cend());
	node->support = node->tidlist.size();
	node->type = INFREQUENT_NODE;

	node->tidsum = get_sum(node->tidlist);

	node->itemset.assign(parent->itemset.begin(), parent->itemset.end());
	node->itemset.push_back(maxitem);
	node->itemsum = get_sum(node->itemset);

	if (_identify) {
		identify(node);
	}
	else if (node->support > minsup) {
		node->type = UNPROMISSING_NODE;
	}

	parent->children.emplace(maxitem, node);
}

bool has_child(const CETNode* node, const uint32_t maxitem) {
	return node->children.find(maxitem) != node->children.end();
}

void clean_children(std::shared_ptr<CETNode> node) { // should delete child
	for (const std::pair<uint32_t, std::shared_ptr<CETNode>>& child : node->children) {
		if (child.second->type == GENERATOR_NODE) { // instead, add a destructor which removes himself from the generators
			remove_generator(child.second);
		}
	}
	node->children.clear();
}

void remove_child(std::shared_ptr<CETNode> node, uint32_t item) { // should delete child
	std::map<uint32_t, std::shared_ptr<CETNode>>::iterator it = node->children.find(item);
	if (it != node->children.end()) {
		node->children.erase(it); // here, erase should decrease the reference count to 0, which will delete and free the node
	}
}

void clean(std::shared_ptr<CETNode> node) {
	NBR_GENERATOR_NODES--;
	remove_generator(node);

	clean_children(node);

	for (const std::pair<uint32_t, std::shared_ptr<CETNode>>& sibling : node->parent->children) {
		if (sibling.second->type == GENERATOR_NODE && sibling.second->maxitem < node->maxitem) {
			if (has_child(sibling.second.get(), node->maxitem)) {

				std::shared_ptr<CETNode> child = sibling.second->children[node->maxitem];
				if (child->type == GENERATOR_NODE) {
					clean(child);
				}
				remove_child(child->parent, child->maxitem);
			}
		}
	}
}

//Below are utility functions for StreamGen (non in algorithm).

std::shared_ptr<CETNode> create_node(std::shared_ptr<CETNode> parent, uint32_t maxitem, std::vector<uint32_t>* tidlist, bool _identify) {
	std::shared_ptr<CETNode> node = std::make_shared<CETNode>();
	node->id = ++CET_NODE_ID;
	node->maxitem = maxitem;
	node->parent = parent;
	node->tidlist.assign(tidlist->cbegin(), tidlist->cend());
	node->support = node->tidlist.size();
	node->type = INFREQUENT_NODE;

	node->tidsum = get_sum(node->tidlist);

	node->itemset.assign(parent->itemset.begin(), parent->itemset.end());
	node->itemset.push_back(maxitem);
	node->itemsum = get_sum(node->itemset);

	if (_identify) {
		identify(node);
	}
	else if(node->support > minsup) {
		node->type = UNPROMISSING_NODE;
	}
	return node;
}

bool node_is_a_generator(const CETNode* node) {
	uint32_t itemsum = node->itemsum;

	std::map<uint32_t, std::map<uint32_t, std::vector<std::shared_ptr<CETNode>>>>::iterator git = GENERATORS.find(node->itemset.size() - 1);
	if (git == GENERATORS.end())
		return false;

	std::map<uint32_t, std::vector<std::shared_ptr<CETNode>>>& generators = git->second;

	for (std::vector<uint32_t>::const_iterator it = node->itemset.cbegin(); it != node->itemset.cend(); it++) {
		itemsum -= *it;

		std::map<uint32_t, std::vector<std::shared_ptr<CETNode>>>::const_iterator git = generators.find(itemsum);
		if (git == generators.end())
			return false;

		for (const std::shared_ptr<CETNode>& gen : generators[itemsum]) {
			if (contains(&node->itemset, &gen->itemset, true)) { // can this be removed ?
				if (gen->support == node->support) {
					return false;
				}
			}
		}
		itemsum += *it;
	}
	
	return true;
}

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

void add_generator(std::shared_ptr<CETNode> node) {
	GENERATORS[node->itemset.size()][node->itemsum].push_back(node);
}

void remove_generator(std::shared_ptr<CETNode> node) {
	std::vector<std::shared_ptr<CETNode>>& v = GENERATORS[node->itemset.size()][node->itemsum];
	v.erase(std::find(v.begin(), v.end(), node));
}

uint32_t get_sum(const std::vector<uint32_t>& itemset) {
	uint32_t sum = 0;
	for (const uint32_t item : itemset) {
		sum += item;
	}
	return sum;
}
