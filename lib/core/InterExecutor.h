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

#ifndef INTEREXECUTOR_H_
#define INTEREXECUTOR_H_

#include "llvm/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Instruction.h"
#include "llvm/Instructions.h"
#include "AliasAnalyzer.h"
#include "../statistic/ModuleStatistic.h"
#include "../util/CFG.h"
#include "../util/Log.h"
#include "../util/CDG.h"
#include "Analyzer.h"
#include <set>
#include <map>

using namespace llvm;

namespace esp {

// Define analysis commands here
#define INTER_LOCK_STATISTIC 1
#define INTER_LOCK_PATTERN   1
#define INTER_LOCK_PAIR_UP    0
#define INTER_USE_ALIAS          1
#define INTER_ALIAS_ACCURACY MayAlias


/*
 * Intra-procedural analysis
 */
class IntraExecutor{
private:
  Value *currentLock;                                                          //Current lock being handled
  FunctionStatistic *fs;                                                        //Function Statistic
  Module *module;                                                             //Analyzed module
  Function *function;                                                          //Analyzed function
  AliasAnalyzer *aa;                                                           //Alias analyzer;
  ModuleStatistic* ms;                                                       //Module statistic
  set<Value*> locks;                                                         //Lock set
  Instruction *returnNode;                                                 //Node that the function will return to
  set<Instruction*> returnSites;                                       //Return sites
  set<Instruction*> returnValues;                                    //Call to lock or unlock function
  int returnValuesSize;                                                      //Size of lock/unlock call

  void initExecutor();

  /*
   * Build use-define chains of each function for future use and find lock
   * variables
   */
  void buildUDChains();

  /*
   * Build control dependency graph
   */
  CDG* buildCDG();

  /*
   * Find lock pattern
   */
  void findLockPattern();

  /*
   * Judge whether it is not a call using function pointer
   * or using instruction operand
   */
  bool shouldBeAnalyzed(CallInst *callInst);

  /*
   * Judge whether it is a lock-related call
   */
  bool shouldBeAnalyzed(Function *function);

  /*
   * Judge whether it is a lock-related call
   */
  bool shouldBeAnalyzed2(Function *function);

public:
  IntraExecutor(Module *module, Function* function, ModuleStatistic* ms, AliasAnalyzer *aa){
    this->module     =    module;
    this->function    =    function;
    this->ms            =    ms;
    this->aa             =    aa;
  }

  void run();

  ~IntraExecutor(){/*Do nothing here*/};

};

/*
 * Core inter-procedural analysis component
 */
class InterExecutor:public Analyzer{
private:
  string applicationName;                                                 //Application name

  Function *currentFunction;                                             //Current function being handled

  ModuleStatistic *statistic;                                              //Collect statistic data

  AliasAnalyzer *aliasAnalyzer;                                        //Alias analyzer;

public:
  /*
   * Constructor
   */
  InterExecutor(string name);

  /*
   * Destructor
   */
  ~InterExecutor();

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
  void analyseInstruction(Instruction *inst){}

  /*
   * Analyze function pointer before the main analysis.
   * Maybe it is no use in intra-procedural analysis
   */
  void analyseFunctionPtr(Module *module){}

  /*
   * Close executor
   */
  void closeAnalysis(){}

};


} /* namespace esp */
#endif /* INTEREXECUTOR_H_ */
