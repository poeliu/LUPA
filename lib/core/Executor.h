// Copyright 2014 Shanghai Jiao Tong University
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Authors - Trusted Cloud Group (http://tcloud.sjtu.edu.cn)

#ifndef EXECUTOR_H_
#define EXECUTOR_H_

#include "llvm/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Instruction.h"
#include "llvm/Instructions.h"
#include "SymbolicState.h"
#include "ValueFlow.h"
#include "AliasAnalyzer.h"
#include "../statistic/GlobalStatistic.h"
#include "../util/CFG.h"
#include "../util/Log.h"
#include "../util/CDG.h"
#include "../util/Naming.h"
#include "Worklist.h"
#include <set>
#include <map>

using namespace llvm;
using namespace esp;

namespace esp{
  class Edge;
  class Work;
  class Solver;
  class AbstractState;
  class ExecutionState;
  class SymbolicState;
  class Worklist;

// Define analysis commands here
#define LOCK_STATISTIC 1
#define LOCK_PATTERN   0
#define LOCK_PAIR_UP    0
#define USE_ALIAS          1
#define ALIAS_ACCURACY MustAlias

/*
 * Judge whether it is a lock-related call
 * @Params
 * name: name of the caller function
 */
bool shouldBeAnalyzed(string name);

class InstWork: public Work{
public:
  /* instruction that is to be analyzed */
  Instruction *content;

  /*
   * Constructor
   * @Params
   * inst: llvm instruction that is to be analyzed
   */
  InstWork(Instruction *inst){
    this->content = inst;
  }

  /*
   * Destructor
   */
  ~InstWork();

  /*
   * Override function
   */
  virtual bool equalsTo(Work *other);

  /*
   * Override function
   */
  void printContent();
};

/*
 * Core intra-procedural analysis component
 */
class Executor:public Worklist{
private:
  string applicationName;                                                  //Application name

  set<Instruction*> worklist;                                            //List of instructions to be analyzed
  set<Lock*> locks;                                                          //Lock set
  Lock *currentLock;                                                          //Current lock being handled
  Function *currentFunction;                                             //Current function being handled

  map<Instruction*, pair<Edge*, Edge*>* > inEdges;   //Edges that flow into each instruction node
  map<Instruction*, pair<Edge*, Edge*>* > outEdges; //Edges that go out of each instruction node
  map<Value*, set<Function*>* > functionPtrs;            //Function pointers and potential pointed function

  Instruction *returnNode;                                                //Node that the function will return to
  set<Instruction*> returnSites;                                      //Return sites
  set<Instruction*> returnValues;                                   //Call to lock or unlock function

  map<Edge*, set<SymbolicState*>* > infos;              //Symbolic state set

  GlobalStatistic statistic;                                                 //Collect statistic data

  AliasAnalyzer aliasAnalyzer;                                         //Alias analyzer;

public:
  /*
   * Constructor
   */
  Executor(string name);

  /*
   * Destructor
   */
  ~Executor();

  /*
   * Initialize executor for intra-procedural analysis
   */
  void initExecutor();

  /*
   * Run intra-procedural analysis
   */
  void run(Module *module);

  /*
   * Analyze every instruction
   */
  void analyseInstruction(Instruction *inst);

  /*
   * Build use-define chains of each function for future use and find lock
   * variables
   */
  void buildUDChains(Module *module, Function *function);

  /*
   * Analyze function pointer before the main analysis.
   * Maybe it is no use in intra-procedural analysis
   */
  void analyseFunctionPtr(Module *module);

  /*
   * Set going-in edges for each instruction node
   */
  void setInEdge(Instruction *inst, int index);

  /*
   * Set going-out edges for each instruction node
   */
  void setOutEdge(Instruction *inst, bool selectedPath);

  /*
   * Return going-in edges of the given instruction node
   */
  Edge* getInEdge(Instruction *inst, int index);

  /*
   * Return going-out edges of the given instruction node
   */
  Edge* getOutEdge(Instruction *inst, bool selectedPath);

  /*
   * Grouping the given symbolic states
   */
  set<SymbolicState*>* grouping(set<SymbolicState*>* states);

  /*
   * Flow function of merge nodes
   */
  set<SymbolicState*>* flowMerge(Instruction *inst, map<Edge*, set<SymbolicState*>* >* infos, Lock *lock);

  /*
   * Flow function of branch nodes
   */
  set<SymbolicState*>* flowBranch(Instruction *inst, set<SymbolicState*>* infos, bool selectedPath, Lock *lock);

  /*
   * Flow function of none-branch and none-merge nodes
   */
  set<SymbolicState*>* flowOther(Instruction *inst, set<SymbolicState*>* infos, Lock *lock);

  /*
   * Find the symbolic state of the edge
   */
  set<SymbolicState*>* getStates(Edge* edge);

  /*
   * Add instruction to worklist for future analysis
   */
  void addToWorklist(Edge *e, std::set<SymbolicState *> *ss, std::map<Edge*, std::set<SymbolicState*> *> &infos);

  /*
   * Override function
   */
  void doEachWork(Work *work);

  /*
   * Override function
   */
  void initWorklist();

  /*
   * Build control dependency graph
   */
  CDG* buildCDG(Function *function);

  /*
   * Find lock pattern
   */
  void findLockPattern(Function *function);

};

}

#endif /* EXECUTOR_H_ */
