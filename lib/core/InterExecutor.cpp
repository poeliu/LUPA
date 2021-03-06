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

#include "InterExecutor.h"
#include "../util/MySlotTracker.h"
#include <iostream>
#include <assert.h>

using namespace std;
using namespace llvm;

namespace esp {

// Define lock function
const int targetNumber = 2;
string targets[] =
    { "pthread_mutex_lock", "pthread_mutex_unlock", "pthread_mutex_try_lock",
        "pthread_mutex_try_unlock", "rw_lock", "rw_unlock" };

// Analyzing stack
list<Function *> workList;

bool IntraExecutor::shouldBeAnalyzed(CallInst *callInst) {
  Value *calledValue = callInst->getCalledValue();
  if (isa<Function>(calledValue))
    return true;
  // Function pointer or other instruction
  return false;
}

bool IntraExecutor::shouldBeAnalyzed(Function *function) {
  string name = function->getNameStr();

  for (int i = 0; i < targetNumber; i++) {
    if (targets[i].compare(name) == 0)
      return true;
  }

  return false;
}

bool IntraExecutor::shouldBeAnalyzed2(Function *function) {
  if (function->isDeclaration())
    return false;

  if (ms->functionStatistics.find(function) != ms->functionStatistics.end()) {
    if (ms->functionStatistics[function] != NULL
        && ms->functionStatistics[function]->lockNumber != 0)
      return true;
    else
      return false;
  } else {
    // If the function has not been analyzed before,
    // analyze this function recursively except that this
    // function is being analyzing
    if (this->function == function) {
      cout << "recursive function: " << function->getNameStr() << endl;
      return false;
    } else {
      for (list<Function*>::iterator it = workList.begin();
          it != workList.end(); it++) {
        if ((*it) == function)
          return false;
      }
    }

    workList.push_back(function);
    IntraExecutor intraExecutor(module, function, ms, aa);
    intraExecutor.run();
    workList.pop_back();

    if (ms->functionStatistics[function] != NULL
        && ms->functionStatistics[function]->lockNumber != 0)
      return true;
    else
      return false;
  }

  return false;
}

void IntraExecutor::initExecutor() {
  locks.clear();
  returnNode = NULL;
  returnSites.clear();
  returnValues.clear();
  returnValuesSize = 0;

  returnNode = ReturnInst::Create(function->getContext());

}

void IntraExecutor::run() {

  if (ms->functionStatistics.find(function) != ms->functionStatistics.end())
    return; // This function has been analyzed before

  initExecutor();
  ms->functionNumber++;

  printDebugMsg("========Enter function " + function->getNameStr());

  fs = new FunctionStatistic(function->getNameStr());
  buildUDChains();

  if (returnValuesSize == 0) { //no lock or unlock inside the function
    ms->functionStatistics[function] = NULL;
    printDebugMsg("========Exit function " + function->getNameStr());
    //ms->functionStatistics[function] = NULL;
    delete fs;
    return;
  } else {
    ms->lockFunctionNumber++;
    ms->functionStatistics[function] = fs;
  }

  // Get lock type information
  for (set<Value*>::iterator lit = locks.begin(); lit != locks.end(); lit++) {
    string typeName = getValueName(fs->locks[(*lit)]->type);
    if (typeName.substr(0, 1) != "%")
      fs->globalLockNumber++;
    else
      fs->localLockNumber++;
  }

  for (set<Value*>::iterator lit = locks.begin(); lit != locks.end(); lit++) {
    if ((*lit) != NULL) {
      currentLock = (*lit);

      // Find lock pattern
      if (INTER_LOCK_PATTERN) {
        this->findLockPattern();
      }

      // Check lock deep
      uint maxDeep = 0;
      bool isInter = false;
      for (set<Instruction*>::iterator rit = returnValues.begin();
          rit != returnValues.end(); rit++) {
        CallInst *callInst = (CallInst*) (*rit);
        string name = callInst->getCalledFunction()->getNameStr();

        if (callInst->getCalledFunction() == function)
          fs->recursiveLock = true;

        if (name != "pthread_mutex_lock" && name != "pthread_mutex_unlock") {
          FunctionStatistic *callS =
              ms->functionStatistics[callInst->getCalledFunction()];
          for (map<Value*, LockData*>::iterator vit = callS->locks.begin();
              vit != callS->locks.end(); vit++) {
            if (aa->isAlias(currentLock, (*vit).first, function,
                callInst->getCalledFunction()) >= INTER_ALIAS_ACCURACY) {
              isInter = true;
              if (maxDeep < (*vit).second->callDeep) {
                maxDeep = (*vit).second->callDeep;
              }
            }
          }
        }
        if (isInter)
          fs->locks[currentLock]->callDeep = 1 + maxDeep;
      }

      // Check lock pair up
      if (INTER_LOCK_PAIR_UP) {
        // LUPA does not care about this
      }

    } else {
      printWarningMsg("NULL lock variable!");
    }
    //break; //Debug single lock
  }

  printDebugMsg("========Exit function " + function->getNameStr());

}

void IntraExecutor::buildUDChains() {
  //Build ud-chains between globals and their user nodes in the given function
  for (Module::global_iterator globals = module->global_begin();
      globals != module->global_end(); globals++) {
    Value *globalv = &*globals;
    for (Value::use_iterator uit = globalv->use_begin();
        uit != globalv->use_end(); uit++) {
      if (isa<Instruction>(*uit))
        if (((Instruction*) *uit)->getParent() != NULL)
          if (((Instruction*) *uit)->getParent()->getParent() == function) {
            parents[*uit] = globalv;
          }
    }
  }

  //Build ud-chains between arguments and their user nodes
  for (Function::arg_iterator args = function->arg_begin();
      args != function->arg_end(); args++) {
    Value *argv = &*args;
    for (Value::use_iterator uit = argv->use_begin(); uit != argv->use_end();
        uit++) {
      parents[*uit] = argv;
      arguments.insert(argv);
    }
  }

  //Build ud-chains between every instruction and its user nodes
  for (inst_iterator it = inst_begin(function); it != inst_end(function);
      it++) {
    for (Value::use_iterator uit = (*it).use_begin(); uit != (*it).use_end();
        uit++) {
      if (parents[*uit] == NULL) {
        parents[*uit] = &*it;
        //printDebugMsg("Build ud between: ");
      }
    }
    if (isa<CallInst>(&*it)) {
      //printInstruction(&*it);
      CallInst * callinst = dyn_cast<CallInst>(&*it);
      Function *calledfunc = callinst->getCalledFunction();

      //If a call instruction to lock or unlock function then add it to returnValues
      if (calledfunc != NULL && shouldBeAnalyzed(callinst)
          && shouldBeAnalyzed(calledfunc)) {

        returnValues.insert(&*it);
        returnValuesSize++;

      } else if (calledfunc != NULL && shouldBeAnalyzed(callinst)
          && shouldBeAnalyzed2(calledfunc)) {
        returnValues.insert(&*it);
      }
    }
  }

  //Need not to analysis this function because it is not relative to lock operation
  if (returnValuesSize == 0)
    return;

  //Look for lock variables
  printDebugMsg("Lock list:");
  map<Value*, int> lockNumbers;
  map<Value*, int> unlockNumbers;
  for (std::set<Instruction*>::iterator it = returnValues.begin();
      it != returnValues.end(); it++) {
    CallInst *callinst = (CallInst*) (*it);

    if (shouldBeAnalyzed(callinst)
        && (!shouldBeAnalyzed(callinst->getCalledFunction())))
      continue;

    //Eliminate alias lock variables
    //The first operand is the name of the function
    //printDebugMsg("insert lock: "+callinst->getOperand(0)->getNameStr());
    bool shouldInsert = true;
    for (set<Value*>::iterator itt = locks.begin(); itt != locks.end(); itt++) {
      if (aa->isAlias((*itt), callinst->getOperand(0), function,
          function) >= INTER_ALIAS_ACCURACY) {
        shouldInsert = false;
        if (callinst->getNameStr() == "pthread_mutex_lock") {
          lockNumbers[(*itt)]++;
        } else if (callinst->getNameStr() == "pthread_mutex_unlock")
          unlockNumbers[(*itt)]++;
        break;
      }
    }

    if (shouldInsert) {
      fs->locks[callinst->getOperand(0)] = new LockData();
      fs->locks[callinst->getOperand(0)]->callDeep = 0;
      fs->lockNumber++;
      if (callinst->getNameStr() == "pthread_mutex_lock") {
        lockNumbers[callinst->getOperand(0)] = 1;
        unlockNumbers[callinst->getOperand(0)] = 0;
      } else if (callinst->getNameStr() == "pthread_mutex_unlock") {
        unlockNumbers[callinst->getOperand(0)] = 1;
        lockNumbers[callinst->getOperand(0)] = 0;
      }
      locks.insert(callinst->getOperand(0));

      // Get look type
      Value *parent = (*it);
      while (true) {
        if (parents[parent] == NULL) {
          fs->locks[callinst->getOperand(0)]->type = parent;
          break;
        }
        parent = parents[parent];
      }
    }
  }

  for (set<Value*>::iterator it = locks.begin(); it != locks.end(); it++) {
    if (lockNumbers[(*it)] != 0 && unlockNumbers[(*it)] == 0)
      fs->locks[(*it)]->isLockWrapper = true;
    else if (lockNumbers[(*it)] == 0 && unlockNumbers[(*it)] != 0)
      fs->locks[(*it)]->isUnlockWrapper = true;
  }

}

CDG* IntraExecutor::buildCDG() {
  //cout<<"build CDG"<<endl;

  CDG *cdg = new CDG();
  cdg->buildCDG(function);
  //cdg->printCDG();

  //cout<<"build CDG end"<<endl;

  return cdg;
}

void IntraExecutor::findLockPattern() {
  CDG *cdg = buildCDG();
  assert(cdg != NULL);
  //cdg->printCDG();

  // Iteration over CFG from path to path
  stack<BasicBlock*> cfg;
  stack<CallInst*> lockInsts;
  map<CallInst*, bool> lockStates;
  map<BasicBlock*, bool> accessFlags;
  set<pair<Value*, Value*> > lockPairs;

  BasicBlock *entry = &function->front();
  cfg.push(entry);
  accessFlags[entry] = false;

  while (cfg.size() != 0) {
    BasicBlock *top = cfg.top();
    CallInst *lastLockCall;
    if (lockInsts.size() != 0)
      lastLockCall = lockInsts.top();
    else
      lastLockCall = NULL;

    if (!accessFlags[top]) {
      accessFlags[top] = true;
    } else {
      accessFlags.erase(accessFlags.find(top));
      cfg.pop();
      if (lockInsts.size() != 0 && lockInsts.top()->getParent() == top) {
        lockStates.erase(lockStates.find(lockInsts.top()));
        lockInsts.pop();
      }
      continue;
    }

    for (BasicBlock::iterator it = top->begin(); it != top->end(); it++) {
      Instruction *inst = &(*it);

      // Lock pattern detection is done here
      if (isa<CallInst>(inst)) {
        CallInst * callInst = dyn_cast<CallInst>(&*it);
        Function *calledFunc = callInst->getCalledFunction();

        if (calledFunc != NULL && shouldBeAnalyzed(callInst)
            && (shouldBeAnalyzed2(calledFunc) || shouldBeAnalyzed(calledFunc))) {
          if (callInst->getCalledFunction()->getNameStr()
              == "pthread_mutex_lock") {
            if (aa->isAlias(currentLock, callInst->getOperand(0), function,
                function) >= INTER_ALIAS_ACCURACY) {
              lastLockCall = callInst;
              lockInsts.push(callInst);
              lockStates[callInst] = false;
            }
          } else if (shouldBeAnalyzed(callInst)
              && callInst->getCalledFunction()->getNameStr()
                  != "pthread_mutex_unlock") {
            if (!ms->functionStatistics[callInst->getCalledFunction()]->isUnlockWrapper()) {
              FunctionStatistic *callS =
                  ms->functionStatistics[callInst->getCalledFunction()];
              Value *lockVariable;
              for (map<Value*, LockData*>::iterator iul = callS->locks.begin();
                  iul != callS->locks.end(); iul++) {
                lockVariable = (*iul).first;
                // Consider lock wrapper function only
                if (aa->isAlias(currentLock, lockVariable, function,
                    callInst->getCalledFunction()) >= INTER_ALIAS_ACCURACY
                    && callS->locks[lockVariable]->isLockWrapper) {
                  lastLockCall = callInst;
                  lockInsts.push(callInst);
                  lockStates[callInst] = false;
                }
              }
            }
          }
        }
        //llvm::errs()<<(*callInst)<<"\n";
        if (shouldBeAnalyzed(callInst)
            && (shouldBeAnalyzed2(calledFunc) || shouldBeAnalyzed(calledFunc))
            && callInst->getCalledFunction()->getNameStr()
                == "pthread_mutex_unlock") {

          if (lastLockCall == NULL || lockStates[lastLockCall])
            continue;

          bool detected = false;
          for (set<pair<Value*, Value*> >::iterator pit = lockPairs.begin();
              pit != lockPairs.end(); pit++) {
            if ((*pit).first == lastLockCall && (*pit).second == callInst) {
              // Detected pattern
              detected = true;
              break;
            }
          }
          if (detected)
            continue;
          else
            lockPairs.insert(pair<Value*, Value*>(lastLockCall, callInst));

          Value *unlockVariable = callInst->getOperand(0);

          // Pattern should only exist within a pair of lock and unlock instruction
          // with the same lock variable
          if (aa->isAlias(currentLock, unlockVariable, function,
              function) >= INTER_ALIAS_ACCURACY) {
            bool isOtherPattern = true;

            // Compute the control dependency node of the two instructions
            BasicBlock *lockDependency =
                cdg->dependences[lastLockCall->getParent()];
            BasicBlock *unlockDependency =
                cdg->dependences[callInst->getParent()];

            if (lastLockCall->getParent() == callInst->getParent()) { // lock..unlock pattern
              fs->locks[currentLock]->directLockNumber++;
              lockStates[lastLockCall] = true;
              isOtherPattern = false;
            } else if (lockDependency == unlockDependency) { // lock..unlock pattern
              fs->locks[currentLock]->directLockNumber++;
              lockStates[lastLockCall] = true;
              isOtherPattern = false;
            } else if (lockDependency == NULL) {
              bool isTestPattern = false;
              if (lastLockCall->getNumUses() != 0) { // if(lock) pattern
                isTestPattern = true;
                fs->locks[currentLock]->testLockNumber++;
                lockStates[lastLockCall] = true;
              }

              if (!isTestPattern) { // lock..unlock..pattern
                fs->locks[currentLock]->directLockNumber++;
                lockStates[lastLockCall] = true;
              }
            } else if (unlockDependency == NULL) {
              isOtherPattern = true;
            } else if (lockDependency != unlockDependency) {
              Instruction *lockParent = lockDependency->getTerminator();
              Instruction *unlockParent = unlockDependency->getTerminator();
              while (true) {
                if (parents.find(lockParent) == parents.end())
                  break;
                lockParent = (Instruction*) parents[lockParent];
              }
              while (true) {
                if (parents.find(unlockParent) == parents.end())
                  break;
                unlockParent = (Instruction*) parents[unlockParent];
              }
              if (lockParent == unlockParent) { // if..lock..if..unlock pattern
                fs->locks[currentLock]->ifLockNumber++;
                lockStates[lastLockCall] = true;
              } else {
                bool isTestPattern = false;
                if (lastLockCall->getNumUses() != 0) { // if(lock) pattern
                  isTestPattern = true;
                  fs->locks[currentLock]->testLockNumber++;
                  lockStates[lastLockCall] = true;
                  //break;
                }

                if (!isTestPattern) { // lock..unlock..pattern
                  fs->locks[currentLock]->directLockNumber++;
                  lockStates[lastLockCall] = true;
                }
              }
              isOtherPattern = false;
            }

            if (isOtherPattern) {
              //cout<<"haha"<<endl;
              fs->locks[currentLock]->otherNumber++;
              lockStates[lastLockCall] = true;
            }

            fs->locks[currentLock]->lockUsage++;

          }

        }
        // Find unlock instruction inter-procedurally
        else if (shouldBeAnalyzed(callInst)
            && (shouldBeAnalyzed2(calledFunc) || shouldBeAnalyzed(calledFunc))
            && callInst->getCalledFunction()->getNameStr()
                != "pthread_mutex_lock") {
          //cout<<"function name: "<<unlockInst->getCalledFunction()->getNameStr()<<endl;
          //cout<<"stopped at: "<<callInst->getCalledFunction()->getNameStr()<<endl;
          if (!ms->functionStatistics[callInst->getCalledFunction()]->isLockWrapper()) {
            if (lastLockCall == NULL || lockStates[lastLockCall])
              continue;

            bool detected = false;
            for (set<pair<Value*, Value*> >::iterator pit = lockPairs.begin();
                pit != lockPairs.end(); pit++) {
              if ((*pit).first == lastLockCall && (*pit).second == callInst) {
                // Detected pattern
                detected = true;
                break;
              }
            }
            if (detected)
              continue;
            else
              lockPairs.insert(pair<Value*, Value*>(lastLockCall, callInst));

            FunctionStatistic *callS =
                ms->functionStatistics[callInst->getCalledFunction()];
            Value *unlockVariable;
            for (map<Value*, LockData*>::iterator iul = callS->locks.begin();
                iul != callS->locks.end(); iul++) {
              unlockVariable = (*iul).first;
              // Pattern should only exist within a pair of lock and unlock instruction
              // with the same lock variable
              // Here consider unlock wrapper function only
              if (aa->isAlias(currentLock, unlockVariable, function,
                  callInst->getCalledFunction()) >= INTER_ALIAS_ACCURACY
                  && callS->locks[unlockVariable]->isUnlockWrapper) {
                bool isOtherPattern = true;

                // Compute the control dependency node of the two instructions
                BasicBlock *lockDependency =
                    cdg->dependences[lastLockCall->getParent()];
                BasicBlock *unlockDependency =
                    cdg->dependences[callInst->getParent()];

                if (lastLockCall->getParent() == callInst->getParent()) { // lock..unlock pattern
                  fs->locks[currentLock]->directLockNumber++;
                  lockStates[lastLockCall] = true;
                  isOtherPattern = false;
                } else if (lockDependency == unlockDependency) { // lock..unlock pattern
                  fs->locks[currentLock]->directLockNumber++;
                  lockStates[lastLockCall] = true;
                  isOtherPattern = false;
                } else if (lockDependency == NULL) {
                  bool isTestPattern = false;
                  if (lastLockCall->getNumUses() != 0) { // if(lock) pattern
                    isTestPattern = true;
                    fs->locks[currentLock]->testLockNumber++;
                    lockStates[lastLockCall] = true;
                  }

                  if (!isTestPattern) { // lock..unlock..pattern
                    fs->locks[currentLock]->directLockNumber++;
                    lockStates[lastLockCall] = true;
                  }
                } else if (unlockDependency == NULL) {
                  isOtherPattern = true;
                } else if (lockDependency != unlockDependency) {
                  Instruction *lockParent = lastLockCall;
                  Instruction *unlockParent = callInst;
                  while (true) {
                    if (parents.find(lockParent) == parents.end())
                      break;
                    lockParent = (Instruction*) parents[lockParent];
                  }
                  while (true) {
                    if (parents.find(unlockParent) == parents.end())
                      break;
                    unlockParent = (Instruction*) parents[unlockParent];
                  }
                  if (lockParent == unlockParent) { // if..lock..if..unlock pattern
                    fs->locks[currentLock]->ifLockNumber++;
                    lockStates[lastLockCall] = true;
                  } else {
                    bool isTestPattern = false;
                    if (lastLockCall->getNumUses() != 0) {
                      isTestPattern = true;
                      fs->locks[currentLock]->testLockNumber++;
                      lockStates[lastLockCall] = true;
                      //break;
                    }
                    if (!isTestPattern) // lock..unlock..pattern
                      fs->locks[currentLock]->directLockNumber++;
                    lockStates[lastLockCall] = true;
                  }
                  isOtherPattern = false;
                }

                if (isOtherPattern) {
                  //cout<<"haha"<<endl;
                  fs->locks[currentLock]->otherNumber++;
                  lockStates[lastLockCall] = true;
                }

                fs->locks[currentLock]->lockUsage++;

              }
            }
          }
        }

      }
    }

    TerminatorInst *tinst = top->getTerminator();
    for (uint i = 0; i < tinst->getNumSuccessors(); i++) {
      BasicBlock *successor = tinst->getSuccessor(i);
      if (accessFlags.find(successor) == accessFlags.end()) {
        cfg.push(successor);
        accessFlags[successor] = false;
      }
    }
  }

  /*
   // Get lock call instructions
   set<CallInst*> lockCalls;
   for(set<Instruction*>::iterator it = returnValues.begin();
   it != returnValues.end(); it++){
   CallInst *callInst = dyn_cast<CallInst>(*it);

   // Get lock cal instruction intra-procedurally
   if(callInst->getCalledFunction()->getNameStr() == "pthread_mutex_lock"){
   if(aa->isAlias(currentLock, callInst->getOperand(1), function, function)
   >= INTER_ALIAS_ACCURACY)
   lockCalls.insert(callInst);
   }
   // Get lock call instruction inter-procedurally
   else if(callInst->getCalledFunction()->getNameStr() != "pthread_mutex_unlock"){
   if(!ms->functionStatistics[callInst->getCalledFunction()]->isUnlockWrapper()){
   FunctionStatistic *callS = ms->functionStatistics[callInst->getCalledFunction()];
   Value *lockVariable;
   for(map<Value*, LockData*>::iterator iul = callS->locks.begin();
   iul != callS->locks.end(); iul++){
   lockVariable = (*iul).first;
   // Consider lock wrapper function only
   if(aa->isAlias(currentLock, lockVariable, function, callInst->getCalledFunction())
   >= INTER_ALIAS_ACCURACY && callS->locks[lockVariable]->isLockWrapper){
   lockCalls.insert(callInst);
   }
   }
   }
   }
   }

   // Begin to find lock pattern
   for(set<CallInst*>::iterator it = lockCalls.begin();
   it != lockCalls.end(); it++){
   CallInst *lockInst = *it;
   Value *lockVariable = currentLock;

   for(set<Instruction*>::iterator itt = returnValues.begin();
   itt != returnValues.end(); itt++){

   CallInst *unlockInst = dyn_cast<CallInst>(*itt);

   // Find unlock instruction intra-procedurally
   if(unlockInst->getCalledFunction()->getNameStr() == "pthread_mutex_unlock"){

   Value *unlockVariable = unlockInst->getOperand(1);

   // Pattern should only exist within a pair of lock and unlock instruction
   // with the same lock variable
   if(aa->isAlias(lockVariable, unlockVariable, function, function)
   >= INTER_ALIAS_ACCURACY){

   // Compute the control dependency node of the two instructions
   BasicBlock *lockDependency = cdg->dependences[lockInst->getParent()];
   BasicBlock *unlockDependency = cdg->dependences[unlockInst->getParent()];

   if(lockInst->getParent() == unlockInst->getParent()){ // lock..unlock pattern
   fs->locks[lockVariable]->directLockNumber ++;
   }
   else if( lockDependency == unlockDependency ){  // lock..unlock pattern
   fs->locks[currentLock]->directLockNumber ++;
   }
   else if(lockDependency != unlockDependency){ // if..lock..if..unlock pattern
   fs->locks[currentLock]->ifLockNumber ++;
   }
   else{
   cout<<"haha"<<endl;
   }

   fs->locks[currentLock]->lockUsage ++ ;

   }

   }
   // Find unlock instruction inter-procedurally
   else if(unlockInst->getCalledFunction()->getNameStr() != "pthread_mutex_lock"){
   //cout<<"function name: "<<unlockInst->getCalledFunction()->getNameStr()<<endl;
   if(!ms->functionStatistics[unlockInst->getCalledFunction()]
   ->isLockWrapper()){
   FunctionStatistic *callS = ms->functionStatistics[unlockInst->getCalledFunction()];
   Value *unlockVariable;
   for(map<Value*, LockData*>::iterator iul = callS->locks.begin();
   iul != callS->locks.end(); iul++){
   unlockVariable = (*iul).first;
   // Pattern should only exist within a pair of lock and unlock instruction
   // with the same lock variable
   // Here consider unlock wrapper function only
   if(aa->isAlias(lockVariable, unlockVariable, function, unlockInst->getCalledFunction())
   >= INTER_ALIAS_ACCURACY && callS->locks[unlockVariable]->isUnlockWrapper){

   // Compute the control dependency node of the two instructions
   BasicBlock *lockDependency = cdg->dependences[lockInst->getParent()];
   BasicBlock *unlockDependency = cdg->dependences[unlockInst->getParent()];

   if(lockInst->getParent() == unlockInst->getParent()){ // lock..unlock pattern
   fs->locks[currentLock]->directLockNumber ++;
   }
   else if( lockDependency == unlockDependency ){  // lock..unlock pattern
   fs->locks[currentLock]->directLockNumber ++;
   }
   else if(lockDependency != unlockDependency){ // if..lock..if..unlock pattern
   fs->locks[currentLock]->ifLockNumber ++;
   }
   else{
   cout<<"haha"<<endl;
   }

   fs->locks[currentLock]->lockUsage ++ ;

   }
   }
   }
   }
   }
   }
   */
}

InterExecutor::InterExecutor(string name) {
  applicationName = name;
  currentFunction = NULL;
  statistic = new ModuleStatistic(applicationName);
  if (INTER_USE_ALIAS)
    aliasAnalyzer = new AliasAnalyzer(false);
  else
    aliasAnalyzer = new AliasAnalyzer();
}

InterExecutor::~InterExecutor() {
  // Do nothing here
}

void InterExecutor::initExecutor() {
  // Do nothing here
}

void InterExecutor::run(Module *module) {
  if (INTER_USE_ALIAS) {
#ifdef USE_ALIAS_FILE
    cout<<getLogDirectory().append("alias.log")<<endl;
    aliasAnalyzer->run(getLogDirectory().append("alias.log"));
    aliasAnalyzer->printAliasSets();
#else
    aliasAnalyzer->run(module);
#endif
  }

  for (Module::iterator it = module->begin(), ie = module->end(); it != ie;
      it++) {
    currentFunction = &(*it);
    workList.push_back(currentFunction);
    IntraExecutor intraExecutor(module, currentFunction, statistic,
        aliasAnalyzer);
    intraExecutor.run();
    workList.pop_back();
  }

  // Output the result
  statistic->printStatistic();

}

} /* namespace esp */
