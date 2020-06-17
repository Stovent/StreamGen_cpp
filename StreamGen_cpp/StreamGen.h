#ifndef STREAMGEN_H
#define STREAMGEN_H

#include <map>
#include <string>
#include <vector>

struct CETNode {
	uint8_t type;
	uint32_t support;
	uint32_t maxitem; // theorically, item and maxitem should refer to the same item.
					  // is using children[children->size() - 1] a better solution?
	uint32_t tidsum = 0;
	uint32_t id;
	uint32_t itemsum;

	std::vector<uint32_t>* itemset;
	std::map<uint32_t, CETNode*>* children;
	CETNode* parent;
	std::vector<uint32_t>* tidlist;
	
	~CETNode() {
		delete itemset;
		delete tidlist;
		if (children) {
			for (const std::pair<uint32_t, CETNode*> child : *children) {
				delete child.second;
			}
		}
		delete children;
	}
};

const uint8_t INFREQUENT_NODE   = 0x01;
const uint8_t UNPROMISSING_NODE = 0x02;
const uint8_t GENERATOR_NODE     = 0x03;

// given complexity is worst case
void Explore(CETNode* const node); // O(n * (O(has_child) + O(new_child))), n = node->parent->children->size()
void Addition(const uint32_t tid, std::vector<uint32_t>* transaction); // O(n * m * O(identify) * O(explore) * O(new_child)), n = number of nodes in the CET, m = complexity of function inter (O(left->size() * right->size())
void Deletion(const uint32_t tid, std::vector<uint32_t>* transaction); // O(n * m * O(identify) * O(clean) * O(remove_child)), n = number of nodes in the CET, m = complexity of function inter (O(left->size() * right->size())

void identify(CETNode* node); // O(O(node_is_a_generator) + O(add_generator))
void new_child(CETNode* parent, uint32_t maxitem, std::vector<uint32_t>* tidlist, bool _identify); // O(create_node)
bool has_child(const CETNode* node, const uint32_t maxitem); // O(n), n = complexity to find key in node->children (usually log(n))
void clean_children(CETNode* node); // O(n), n = node->children->size()
void remove_child(CETNode* node, uint32_t item); // O(n), n = complexity to find key in node->children (usually log(n))
void clean(CETNode* node); // O(O(remove_generator) + O(clean_children) + n * m * O(has_child) * O(clean) * O(remove_child)), n = node->parent->children->size(), m = complexity to find key in node->parent->children (usually log(n))

// utility, giving worst case complexity
CETNode* create_node(CETNode* parent, uint32_t maxitem, std::vector<uint32_t>* tidlist, bool _identify); // O(n + m + O(identify)), n = tidlist->size(), m = parent->itemset->size()
bool node_is_a_generator(const CETNode* node); // time complexity:
	// O(L + n * L * m * O(contains)), O(contains) = _a.size() * _b.size(), L = time complexity to search for a key in a map (usually logarithmic), n = node->itemset->size(), m = number of nodes in GENERATORS[node->itemset->size()][node->itemsum]
bool is_contained_strict(const std::vector<uint32_t>* contained, const std::vector<uint32_t>* container); // O(n * m), n = contained->size(), m = container->size()
std::string itemset_to_string(const std::vector<uint32_t>* itemset); // O(n), n = itemset->size()
void add_generator(CETNode* node); // O(n), n = complexity to add an item to a map (usually logarithmic)
void remove_generator(CETNode* node); // O(n), n = number of nodes in GENERATORS[node->itemset->size()][node->itemsum]
uint32_t get_itemsum(const std::vector<uint32_t>* itemset); // O(n), n = itemset->size()

#endif // STREAMGEN_H
