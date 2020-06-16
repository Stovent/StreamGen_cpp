#ifndef STREAMGEN_H
#define STREAMGEN_H

#include <map>
#include <string>
#include <vector>
#include <memory>

struct CETNode {
	uint8_t type;
	uint32_t support;
	uint32_t maxitem; // theorically, item and maxitem should refer to the same item.

	uint32_t tidsum = 0;
	uint32_t id;
	uint32_t itemsum;

	std::shared_ptr<CETNode> parent;
	std::vector<uint32_t> itemset;
	std::map<uint32_t, std::shared_ptr<CETNode>> children;
	std::vector<uint32_t> tidlist;
};

const uint8_t INFREQUENT_NODE   = 0x01;
const uint8_t UNPROMISSING_NODE = 0x02;
const uint8_t GENERATOR_NODE    = 0x03;

void Explore(std::shared_ptr<CETNode> node);
void Addition(const uint32_t tid, std::vector<uint32_t>* transaction);
void Deletion(const uint32_t tid, std::vector<uint32_t>* transaction);

void identify(std::shared_ptr<CETNode> node);
void new_child(std::shared_ptr<CETNode> parent, uint32_t maxitem, std::vector<uint32_t>* tidlist, bool _identify);
bool has_child(const CETNode* node, const uint32_t maxitem);
void clean_children(std::shared_ptr<CETNode> node);
void remove_child(std::shared_ptr<CETNode> node, uint32_t item);
void clean(std::shared_ptr<CETNode> node);

// utility
std::shared_ptr<CETNode> create_node(std::shared_ptr<CETNode> parent, uint32_t maxitem, std::vector<uint32_t>* tidlist, bool _identify);
bool node_is_a_generator(const CETNode* node);
bool is_contained_strict(const std::vector<uint32_t>* contained, const std::vector<uint32_t>* container);
std::string itemset_to_string(const std::vector<uint32_t>* itemset);
void add_generator(std::shared_ptr<CETNode> node);
void remove_generator(std::shared_ptr<CETNode> node);
uint32_t get_sum(const std::vector<uint32_t>& itemset);

#endif // STREAMGEN_H
