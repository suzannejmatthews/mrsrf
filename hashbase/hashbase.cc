
//*****************************************************************/
// This is HashBase, the tree collection core of MrsRF. It is based
// On HashRF, developed by Seung-Jin Sul
// Copyright (C) 2009 HashBase, MrsRF: Suzanne Matthews, Tiffani Williams
//              Department of Computer Science
//              Texas A&M University
//              [sjm,tlw]@cse.tamu.edu
//
// Copyright (C) 2006-2009 Seung-Jin Sul
//              Department of Computer Science
//              Texas A&M University
//              Contact: sulsj@cs.tamu.edu
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details (www.gnu.org).
//
//*****************************************************************/



#include <sys/time.h>
#include <sys/resource.h>
#include <cassert>
#include <valarray>
#include <fstream>
#include <iostream>

#include "label-map.hh"
#include "hashfunc.hh"
#include "hash.hh"
#include "./tclap/CmdLine.h"

// For newick parser
extern "C" {
#include <newick.h>
}

using namespace std;

#define LEFT               								0
#define RIGHT              								1
#define ROOT               								2
#define LABEL_WIDTH        								3

// Set a random number for m1 (= Initial size of hash table)
// m1 is the closest value to (t*n)+(t*n*HASHTABLE_FACTOR)
#define HASHTABLE_FACTOR    							0.2

// the c value of m2 > c*t*n in the paper
static unsigned int C						    			= 1000; 
static unsigned NUM_TREES                 = 0; // number of trees
static unsigned NUM_TREES_ORIG            = 0; //for random number generator goodness
static unsigned NUM_TAXA                	= 0; // number of taxa
static bool WEIGHTED                      = false; // unweighted
//static unsigned PRINT_OPTIONS							= 3; // matrix
static int32 NEWSEED                      = 0; //for user defined seed

#define __HALF_MAX_SIGNED(type) ((type)1 << (sizeof(type)*8-2))
#define __MAX_SIGNED(type) (__HALF_MAX_SIGNED(type) - 1 + __HALF_MAX_SIGNED(type))
#define __MIN_SIGNED(type) (-1 - __MAX_SIGNED(type))

#define __MIN(type) ((type)-1 < 1?__MIN_SIGNED(type):(type)0)
#define __MAX(type) ((type)~__MIN(type))

#define assign(dest,src) ({ \
  typeof(src) __x=(src); \
  typeof(dest) __y=__x; \
  (__x==__y && ((__x<1) == (__y<1)) ? (void)((dest)=__y),0:1); \
})

#define add_of(c,a,b) ({ \
  typeof(a) __a=a; \
  typeof(b) __b=b; \
  (__b)<1 ? \
    ((__MIN(typeof(c))-(__b)<=(__a)) ? assign(c,__a+__b):1) : \
    ((__MAX(typeof(c))-(__b)>=(__a)) ? assign(c,__a+__b):1); \
})

void
GetTaxaLabels(
  NEWICKNODE *node,
  LabelMap &lm)
{
  if (node->Nchildren == 0) {
    string temp(node->label);
    lm.push(temp);
  }
  else
    for (int i=0;i<node->Nchildren;++i)
      GetTaxaLabels(node->child[i], lm);
}

void
dfs_compute_hash(
  NEWICKNODE* startNode,
  LabelMap &lm,
  HashRFMap &vvec_hashrf,
  unsigned treeIdx,
  unsigned &numBitstr,
  unsigned long long m1,
  unsigned long long m2)
{
  // If the node is leaf node, just set the place of the taxon name in the bit string to '1'
  // and push the bit string into stack
  if (startNode->Nchildren == 0) { // leaf node
    string temp(startNode->label);
    unsigned int idx = lm[temp];

    // Implicit BPs /////////////////////////
    // Set the hash values for each leaf node.
    startNode->hv1 = vvec_hashrf._HF.getA1(idx);
    startNode->hv2 = vvec_hashrf._HF.getA2(idx);
  }
  else {
    for (int i=0; i<startNode->Nchildren; ++i) {
      dfs_compute_hash(startNode->child[i], lm, vvec_hashrf, treeIdx, numBitstr, m1, m2);
    }

    // For weighted RF
    float dist = 0.0;
    if (WEIGHTED)
      dist = startNode->weight;
    else
      dist = 1;

    ++numBitstr;

    // Implicit BPs ////////////
    // After an internal node is found, compute the hv1 and hv2
    unsigned long long temphv1=0;
    unsigned long long temphv2=0;
   
    for (int i=0; i<startNode->Nchildren; ++i) {    	
    	unsigned long long t1 = temphv1;
    	unsigned long long t2 = temphv2;
    	unsigned long long h1 = startNode->child[i]->hv1;
    	unsigned long long h2 = startNode->child[i]->hv2;

    	if ( add_of(temphv1, t1, h1) ) {
    		cout << "ERROR: ullong add overflow!!!\n"; 
    		cout << "t1=" << t1 << " h1=" << h1 << " t1+h1=" << t1+h1 << endl;
    		exit(0);
    	}
    	if ( add_of(temphv2, t2, h2) ) {
    		cout << "ERROR: ullong add overflow!!!\n"; 
    		cout << "t2=" << t2 << " h2=" << h2 << " t2+h2=" << t2+h2 << endl;
    		exit(0);
    	}
	  }
		
		// Check overflow
		unsigned long long temp1 = temphv1 % m1;
		unsigned long long temp2 = temphv2 % m2;
    	startNode->hv1 = temp1;
    	startNode->hv2 = temp2;

    // Store bitstrings in hash table
    if (numBitstr < NUM_TAXA-2) {
      vvec_hashrf.hashing_bs_without_type2_nbits(treeIdx, NUM_TAXA, startNode->hv1, startNode->hv2, dist, WEIGHTED);   // without TYPE-III using n-bits (Hash-RF)
    }
  }
}

