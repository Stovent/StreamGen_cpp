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

void Explore(CETNode* const node);
void Addition(const uint32_t tid, std::vector<uint32_t>* transaction);
void Deletion(const uint32_t tid, std::vector<uint32_t>* transaction);

void identify(CETNode* node);
void new_child(CETNode* parent, uint32_t maxitem, std::vector<uint32_t>* tidlist, bool _identify);
bool has_child(const CETNode* node, const uint32_t maxitem);
void clean_children(CETNode* node);
void remove_child(CETNode* node, uint32_t item);
void clean(CETNode* node);

// utility
CETNode* create_node(CETNode* parent, uint32_t maxitem, std::vector<uint32_t>* tidlist, bool _identify);
bool node_is_a_generator(const CETNode* node);
bool is_contained_strict(const std::vector<uint32_t>* contained, const std::vector<uint32_t>* container);
std::string itemset_to_string(const std::vector<uint32_t>* itemset);
void add_generator(CETNode* node);
void remove_generator(CETNode* node);
uint32_t get_itemsum(const std::vector<uint32_t>* itemset);

#endif // STREAMGEN_H
