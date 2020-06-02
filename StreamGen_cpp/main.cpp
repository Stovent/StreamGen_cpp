//variables defined for book-keeping
//FP::numberOfFPNodes
//CET::numberOfCETNodes

#include "misc.h"
#include "FP.h"
#include "CET.h"

#ifdef _WIN32
	#include <windows.h>
	#include <psapi.h>
#endif

unsigned int WINDOW_SIZE;
unsigned int SUPPORT;
unsigned short MAX_ITEM;
unsigned int TRANSACTION_SIZE;

const unsigned short ENDSHORT = 65535;
const int ENDINT = 2000000001;

long FPNode::numberOfFPNodes = 0;
long TreeNode::numberOfCETNodes = 0;

long numberOfExploreCall = 0; //number of newly frequent and newly promising, for an addition

int main(int argc, char* argv[])
{
	if (argc != 6) {
		cout << "Usage: " << argv[0] << " window_size support item_size input_file output_file" << endl;
		exit(1);
	}

	istringstream iss1(argv[1]);
	iss1 >> WINDOW_SIZE;
	if (!iss1) {
		cerr << "Invalid window_size, not an integer value!" << endl;
		exit(1);
	}

	istringstream iss2(argv[2]);
	iss2 >> SUPPORT;
	if (!iss2 && SUPPORT < 1 && SUPPORT > WINDOW_SIZE) {
		cerr << "Invalid support, value must be greater than 0 and should be smaller or equal than window_size!" << endl;
		exit(1);
	}

	istringstream iss3(argv[3]);
	iss3 >> MAX_ITEM;
	if (!iss3 && MAX_ITEM <= 0) {
		cerr << "Invalid number of items, not a positive integer value!" << endl;
		exit(1);
	}

	string inputFile = argv[4];
	string outputFile = argv[5];

	ofstream outFile(outputFile.c_str());
	if (!outFile) {
		cerr << "cannot open OUTPUT file!" << endl;
		exit(1);
	}

	ifstream inFile(inputFile.c_str());
	if (!inFile) {
		cerr << "cannot open INPUT file!" << endl;
		outFile.close();
		exit(1);
	}

	clock_t t1 = clock();

	FP mainFPTree(MAX_ITEM);

	unsigned int tid = 0;
	while (tid < WINDOW_SIZE && !inFile.eof()) {
		++tid;
		vector<unsigned short> items;
		string line;
		string token;
		std::getline(inFile, line);
		istringstream iss(line);
		while (getline(iss, token, ' ')) {
			items.push_back(stoi(token));
		}
		mainFPTree.addItemset(items, tid);
	}

	/*for ( int i = WINDOW_SIZE; i > 0; i-- ) {
		inFile >> dummyInt;
		if ( !inFile ) {
			cerr << "not enough transactions for one window!" << endl;
			inFile.close();
			outFile.close();
			exit(1);
		}
		inFile >> dummyInt;
		int length;
		inFile >> length;
		vector<unsigned short> items;
		for ( int j = 0; j < length; j++ ) {
			inFile >> dummyShort;
			items.push_back(dummyShort);
		}
		mainFPTree.addItemset(items, dummyInt);
	}*/

	outFile << "FP_tree_size: " << FPNode::numberOfFPNodes << " in " << static_cast<float>(clock() - t1) / CLOCKS_PER_SEC << endl;

	t1 = clock();

	CET mainCET;
	mainCET.initialize(mainFPTree);

	float initialFPTree_time = static_cast<float>(clock() - t1) / CLOCKS_PER_SEC;
	outFile << "Main window of size:" << tid << " in " << initialFPTree_time << "s, closed itemset:" << mainCET.closedItemsets.size() << ", # CET nodes:" << TreeNode::numberOfCETNodes << endl;

	////debug
	//cout << "***********************************" << endl;
	//mainCET.printMe(mainCET.CETRoot,0);
	//cout << endl;
	//mainCET.printHash();
	//cout << endl;
	float avgTime = 0;
	float totalTime = 0;
	long totalExplore = 0;
	long totalAddedNodes = 0;
	long totalDeletedNodes = 0;
	size_t totalClosed = 0;
	long totalCETNodes = 0;

	long previousNodes;

	outFile << "  #   sec     CI- CET_nodes  # call  +nodes  -nodes    CI+" << endl;

	int i = 0;
	while (!inFile.eof()) {
		++i;
		numberOfExploreCall = 0;
		long addedNodes = 0;
		long deletedNodes = 0;
		previousNodes = TreeNode::numberOfCETNodes;

		//fix start
		vector<unsigned short> items;
		string line;
		string token;
		std::getline(inFile, line);
		istringstream iss(line);
		while (getline(iss, token, ' ')) {
			items.push_back(stoi(token));
		}
		//fix end

			//inFile >> dummyInt;
			//if ( inFile.eof() ) break;
			//inFile >> dummyInt;
			//int length;
			//inFile >> length;
			//vector<unsigned short> myItems;
			//for ( int j = 0; j < length; j++ ) {
			//	inFile >> dummyShort;
			//	myItems.push_back(dummyShort);
			//}

		t1 = clock();

		++tid;
		int t = tid; // tid temp needed in deleteItemset
		mainFPTree.addItemset(items, t);
		mainCET.addition(t, items, mainFPTree);

		////debug
		//cout << "***********************************" << endl;
		//mainCET.printMe(mainCET.CETRoot,0);
		//cout << endl;
		//mainCET.printHash();
		//cout << endl;

		addedNodes = TreeNode::numberOfCETNodes - previousNodes;
		previousNodes = TreeNode::numberOfCETNodes;
		size_t temp = mainCET.closedItemsets.size();

		items.clear();
		mainFPTree.deleteItemset(items, t);
		mainCET.deletion(t, items, mainFPTree);

		////debug
		//cout << "***********************************" << endl;
		//mainCET.printMe(mainCET.CETRoot,0);
		//cout << endl;
		//cout << i << ":" << endl;
		//mainCET.printHash();
		//cout << endl;

		deletedNodes = previousNodes - TreeNode::numberOfCETNodes;

		float tempTime = static_cast<float>(clock() - t1) / CLOCKS_PER_SEC;
		totalTime += tempTime;
		avgTime = (avgTime + tempTime) / 2;
		outFile.width(3);
		outFile << i;
		outFile.width(6); outFile.precision(4);
		outFile << tempTime;
		outFile.width(8);
		outFile << mainCET.closedItemsets.size();
		outFile.width(10);
		outFile << TreeNode::numberOfCETNodes;
		outFile.width(8);
		outFile << numberOfExploreCall;
		outFile.width(8);
		outFile << addedNodes;
		outFile.width(8);
		outFile << deletedNodes;
		outFile.width(7);
		outFile << temp << endl;

		totalExplore += numberOfExploreCall;
		totalAddedNodes += addedNodes;
		totalDeletedNodes += deletedNodes;
		totalClosed = mainCET.closedItemsets.size();
		totalCETNodes = TreeNode::numberOfCETNodes;

		//if ( i+1 == 15 ) {
		//	cout << "checking closed itemsets..." << endl;
		//	mainCET.checkMe();
		//}
	}
	outFile << endl;
	outFile << "total time      : " << totalTime << "s" << endl;
	outFile << "initial FPTree time : " << initialFPTree_time << "s" << endl;
	outFile << "average time of all sliding windows : " << avgTime << "s" << endl;
	outFile << "closed itemset #: " << totalClosed << endl;
	outFile << "CET node       #: " << totalCETNodes << endl;
	outFile << "Explore call   #: " << totalExplore << endl;
	outFile << "Added node     #: " << totalAddedNodes << endl;
	outFile << "Deleted node   #: " << totalDeletedNodes << endl;

#ifdef _WIN32
	PROCESS_MEMORY_COUNTERS_EX info;
	GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&info, sizeof(info));
	outFile << "WorkingSet " << info.WorkingSetSize / 1024 << "K, PeakWorkingSet " << info.PeakWorkingSetSize / 1024 << "K, PrivateSet " << info.PrivateUsage / 1024 << "K" << endl;
#endif

	inFile.close();
	outFile.close();

	return 0;
}