#ifndef STREAMGEN_H
#define STREAMGEN_H

//#include "../CommonUtility/Utility.h"

#include <algorithm>
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

	std::vector<uint32_t>* itemset;
	std::map<uint32_t, CETNode*>* children;
	CETNode* parent;
	std::vector<uint32_t>* tidlist;
};

const uint8_t INFREQUENT_NODE   = 0x01;
const uint8_t UNPROMISSING_NODE = 0x02;
const uint8_t GENERATOR_NODE     = 0x03;

void Explore(CETNode* const node);
void Addition(const uint32_t tid, std::vector<uint32_t>* transaction);
void Deletion(const uint32_t tid, std::vector<uint32_t>* transaction);

void identify(CETNode* node);
void new_child(CETNode* node, uint32_t maxitem, std::vector<uint32_t>* tidlist, bool _identify);
bool has_child(const CETNode* node, const uint32_t maxitem);
void clean_children(CETNode* node);
void remove_child(CETNode* node, uint32_t item);
void clean(CETNode* node);

// utility
CETNode* create_node(CETNode* parent, uint32_t maxitem, std::vector<uint32_t>* tidlist, bool _identify);
bool itemset_is_a_generator(const std::vector<uint32_t>* itemset, const uint32_t refsup);
bool is_contained_strict(const std::vector<uint32_t>* contained, const std::vector<uint32_t>* container);
std::string itemset_to_string(const std::vector<uint32_t>* itemset);
void add_generator(CETNode* node);
void remove_generator(CETNode* node);
/*
void add_ci(CETNode* const _node, std::map<long, std::vector<std::vector<CETNode*>*>*>* const _EQ_TABLE);
void delete_ci(CETNode* const _node, std::map<long, std::vector<std::vector<CETNode*>*>*>* const _EQ_TABLE);
void prune_children(CETNode* const _node, const uint32_t _tid, std::map<long, std::vector<std::vector<CETNode*>*>*>* const _EQ_TABLE);
void remove_from_class(CETNode* const _node, std::map<long, std::vector<std::vector<CETNode*>*>*>* const _EQ_TABLE);
void update_cetnode_in_hashmap(CETNode* const _node, std::map<long, std::vector<std::vector<CETNode*>*>*>* const _EQ_TABLE);
void print_cet_node(CETNode* const _node);*/


//error codes
const uint32_t ERROR_INCORRECT_CREATION_OF_NODE = 0x01;//An invalid node has been created: its support is higher than at least one of its parents (i.e. violates downward closure property)
const uint32_t ERROR_COULD_NOT_FIND_CI_OLD_CLASS = 0x02;//An existing CI's class could not be found in hashtable: either its hash is invalid or the hashtable is not correct (i.e. integrity error)
const uint32_t ERROR_COULD_NOT_FIND_CI = 0x04;//An existing CI class could not be found in hashtable: either its hash is invalid or the hashtable is not correct (i.e. integrity error)
const uint32_t ERROR_DELETE_NODE_WITH_EMPTY_TIDSET = 0x08;//A node has an empty tidset: then it should not exist (i.e. integrity error)
const uint32_t ERROR_DELETE_NODE_WITH_HASH = 0x10;
const uint32_t ERROR_DELETE_NODE_CONTAINS_TRX_BUT_NOT_IN_TIDSET = 0x20;
const uint32_t ERROR_DELETE_REMOVING_INFREQUENT_CI = 0x40;//Removing some infrequent closed node
const uint32_t ERROR_NBR_CLOSED_NODES_DOES_NOT_MATCH_CI_SET_SIZE = 0x80;
const uint32_t ERROR_ID_NEW_CI_ALREADY_REGISTERED = 0x100;
const uint32_t ERROR_CANNOT_DELETE_UNREGISTRED_CI = 0x200;
#endif // STREAMGEN_H
