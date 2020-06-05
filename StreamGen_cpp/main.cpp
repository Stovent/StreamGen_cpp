#include "Transaction.h"
#include "StreamGen.h"

#include <cstdio>   //fopen, printf
#include <cstdlib>  //atol
#include <queue>    //sliding window
#include <cstring>  //strtok
#include <ctime>    //clock_t
#include <iostream> //cout
#include <fstream>
#include <iomanip>
#include <queue>
#include <map>
#include <vector>
#include <set>


#ifdef _WIN32
//#include <windows.h>
//#include <psapi.h>
#endif

uint32_t CET_NODE_ID = 0;
uint32_t NBR_NODES = 0;
uint32_t NBR_GENERATOR_NODES = 0;
uint32_t minsup = 0;
CETNode ROOT = CETNode();
std::map<uint32_t, CETNode*> CLOSED_ITEMSETS;

static std::string itemset_to_string(const std::vector<uint32_t>* itemset) {
    std::string str;
    for (const uint32_t item : *itemset) {
        str += std::to_string(item) + " ";
    }
    return str;
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        std::cout << "Usage: StreamGen_cpp.exe window_size item_number minsup inputfile outputfile" << std::endl;
        return 0;
    }
    clock_t start = clock(); clock_t running = clock();
    std::queue<Transaction<uint32_t>> window;
    const uint32_t window_size = strtoul(argv[1], 0, 10);//1500
    const uint32_t MAX_ATTRIBUTES = strtoul(argv[2], 0, 10);//100001
    minsup = strtoul(argv[3], 0, 10);//1
    std::ifstream input(argv[4]);
    if (!input.is_open()) {
        std::cout << "Cannot open input file " << argv[4] << std::endl;
        return 1;
    }
    std::ofstream output(argv[5]);
    if (!output.is_open()) {
        std::cout << "Cannot open output file " << argv[5] << std::endl;
        return 1;
    }

    std::cout << "  minsup: " << minsup << "; window_size: " << window_size << std::endl;

    ROOT.children = new std::map<uint32_t, CETNode*>();
    ROOT.itemset = new std::vector<uint32_t>();
    ROOT.tidsum = 0;
    ROOT.maxitem = 0;
    ROOT.type = GENERATOR_NODE;
    std::map<long, std::vector<std::vector<CETNode*>*>*> EQ_TABLE = std::map<long, std::vector<std::vector<CETNode*>*>*>();
    //uint32_t minsup = 0;

    //const uint32_t MAX_ATTRIBUTES = 1001;
    //initialiser l'arbre (autant de noeuds de d'items)
    //ou on peut le faire a chaque trx ? si nouvel item, on rajoute l'item dans l'arbre ?
    for (int i = 0; i != MAX_ATTRIBUTES; ++i) {
        CETNode* atom = new CETNode();
        ROOT.children->emplace(i, atom);
        atom->parent = &ROOT;
        atom->maxitem = i;
        atom->itemset = new std::vector<uint32_t>();
        atom->itemset->push_back(i);
        atom->type = INFREQUENT_NODE;//? a verifier, mais ca se tient
        atom->tidlist = new std::vector<uint32_t>();// [0];
        atom->tidsum = 0;
        atom->id = ++CET_NODE_ID;
        // atom->hash = 0;
        // atom->oldHash = 0;
        atom->support = 0;
        NBR_NODES += 1;
    }

    char s[10000];
    uint32_t i = 0;
    while (input.getline(s, 10000)) {
        char *pch = strtok(s, " ");
        //if (i > 9998) break;
        if (window_size != 0 && i >= window_size) {
            //delete
            Transaction<uint32_t> old_transaction = window.front();
            Deletion(1 + (i - window_size), old_transaction.data(), minsup, &ROOT, &EQ_TABLE);
            //std::cout << "removed something " << std::endl;
            window.pop();
        }
        Transaction<uint32_t> new_transaction = Transaction<uint32_t>(pch, " ", 0);
        //new_transaction.load(pch, " ", 0);
        //add
        //std::cout << "added something " << std::endl;
        Addition(i + 1, new_transaction.data());
        window.push(new_transaction);
        i += 1;

        if (i % 500 == 0){
        }
            std::cout << i << " transaction(s) processed" << std::endl;

#ifdef DEBUG
      if ((row % 1000 == 0 && row < 10001) || row % 10000 == 0) {
        printf("elapsed time between checkpoint %0.2f ms, ", (clock() - running) / (double)CLOCKS_PER_SEC * 1000);
          running = clock();
          cout << row << " rows processed, idx size/capacity:" << idx.size() << "/" << idx.capacity() << ", # concept:" << fCI2.size() << endl;
      }
#endif
    }

    printf("Stream completed in %0.2f sec, ", (clock() - start) / (double)CLOCKS_PER_SEC);
    std::cout << "NBR Generators: " << NBR_GENERATOR_NODES << std::endl;
    output << " node_id supp itemset" << std::endl;
    // export generators for debug
    std::queue<CETNode*> queue;
    queue.push(&ROOT);
    while (!queue.empty()) {
        CETNode* node = queue.front();
        queue.pop();

        if (node->type == GENERATOR_NODE) {
            output << std::setw(8) << node->id << " " << std::setw(4) << node->support << " " << itemset_to_string(node->itemset) << std::endl;
        }/*
        else {
            output << std::setw(13) << node->support << " " << itemset_to_string(node->itemset) << std::endl;
        }*/

        if (node->children) {
            for (const std::pair<uint32_t, CETNode*>& child : *node->children) {
                queue.push(child.second);
            }
        }
    }

    //nettoyage de l'arbre (TODO: put this in a function)
    //NOTA: peut etre prune_children pourrait faire l'affaire ici !!
    {
        //prune_children(&ROOT, 0, &EQ_TABLE);
        delete ROOT.children;
        delete ROOT.itemset;
        delete ROOT.tidlist;
        //nettoyer, children, itemset
        //y aller directement DFS
    }

    //nettoyage de la map (TODO: put this in a function)
    {
        std::map<long, std::vector<std::vector<CETNode*>*>*>::iterator it = EQ_TABLE.begin();
        for (; it != EQ_TABLE.end(); ++it) {
            std::vector<std::vector<CETNode*>*>* eq_class_stratified = it->second;
            std::vector<std::vector<CETNode*>*>::iterator eq_class_stratified_it = eq_class_stratified->begin();
            for (; eq_class_stratified_it != eq_class_stratified->end(); ++eq_class_stratified_it) {
            std::vector<CETNode*>* eq_class = *eq_class_stratified_it;
            delete eq_class;//Not necessary to clean referenced node pointers, they are cleaned below with the tree
            }
            delete eq_class_stratified;
        }
    }



#ifdef _WIN32
    //PROCESS_MEMORY_COUNTERS_EX info;
    //GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS *)&info, sizeof(info));
    //std::cout << "WorkingSet " << info.WorkingSetSize / 1024 << "K, PeakWorkingSet " << info.PeakWorkingSetSize / 1024 << "K, PrivateSet " << info.PrivateUsage / 1024 << "K" << endl;
#endif
    return EXIT_SUCCESS;
}