int main(int argc, char** argv)
{
  string outfilename;
  string infilename;
  string infilename2;
  bool bUbid = false; // for counting the number of unique bipartitions
  int OFFSET = 0;
  // TCLAP
  try {

    // Define the command line object.
    string 	helpMsg  = "HashBase [input file] [number of trees] [offset]\n";

    helpMsg += "Input file: \n";
    helpMsg += "   The current version of HashBase only supports the Newick format.\n";

    helpMsg += "Example of Newick tree: \n";
    helpMsg += "   (('Chimp':0.052625,'Human':0.042375):0.007875,'Gorilla':0.060125,\n";
    helpMsg += "   ('Gibbon':0.124833,'Orangutan':0.0971667):0.038875);\n";
    helpMsg += "   ('Chimp':0.052625,('Human':0.042375,'Gorilla':0.060125):0.007875,\n";
    helpMsg += "   ('Gibbon':0.124833,'Orangutan':0.0971667):0.038875);\n";
    helpMsg += "Specify c value: \n";   
    helpMsg += "   -c <rate>, specify c value (default: 1000) \n";    			
    helpMsg += "Examples: \n";
    helpMsg += "  hash_base foo.tre 1000 0\n";
    helpMsg += "  hash_base foo.tre 1000 0 -s 17\n";
    helpMsg += "  hash_base foo-1.tre 500 0 -s 17\n";
    helpMsg += "  hash_base foo-2.tre 500 500 -s 17\n";
    
    TCLAP::CmdLine cmd(helpMsg, ' ', "6.0.0");

    TCLAP::UnlabeledValueArg<string>  fnameArg( "name", "file name", true, "intree", "Input tree file name"  );
    cmd.add( fnameArg );

    TCLAP::UnlabeledValueArg<string>  fname2Arg( "name2", "seed file name", true, "intree2", "Input seed tree file name"  );
    cmd.add( fname2Arg );

    TCLAP::UnlabeledValueArg<int>  numtreeArg( "numtree", "number of trees", true, 2, "Number of trees"  );
    cmd.add( numtreeArg );

    TCLAP::UnlabeledValueArg<int>  numtreeOrigArg( "numtreeOrig", "number of trees in original file", true, 2, "Number of trees in original file"  );
    cmd.add( numtreeOrigArg );

    TCLAP::UnlabeledValueArg<int>  offsetArg( "offset", "offset for correction", true, 2, "Offset"  );
    cmd.add( offsetArg );

    TCLAP::ValueArg<unsigned int> cArg("c", "cvalue", "c value", false, 1000, "c value");
    cmd.add( cArg );
    
    TCLAP::SwitchArg ubidSwitch("u", "uBID", "unique BID count", false);
    cmd.add( ubidSwitch );
   
    TCLAP::ValueArg<int> seedArg("s", "seedvalue", "user specified seed value", false, 1000, "user specified seed value");
    cmd.add( seedArg );

    cmd.parse( argc, argv );

    NUM_TREES = numtreeArg.getValue();
    OFFSET = offsetArg.getValue();
    NUM_TREES_ORIG = numtreeOrigArg.getValue();

    if (NUM_TREES == 0) {
      string strFileLine;
      unsigned long ulLineCount;
      ulLineCount = 0;

      ifstream infile(argv[1]);
      if (infile) {
        while (getline(infile, strFileLine)) {
          ulLineCount++;
        }
      }
      cout << "*** Number of trees in the input file: " << ulLineCount << endl;
      NUM_TREES = ulLineCount;

      infile.close();
    }

    if (NUM_TREES < 2) {cerr << "Fatal error: at least two trees expected.\n"; exit(2);}

    if (cArg.getValue())
      C = cArg.getValue();
    
    if (seedArg.getValue()) { 
      NEWSEED = seedArg.getValue();
    }
    if (ubidSwitch.getValue())
      bUbid = ubidSwitch.getValue();  
 
    //outfilename = outfileArg.getValue();

  } catch (TCLAP::ArgException &e) { // catch any exceptions
    cerr << "error: " << e.error() << " for arg " << e.argId() << endl;
  }


  /*****************************************************/
//  cout << "*** Reading a tree file and parsing the tree for taxon label collection ***\n";
  /*****************************************************/

  NEWICKTREE *newickTree;
  int err;
  FILE *fp;
  fp = fopen(argv[2], "r"); //all labels MUST be populated by the same file
  if (!fp) { cout << "Fatal error: file open error\n"; exit(0); }

  newickTree = loadnewicktree2(fp, &err);
  if (!newickTree) {
    switch (err) {
    case -1:
      printf("Out of memory\n");
      break;
    case -2:
      printf("parse error\n");
      break;
    case -3:
      printf("Can't load file\n");
      break;
    default:
      printf("Error %d\n", err);
    }
  }

  /*****************************************************/
  //cout << "\n*** Collecting the taxon labels ***\n";
  /*****************************************************/
  LabelMap lm;

  try	{
    GetTaxaLabels(newickTree->root, lm);
  }
  catch (LabelMap::AlreadyPushedEx ex) { 
    cerr << "Fatal error: The label '" << ex.label << "' appeard twice in " << endl;
    exit(2);
  }
  NUM_TAXA = lm.size();
 
  killnewicktree(newickTree);
  fclose(fp);


  /*****************************************************/
  //cout << "\n*** Reading tree file and collecting bipartitions ***\n";
  /*****************************************************/
  HashRFMap vvec_hashrf; // Class HashRFMap

  ////////////////////////////
  // Init hash function class
  ////////////////////////////
  unsigned long long M1=0;
  unsigned long long M2=0;

  //generate random numbers:
  //when generating the random numbers, the original tree file's 
  //size must be used to populate correct m1 and m2 values
  if (NEWSEED != 1000) { 
    vvec_hashrf.uhashfunc_init(NUM_TREES_ORIG, NUM_TAXA, C, NEWSEED);
  }
  else { 
    vvec_hashrf.uhashfunc_init(NUM_TREES_ORIG, NUM_TAXA, C);
  }
  M1 = vvec_hashrf._HF.getM1();
  M2 = vvec_hashrf._HF.getM2();
  vvec_hashrf._hashtab2.resize(M1);

  //now we actually begin reading in the trees (use the tree file we actually care about!)
  fp = fopen(argv[1], "r"); 
  if (!fp) {cout << "Fatal error: file open error\n";  exit(0);}
  for (unsigned int treeIdx=0; treeIdx<NUM_TREES; ++treeIdx) {

    newickTree = loadnewicktree2(fp, &err);
    if (!newickTree) {
      switch (err) {
      case -1:
        printf("Out of memory\n");
        break;
      case -2:
        printf("parse error\n");
        break;
      case -3:
        printf("Can't load file\n");
        break;
      default:
        printf("Error %d\n", err);
      }
    }
    else {
      unsigned int numBitstr=0;

      dfs_compute_hash(newickTree->root, lm, vvec_hashrf, treeIdx, numBitstr, M1, M2);

      killnewicktree(newickTree);
    }
  }

  fclose(fp);


  /*****************************************************/
  // print out hashtable elements
  /*****************************************************/
       
  unsigned long uBID = 0;
	
  if (!WEIGHTED) {    // unweighted
    for (unsigned int hti=0; hti<vvec_hashrf._hashtab2.size(); ++hti) {
      unsigned int sizeVec = vvec_hashrf._hashtab2[hti].size();
      
      if (sizeVec) {
      	uBID += sizeVec;
	
      	if (!bUbid) {
	  for (unsigned int i=0; i<sizeVec; ++i) {
	    
	    unsigned int sizeTreeIdx = vvec_hashrf._hashtab2[hti][i]._vec_treeidx.size();
	    cout << "LINE:" << hti << "." << vvec_hashrf._hashtab2[hti][i]._hv2 << ":";
	    
	    for (unsigned int j=0; j<sizeTreeIdx; ++j) {
	      int temp  = vvec_hashrf._hashtab2[hti][i]._vec_treeidx[j] + OFFSET;
	      cout << temp << " ";
	      
	    }
	    cout << endl;
	    
	  }
	} //if 
      }
    }
  } 

 return 0;
}
