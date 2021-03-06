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

#include "Executor.h"
#include <iostream>
#include <assert.h>

using namespace std;
using namespace llvm;
using namespace esp;

// Define lock function
const int targetNumber = 6;
string targets[] = {
    "pthread_mutex_lock",
    "pthread_mutex_unlock",
    "pthread_mutex_try_lock",
    "pthread_mutex_try_unlock",
    "rw_lock",
    "rw_unlock"
};

bool esp::shouldBeAnalyzed(string name){
  for(int i = 0; i < targetNumber; i++){
    if(targets[i].compare(name) == 0)
      return true;
  }
  return false;
}

bool esp::InstWork::equalsTo(Work *other){
  InstWork *instOther = (InstWork*)other;
  return this->content == instOther->content;
}

void esp::InstWork::printContent(){
  printInstruction(this->content);
}

esp::InstWork::~InstWork(){content = NULL;}

esp::Executor::Executor(string name){
  applicationName =     name;
  worklist                =     set<Instruction*>();
  statistic                =     GlobalStatistic(applicationName);
}

esp::Executor::~Executor(){
  worklist.clear();
}

void esp::Executor::initExecutor(){
  locks.clear();
  inEdges.clear();
  outEdges.clear();
  functionPtrs.clear();
  returnNode = NULL;
  returnSites.clear();
  returnValues.clear();

  for(map<Edge*, set<SymbolicState*>* >::iterator it = infos.begin();
      it != infos.end(); it++){
    set<SymbolicState*> *stateSet = (*it).second;
    for(set<SymbolicState*>::iterator itt = stateSet->begin();
        itt != stateSet->end(); itt++){
      SymbolicState *state = (*itt);
      stateSet->erase(itt);
      delete state;
    }
    infos.erase(it);
   // delete stateSet;
  }
  infos.clear();

  returnNode = ReturnInst::Create(currentFunction->getContext());
  //printInstruction(returnNode);
}

void esp::Executor::run(Module * module){
  /* Handle intra-procedure only
  if(AnalysisMethod == "inter"){
    analyseFunctionPtr(module);
  }
  */
  for (Module::iterator it = module->begin(), ie = module->end(); it != ie; it++) {
    currentFunction = &(*it);
    initExecutor();
    statistic.functionNumber ++;

    printDebugMsg("========Enter function "+currentFunction->getNameStr());

    LockStatistic *lockStatistic = new LockStatistic(currentFunction->getNameStr());
    statistic.lockStatistics.push(lockStatistic);

    buildUDChains(module, currentFunction);

    if(returnValues.size() == 0){        //no lock or unlock inside the function
      printDebugMsg("========Exit function "+currentFunction->getNameStr());
      statistic.lockStatistics.pop();
      continue;
    }else{
      statistic.lockFunctionNumber ++;
    }

    // perform alias analysis
    if(USE_ALIAS){
      aliasAnalyzer = AliasAnalyzer(false);
#ifdef USE_ALIAS_FILE
      cout<<getLogDirectory().append("alias.log")<<endl;
      aliasAnalyzer.run(getLogDirectory().append("alias.log"));
      aliasAnalyzer.printAliasSets();
#else
      aliasAnalyzer.run(module);
#endif
    }

    // find lock pattern
    if(LOCK_PATTERN){
      this->findLockPattern(currentFunction);
    }

    //use worklist algorithm once for each lock
    for(set<Lock*>::iterator lit = locks.begin(); lit != locks.end(); lit++){
      if((*lit) != NULL){
        currentLock = (*lit);
        statistic.getLocal()->lockNames.insert(currentLock->name);
        statistic.getLocal()->callDeep[currentLock->name] = 1;
        statistic.getLocal()->lockNumber ++;
        if(LOCK_PAIR_UP){
          this->initWorklist();
          this->solveWorklist();
        }
      }else{
        printWarningMsg("NULL lock variable!");
      }
      //break; //Debug single lock
    }

    statistic.getLocal()->printStatistic();
    statistic.lockStatistics.pop();
    printDebugMsg("========Exit function "+currentFunction->getNameStr());
    //break; //Debug single function
  }

  statistic.printStatistic();

}

void esp::Executor::analyseInstruction(Instruction *inst){
  unsigned int op = inst->getOpcode();
  switch(op){
  default:{
    // Illegal or unsupported instruction
    return;
  }
  }
}

void esp::Executor::buildUDChains(Module *module, Function *function){
  //Build ud-chains between globals and their user nodes in the given function
  for (Module::global_iterator globals = module->global_begin();
      globals != module->global_end(); globals++) {
    Value *globalv = &*globals;
    for (Value::use_iterator uit = globalv->use_begin();
        uit != globalv->use_end(); uit++) {
      if (isa<Instruction>(*uit))
        if (((Instruction*) *uit)->getParent() != NULL)
          if (((Instruction*) *uit)->getParent()->getParent() == function){
            parents[*uit] = globalv;
            //printDebugMsg("Build ud between: ");
            //printDebugMsg(""+globalv->getNameStr());
            //printDebugMsg("and");
            //printInstruction((Instruction*)(*uit));
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
      //printDebugMsg("Build ud between: ");
      //printDebugMsg(""+argv->getNameStr());
      //printDebugMsg("and");
      //printInstruction((Instruction*)(*uit));
    }
  }
  //Build ud-chains between every instruction and its user nodes
  for (inst_iterator it = inst_begin(function); it != inst_end(function); it++) {
    for (Value::use_iterator uit = (*it).use_begin(); uit != (*it).use_end();
        uit++) {
      if (parents[*uit] == NULL){
        parents[*uit] = &*it;
        printDebugMsg("Build ud between: ");
        //printDebugMsg(""+(*it).getNameStr());
        //printInstruction((Instruction*)(&*it));
        //printDebugMsg("and");
        //printInstruction((Instruction*)(*uit));
      }
    }
    if (isa<CallInst>(&*it)) {
      //printInstruction(&*it);
      CallInst * callinst = dyn_cast<CallInst>(&*it);
      Function *calledfunc = callinst->getCalledFunction();

      //If a call instruction to lock or unlock function then add it to returnValues
      if (calledfunc != NULL
          && shouldBeAnalyzed(calledfunc->getNameStr())) {
        returnValues.insert(&*it);
      }
    }
  }

  //Need not to analysis this function because it is not relative to lock operation
  if (returnValues.size() == 0)
    return;

  //Look for lock variables
  printDebugMsg("Lock list:");
  for (std::set<Instruction*>::iterator it = returnValues.begin();
      it != returnValues.end(); it++) {
    CallInst *callinst = (CallInst*) (*it);
    //The first operand is the name of the function
    string name = esp::parseName(callinst->getOperand(1));
    printDebugMsg("insert lock: "+callinst->getOperand(1)->getNameStr());
    locks.insert(new Lock(name));
    printDebugMsg(name);

    Value *parent = (*it);
    while(true){
      if(parents[parent] == NULL){
        this->statistic.getLocal()->originalLocks.insert(parent);
        break;
      }
      parent = parents[parent];
    }
  }


}

void esp::Executor::analyseFunctionPtr(Module *module){

}

void esp::Executor::setInEdge(Instruction *inst, int index){
  if(index == 0){
    if(this->inEdges.find(inst)!=inEdges.end() &&
        inEdges[inst]->first != NULL)
        return;
    Edge *edge = new Edge(NULL, inst);

    if(inEdges[inst] == NULL)
      inEdges[inst] = new pair<Edge*, Edge*>();

    //Default in edge one is one of the its previous instruction's
    //true out edge
    if (this->outEdges.size() != 0){
      //Default in edge one is one of the its previous instruction's
      //false out edge
      for (std::map<Instruction*, pair<Edge*, Edge*>* >::iterator it = outEdges.begin();
          it != outEdges.end(); it++) {
        Edge *tmp = (*it).second->first;
        if (tmp != NULL) {
          if (tmp->dst == inst) {
            edge = tmp;
            if (edge != inEdges[inst]->first) {
              inEdges[inst]->first = edge;
              return;
            }
          }
        }
      }
    }

    //if out edge is set before
    BasicBlock *bb = inst->getParent();
    pred_iterator pi = pred_begin(bb), pie = pred_end(bb);
    if (pi != pie)
      edge->src = &((*pi)->back());
    else
      edge->src = edge->dst;

    inEdges[inst]->first = edge;

  }else if(index == 1){
    if(this->inEdges.find(inst)!=inEdges.end() &&
        inEdges[inst]->second != NULL)
        return;
    Edge *edge;

    if(inEdges[inst] == NULL)
      inEdges[inst] = new pair<Edge*, Edge*>();
    if (this->outEdges.size() != 0){
      //Default in edge one is one of the its previous instruction's
      //false out edge
      for (std::map<Instruction*, pair<Edge*, Edge*>* >::iterator it = outEdges.begin();
          it != outEdges.end(); it++) {
        Edge *tmp = (*it).second->second;
        if (tmp != NULL) {
          if (tmp->dst == inst) {
            edge = tmp;
            if (edge != inEdges[inst]->first){
              inEdges[inst]->second = edge;
              return;
            }
          }
        }
      }
    }

    //in edge two is the same with other out true edge
    if (this->outEdges.size() != 0)
      for (std::map<Instruction*, pair<Edge*, Edge*>* >::iterator it = outEdges.begin();
          it != outEdges.end(); it++) {
        if (((*it).second)->first->dst == inst) {
          edge = (*it).second->first;
          if (edge != inEdges[inst]->first)
            return;
        }
      }

    BasicBlock *bb = inst->getParent();
    pred_iterator pi = pred_begin(bb), pe = pred_end(bb);
    edge = new Edge(NULL, inst);

    while (pi != pe) {
      Instruction* tmp = &((*(pi++))->back());
      if (tmp != inEdges[inst]->first->src) {
        edge->src = tmp;
        break;
      }
    }
    if (edge->src == NULL)
      edge->src = edge->dst;

    inEdges[inst]->second = edge;
  }
}

void esp::Executor::setOutEdge(Instruction *inst, bool selectedPath){
  if(selectedPath){
    if(this->outEdges.find(inst)!=outEdges.end() &&
        outEdges[inst] != NULL &&
        outEdges[inst]->first != NULL)
        return;
  }else{
    if(this->outEdges.find(inst)!=outEdges.end() &&
        outEdges[inst] != NULL &&
        outEdges[inst]->second != NULL)
        return;
  }
  Edge *e = new Edge(NULL, NULL);
  if (isa<TerminatorInst>(inst)) {
    //TerminatorInst * ti = dyn_cast <TerminatorInst > (inst);
    if (isa<BranchInst>(inst)) {
      BranchInst * br = dyn_cast <BranchInst > (inst);
      if (br->isConditional()) {
        e->dst = &*(((BasicBlock*) br->getOperand(selectedPath==true?2:1))->begin());
      } else {
        e->dst = &*(((BasicBlock*) br->getOperand(0))->begin());
      }
    } else {
      if (isa<ReturnInst > (inst)) {
        returnSites.insert(inst);
        e->dst = returnNode;
      }

      if (isa<SwitchInst>(inst)) {
        SwitchInst * sw = dyn_cast<SwitchInst> (inst);
        e->dst = &*((sw->getSuccessor(0))->begin());
      }

      if (isa<UnreachableInst > (inst)) {
        returnSites.insert((Instruction*) inst);
        e->dst = returnNode;
      }

    }
  } else {
    e->dst = NULL;
    BasicBlock *bb = inst->getParent();
    for (BasicBlock::iterator it = bb->begin(); it != bb->end(); it++) {
      if (&*it == inst)
        e->dst = &*(++it);
    }
  }

  if(outEdges[inst] == NULL)
    outEdges[inst] = new pair<Edge*, Edge*>();

  if(selectedPath){
    outEdges[inst]->first = e;
  }else{
    outEdges[inst]->second = e;
  }

}

Edge* esp::Executor::getOutEdge(Instruction *inst, bool selectedPath){
  if(selectedPath){
    if(this->outEdges.find(inst)!=outEdges.end()){
      if(outEdges[inst] == NULL)
        printDebugMsg("outEdges[inst] is NULL");
      else if(outEdges[inst]->first != NULL)
        return outEdges[inst]->first;
      else
        printDebugMsg("outEdges[inst]->first is NULL");
    }
  }else{
    if(this->outEdges.find(inst)!=outEdges.end()){
      if(outEdges[inst] == NULL)
        printDebugMsg("outEdges[inst] is NULL");
      else if(outEdges[inst]->second != NULL)
        return outEdges[inst]->second;
      else
        printDebugMsg("outEdges[inst]->second is NULL");
    }
  }
  return NULL;
}

Edge* esp::Executor::getInEdge(Instruction *inst, int index){
  if(index == 0){
    if(this->inEdges.find(inst)!=inEdges.end())
        return inEdges[inst]->first;
  }else if(index == 1){
    if(this->inEdges.find(inst)!=inEdges.end())
        return inEdges[inst]->second;
  }
  return NULL;
}

set<SymbolicState*>* esp::Executor::grouping(set<SymbolicState*>* states){

  //Do nothing if only one symbolic state
  if (states->size() <= 1) {
    return states;
  }

  //Returned variable
  set<SymbolicState*> *result = new set<SymbolicState*>();

  //Use a temporary map to group lock states
  std::map<Lock*, std::set<SymbolicState *>*> map;
  //Use a temporary set to group symbolic states
  std::set<SymbolicState *> init_set;

  for (std::set<SymbolicState*>::iterator it = states->begin();
      it != states->end(); it++) {
    SymbolicState *s = *it;
    if (s->as.init)
      init_set.insert(s);
    for (AbstractState::ASIterator ait = s->as.lockStates.begin();
        ait != s->as.lockStates.end(); ait++) {
      std::set<SymbolicState*> *set = map[*ait];
      if (set == NULL) {
        set = new std::set<SymbolicState*>();
      }
      set->insert(s);
      map[*ait] = set;
    }
  }

  SymbolicState *initss = new SymbolicState();
  std::set<std::string> trueSet;    //temporary set to store true conditions
  std::set<std::string> falseSet;   //temporary set to store false conditions
  std::map<std::string, std::string> switchSet; //temporary set to store switch conditions
  bool allFlag = false;
  for (std::set<SymbolicState *>::iterator it =
      init_set.begin();
      it != init_set.end(); it++) {
    SymbolicState *tmpss = *it;
    if (tmpss->es.all == true) {
      allFlag = true;
      break;
    }
    for (ExecutionState::ESIterator eit = tmpss->es.constraints.begin();
        eit != tmpss->es.constraints.end(); eit++) {
      std::string str = (*eit).first;
      bool value = (*eit).second;
      size_t prefix = std::string::npos;
      if ((prefix = str.find("==")) != std::string::npos) {
        //switch conditions handling
        std::string condition = str.substr(0, prefix);
        std::string val = str.substr(prefix + 2, str.size() - prefix - 2);
        if (switchSet.find(condition) == switchSet.end())
          switchSet[condition] = val;
        else if (switchSet[condition].compare(val) != 0)
          switchSet[condition] = std::string("");
      } else {
        if (value) {
          //if (!contains(&trueSet,str))
          trueSet.insert(str);
        } else {
          //if (!contains(&falseSet,str))
          falseSet.insert(str);
        }
      }
    }
  }
  if (allFlag) {
    initss->es.all = true;
  } else {
    //Union constraints here
    if (trueSet.size() > falseSet.size()) {
      for (std::set<std::string>::iterator it = trueSet.begin();
          it != trueSet.end(); it++) {
        initss->es.unionConstraint(*it, true);
      }
      for (std::set<std::string>::iterator it = falseSet.begin();
          it != falseSet.end(); it++) {
        initss->es.unionConstraint(*it, false);
      }
    } else {
      for (std::set<std::string>::iterator it = falseSet.begin();
          it != falseSet.end(); it++) {
        initss->es.unionConstraint(*it, false);
      }
      for (std::set<std::string>::iterator it = trueSet.begin();
          it != trueSet.end(); it++) {
        initss->es.unionConstraint(*it, true);
      }
    }
    //switch states
    for (std::map<std::string, std::string>::iterator it = switchSet.begin();
        it != switchSet.end(); it++) {
      if (((*it).second).size() != 0) {
        std::string tmp = (*it).first + std::string("==") + (*it).second;
        initss->es.unionConstraint(tmp, true);
      }
    }
  }
  if (init_set.size() != 0)
    result->insert(initss);
  else
    delete initss;

  for (std::map<Lock*, std::set<SymbolicState *>*>::iterator it =
      map.begin(); it != map.end(); it++) {
    Lock* v = (*it).first;
    std::set<SymbolicState*> *set = (*it).second;
    SymbolicState *vs = new SymbolicState();
    std::set<std::string> trueSet;
    std::set<std::string> falseSet;
    std::map<std::string, std::string> switchSet;
    vs->as.addLockState(v->name, vs->es.lastCondition);
    for (std::set<SymbolicState*>::iterator sit = set->begin();
        sit != set->end(); sit++) {
      SymbolicState *tmpss = *sit;
      if (tmpss->es.all == true) {
        allFlag = true;
        break;
      }
      for (ExecutionState::ESIterator eit = tmpss->es.constraints.begin();
          eit != tmpss->es.constraints.end(); eit++) {
        std::string str = (*eit).first;
        bool value = (*eit).second;
        size_t prefix = std::string::npos;
        if ((prefix = str.find("==")) != std::string::npos) {
          //switch conditions handling
          /*FIXME
           std::string condition = str.substr(0, prefix);
           std::string val = str.substr(prefix + 2, str.size() - prefix - 2);
           if (switchSet.find(condition) == switchSet.end())
           switchSet[condition] = val;
           else
           if (switchSet[condition].compare(val) != 0)
           switchSet[condition] = std::string("");
           */
        } else {

          if (value) {
            trueSet.insert(str);
          } else {
            falseSet.insert(str);
          }

        }
      }
    }

    if (allFlag) {
      vs->es.all = true;
    } else {
      if (trueSet.size() > falseSet.size()) {
        for (std::set<std::string>::iterator it = trueSet.begin();
            it != trueSet.end(); it++) {
          vs->es.unionConstraint(*it, true);
        }
        for (std::set<std::string>::iterator it = falseSet.begin();
            it != falseSet.end(); it++) {
          vs->es.unionConstraint(*it, false);
        }
      } else {
        for (std::set<std::string>::iterator it = falseSet.begin();
            it != falseSet.end(); it++) {
          vs->es.unionConstraint(*it, false);
        }
        for (std::set<std::string>::iterator it = trueSet.begin();
            it != trueSet.end(); it++) {
          vs->es.unionConstraint(*it, true);
        }
      }
      /*FIXME Hack switch instruction here
       //switch states
       for (std::map<std::string, std::string>::iterator it = switchSet.begin(); it != switchSet.end(); it++) {
       if (((*it).second).size() != 0) {
       std::string tmp = (*it).first + std::string("==") + (*it).second;
       vs->es.unionConstraint(tmp, true);
       }
       }
       */
    }

    result->insert(vs);
  }
  return result;
}

set<SymbolicState*>* esp::Executor::flowMerge(Instruction *inst, map<Edge*, set<SymbolicState*>* >* infos,
                                         Lock *lock){
  if(inst==NULL){
    printWarningMsg("NULL instruction in function \'flowMerge\'");
    return NULL;
  }
  if(infos->size()==0){
    printWarningMsg("No need to merge flow");
    return NULL;
  }

  set<SymbolicState*> *mergedStates = new set<SymbolicState*>();
  set< set<SymbolicState*>* > stateSets;

  BasicBlock *bb = inst->getParent();
  //look for in-edges
  if(bb){
    for (pred_iterator pi = pred_begin(bb), pe = pred_end(bb); pi != pe; pi++) {
      Instruction *i = &(*pi)->back();
      Edge *edge = NULL;
      if(i!=NULL && this->outEdges[i]!=NULL){
        if(outEdges[i]->first!=NULL &&
           outEdges[i]->first->dst == inst)
          edge = outEdges[i]->first;
        else if(outEdges[i]->second!=NULL &&
                outEdges[i]->second->dst == inst)
          edge = outEdges[i]->second;
      }
      if(edge != NULL){
        stateSets.insert(infos->at(edge));
      }
    }
  }else{
  //look for in-edges of the beginning node in the function
    for (set<Instruction*>::iterator it = returnSites.begin(); it != returnSites.end(); it++) {
      Edge *edge = this->outEdges[*it]->first;
      if (edge != NULL) {
        if(infos->at(edge) != NULL){
          stateSets.insert(infos->at(edge));
        }
      }
    }
  }

  //eliminate duplicate symbolic state
  //FIXME Is the below code unnecessary?
  for(set< set<SymbolicState*>* >::iterator it = stateSets.begin();
      it != stateSets.end(); it++){
    set<SymbolicState*> *stateSet = *it;
    for(set<SymbolicState*>::iterator itt = stateSet->begin();
        itt != stateSet->end(); itt++){
      SymbolicState *state = *itt;
      bool isDuplicate = false;
      for(set<SymbolicState*>::iterator ittt = stateSet->begin();
          ittt != stateSet->end(); ittt++){
        if(itt != ittt && state->equalsTo(*(*ittt))){
          isDuplicate = true;
          break;
        }
      }
      mergedStates->insert(state);
    }
  }
  return grouping(flowOther(inst, mergedStates, lock));
}

set<SymbolicState*>* esp::Executor::flowBranch(Instruction *inst, set<SymbolicState*>* infos,
                                          bool selectedPath, Lock *lock){
  if(inst==NULL){
    printWarningMsg("NULL instruction in function \'flowBranch\'");
    return NULL;
  }
  if(infos->size()==0){
    printWarningMsg("No need to handle branch flow");
    return NULL;
  }

  set<SymbolicState*> *states = new set<SymbolicState*>();
  //copy symbolic state for the selected path
  //path condition will be updated later
  for(set<SymbolicState*>::iterator it = infos->begin();
      it != infos->end(); it++)
    states->insert((*it)->forkSymbolicState());

  BranchInst *bi = dyn_cast<BranchInst>(inst);
  CmpInst *ci = dyn_cast<CmpInst>(bi->getOperand(0));

  //update path condition by updating execution state
  string name;
  Value *left  = ci->getOperand(0);
  Value *right = ci->getOperand(1);
  //FIXME how to handle the case that 2 operands are variable
  if(isa<Constant>(left)){
    name = parseName(right);
  }else if(isa<Constant>(right)){
    name = parseName(left) ;
  }else printWarningMsg("All of the operands are variable");
  for(set<SymbolicState*>::iterator it = states->begin();
      it != states->end(); it++){
    switch(ci->getPredicate()){
    case CmpInst::ICMP_EQ :
      if(isa<Constant>(left) && dyn_cast<Constant>(left)->isNullValue())
        selectedPath = !selectedPath;
      else if(isa<Constant>(right) && dyn_cast<Constant>(right)->isNullValue())
        selectedPath = !selectedPath;
      (*it)->es.updateConstraint(name, selectedPath);
      break;
    case CmpInst::ICMP_NE :
      if(isa<Constant>(left) && !dyn_cast<Constant>(left)->isNullValue())
        selectedPath = !selectedPath;
      else if(isa<Constant>(right) && !dyn_cast<Constant>(right)->isNullValue())
        selectedPath = !selectedPath;
      (*it)->es.updateConstraint(name, selectedPath);
      break;
    default :
      printWarningMsg("Other predicate...");
      break;
    }
  }

  return grouping(states);
}

set<SymbolicState*>* esp::Executor::flowOther(Instruction *inst, set<SymbolicState*>* infos,
                                          Lock *lock){
  if(inst==NULL){
    printWarningMsg("NULL instruction in function \'flowBranch\'");
    return NULL;
  }
  if(infos == NULL){
    printWarningMsg("NULL state set");
    return NULL;
  }
  if(infos->size()==0){
    printWarningMsg("No need to handle branch flow");
    return NULL;
  }

  //Firstly check whether the instruction is call to lock or unlock
  //If so, increase or decrease the lock's state assuming that
  //all of the lock variables are not involved in store operation
  //and all of the lock and unlock operation are successful
  if(isa<CallInst>(inst)){
    CallInst *ci = dyn_cast<CallInst>(inst);
    Function *callee = ci->getCalledFunction();
    if(callee){
      if(callee->getNameStr().compare("pthread_mutex_lock") == 0){
        Value *lockVariable = ci->getOperand(1);
        printDebugMsg(esp::parseName(lockVariable));    //For debug
        if(lock->name.compare(esp::parseName(lockVariable)) == 0){
          for(set<SymbolicState*>::iterator it = infos->begin();
              it != infos->end(); it++){
            (*it)->as.addLockState(lockVariable, (*it)->es.lastCondition);
          }
        }
      }else if(callee->getNameStr().compare("pthread_mutex_unlock") == 0){
        Value *lockVariable = ci->getOperand(1);
        printDebugMsg(esp::parseName(lockVariable));  //For debug
        if(lock->name.compare(esp::parseName(lockVariable)) == 0){
          for(set<SymbolicState*>::iterator it = infos->begin();
              it != infos->end(); it++){
            if((*it)->as.deleteLockState(lockVariable, (*it)->es.lastCondition))
              printErrorMsg(UNINIT_UNLOCK,"");
          }
        }
      }
    }
  }
  return grouping(infos);
}

set<SymbolicState*>* Executor::getStates(Edge* edge){
  if(edge == NULL)
    return NULL;

  if(infos[edge] != NULL)
    return infos[edge];

  //In some cases we have to find states by comparing edges' content
  for(map<Edge*, set<SymbolicState*>* >::iterator it = infos.begin();
      it != infos.end(); it++){
    Edge *tmp = (*it).first;
    if(tmp->src == edge->src && tmp->dst == edge->dst)
      return (*it).second;
  }

  //Return NULL because those states don't exist
  return NULL;
}

void esp::Executor::addToWorklist(Edge *e, std::set<SymbolicState *> *ss,
    std::map<Edge*, std::set<SymbolicState*> *> &Info){
  bool equal = true;
  std::map<Edge*, std::set<SymbolicState*> *>::iterator res = Info.find(e);
  std::set<SymbolicState *> *Infoe = Info[e];
  if (Infoe == NULL) equal = false;
  if (equal && Infoe->size() != ss->size()) equal = false;
  if (!equal) {
    Info[e] = ss;
    //worklist.insert(e->dst);
    Worklist::addToWorklist(new InstWork(e->dst));
    return;
  }
  if (Infoe == ss)
    return;
  for (std::set<SymbolicState *>::iterator it = ss->begin(); it != ss->end(); it++) {
    bool flag = false;
    for (std::set<SymbolicState *>::iterator cit = Infoe->begin(); cit != Infoe->end(); cit++) {
      SymbolicState &ss = **cit;
      if ((*it)->equalsTo(ss)) {
        flag = true;
        break;
      }
    }
    if (!flag) {
      equal = false;
      break;
    }
  }
  if (!equal) {
    Info[e] = ss;
    //worklist.insert(e->dst);
    Worklist::addToWorklist(new InstWork(e->dst));
    return;
  }
}

//TODO full-of-bug function
void esp::Executor::doEachWork(Work *work){
  if(work == NULL){
    printWarningMsg("NULL work from worklist!");
    return;
  }
  InstWork *instWork = (InstWork*)work;
  Instruction *inst = instWork->content;
  printDebugMsg("******Handle instruction:");
  printInstruction(inst);

  //Reach a "returnNode" and delete it from worklist
  if(inst == returnNode ){
    printDebugMsg("A return node here");
    set<SymbolicState*>* states = this->flowMerge(inst, &infos, currentLock);
    Edge *edgeOut = this->getOutEdge(inst, true);
    if(edgeOut == NULL){
      this->setOutEdge(inst,true);
      edgeOut = this->getOutEdge(inst, true);
    }
    Edge *edgeInT = this->getInEdge(edgeOut->dst, 0);
    if(edgeInT == NULL){
      edgeInT = edgeOut;
      if(inEdges[edgeOut->dst] == NULL){
        inEdges[edgeOut->dst] = new pair<Edge*, Edge*>();
      }
      inEdges[edgeOut->dst]->first = edgeOut;
    }

    //this->addToWorklist(edgeOut, states, infos);

  }else if(isBranchNode(inst)){
    printDebugMsg("A branch node here");
    //Get in edge and if NULL, create one
    Edge *edgeIn = getInEdge(inst, 0);
    if(edgeIn == NULL){
      setInEdge(inst, 0);
      edgeIn = getInEdge(inst, 0);
    }

    //Set out edge and in edge of the next node
    Edge *edgeT = this->getOutEdge(inst, true);
    if(edgeT == NULL){
      this->setOutEdge(inst,true);
      edgeT = this->getOutEdge(inst, true);
    }
    Edge *edgeInT = this->getInEdge(edgeT->dst, 0);
    if(edgeInT == NULL){
      edgeInT = edgeT;
      if(inEdges[edgeT->dst] == NULL){
        inEdges[edgeT->dst] = new pair<Edge*, Edge*>();
      }
      inEdges[edgeT->dst]->first = edgeT;
    }
    set<SymbolicState*>* statesT = this->flowBranch(inst, infos[edgeIn], true, currentLock);

    Edge *edgeF = this->getOutEdge(inst, false);
    if(edgeF == NULL){
      this->setOutEdge(inst,false);
      edgeF = this->getOutEdge(inst, false);
    }
    Edge *edgeInF = this->getInEdge(edgeF->dst, 0);
    if(edgeInF == NULL){
      edgeInF = edgeF;
      if(inEdges[edgeF->dst] == NULL){
        inEdges[edgeF->dst] = new pair<Edge*, Edge*>();
      }
      inEdges[edgeF->dst]->first = edgeF;
    }
    set<SymbolicState*>* statesF = this->flowBranch(inst, infos[edgeIn], false, currentLock);

    //Add next instruction to worklist
    this->addToWorklist(edgeT,statesT,infos);
    this->addToWorklist(edgeF,statesF,infos);

  }else if(isMergeNode(inst)){
    printDebugMsg("A merge node here");
    set<SymbolicState*>* states = this->flowMerge(inst, &infos, currentLock);
    Edge *edgeOut = this->getOutEdge(inst, true);
    if(edgeOut == NULL){
      this->setOutEdge(inst,true);
      edgeOut = this->getOutEdge(inst, true);
    }
    Edge *edgeInT = this->getInEdge(edgeOut->dst, 0);
    if(edgeInT == NULL){
      edgeInT = edgeOut;
      if(inEdges[edgeOut->dst] == NULL){
        inEdges[edgeOut->dst] = new pair<Edge*, Edge*>();
      }
      inEdges[edgeOut->dst]->first = edgeOut;
    }

    this->addToWorklist(edgeOut, states, infos);

  }else if(!isBranchNode(inst) && !isMergeNode(inst)){
    printDebugMsg("Other node here");
    //Get in edge and if NULL, create one
    Edge *edgeIn = getInEdge(inst, 0);
    if(edgeIn == NULL){
      setInEdge(inst, 0);
      edgeIn = getInEdge(inst, 0);
    }
    set<SymbolicState*>* states = infos[edgeIn];
    if(states == NULL)
      printDebugMsg("Null state set of the edge");
    states = this->flowOther(inst, states, currentLock);

    //Set out edge and in edge of the next node
    Edge *edgeT = this->getOutEdge(inst, true);
    if(edgeT == NULL){
      this->setOutEdge(inst,true);
      edgeT = this->getOutEdge(inst, true);
    }
    Edge *edgeInT = this->getInEdge(edgeT->dst, 0);
    if(edgeInT == NULL){
      edgeInT = edgeT;
      if(inEdges[edgeT->dst] == NULL){
        inEdges[edgeT->dst] = new pair<Edge*, Edge*>();
      }
      inEdges[edgeT->dst]->first = edgeT;
    }

    this->addToWorklist(edgeT, states, infos);
  }
  //left some code to handle special instructions such as
  //switch, select, phi and so on
}

void esp::Executor::initWorklist(){
  this->works.clear();

  Edge *exitEdge = new Edge(NULL, NULL);
  exitEdge->src = returnNode;
  exitEdge->dst = returnNode;
  outEdges[returnNode] = new pair<Edge*, Edge*>();
  outEdges[returnNode]->first = exitEdge;
  outEdges[returnNode]->second = NULL;

  SymbolicState *firstState = new SymbolicState();
  firstState->as.init = true;
  firstState->es.none = true;

  for(map<Edge*, set<SymbolicState*>* >::iterator it = infos.begin();
      it != infos.end(); it++){
    set<SymbolicState*> *stateSet = (*it).second;
    for(set<SymbolicState*>::iterator itt = stateSet->begin();
        itt != stateSet->end(); itt++){
      //SymbolicState *state = (*itt);
      stateSet->erase(itt);
      //delete state;
    }
    infos.erase(it);
    //delete stateSet;
  }

  std::set<SymbolicState *> *states = new set<SymbolicState *>();
  states->insert(firstState);

  //Handle the first instruction
  printDebugMsg("******Handle instruction:");
  printInstruction(&*inst_begin(currentFunction));
  states = this->flowOther(&*inst_begin(currentFunction), states, currentLock);
  if(states == NULL)
    printDebugMsg("NULL state set!");

  //Set initial out edge and in edge
  setOutEdge(&*inst_begin(currentFunction), true);
  Edge *secondEdge = this->getOutEdge(&*inst_begin(currentFunction), true);
  infos[secondEdge] = states;
  std::cout<<"out edge "<<(int)secondEdge<<endl;
  setInEdge (secondEdge->dst, 0);
  std::cout<<"in edge ";
  if(getInEdge(secondEdge->dst, 0) != NULL)
    std::cout<<(int)getInEdge(secondEdge->dst, 0)<<endl;
  else
    std::cout<<"NULL"<<endl;

  //Add the second instruction to worklist
  Worklist::addToWorklist(new InstWork(secondEdge->dst));
}

CDG* esp::Executor::buildCDG(Function *function){
  cout<<"build CDG"<<endl;

  CDG *cdg = new CDG();
  cdg->buildCDG(function);
  cdg->printCDG();

  cout<<"build CDG end"<<endl;

  return cdg;
}

void esp::Executor::findLockPattern(Function *function){
  CDG *cdg = buildCDG(function);
  assert(cdg != NULL);
  cdg->printCDG();

  // Get lock instruction
  set<CallInst*> lockCalls;
  for(set<Instruction*>::iterator it = returnValues.begin();
      it != returnValues.end(); it++){
    CallInst *callInst = dyn_cast<CallInst>(*it);
    if(callInst->getCalledFunction()->getNameStr() == "pthread_mutex_lock"){
      lockCalls.insert(callInst);
    }
  }
  cout<<"lock call size: "<<lockCalls.size()<<endl;

  // Begin to find lock pattern
  for(set<CallInst*>::iterator it = lockCalls.begin();
      it != lockCalls.end(); it++){
    CallInst *lockInst = *it;

    for(set<Instruction*>::iterator itt = returnValues.begin();
        itt != returnValues.end(); itt++){

      CallInst *unlockInst = dyn_cast<CallInst>(*itt);

      // Find unlock instruction
      if(unlockInst->getCalledFunction()->getNameStr() == "pthread_mutex_unlock"){

        Value *lockVariable = lockInst->getOperand(1);
        Value *unlockVariable = unlockInst->getOperand(1);

        // Pattern should only exist within a pair of lock and unlock instruction
        // with the same lock variable
        if(aliasAnalyzer.isAlias(lockVariable, unlockVariable, function, function)
            == ALIAS_ACCURACY){

          // Compute the control dependency node of the two instructions
          BasicBlock *lockDependency = cdg->dependences[lockInst->getParent()];
          BasicBlock *unlockDependency = cdg->dependences[unlockInst->getParent()];

          if(lockInst->getParent() == unlockInst->getParent()){ // lock..unlock pattern
            statistic.getLocal()->directLockNumber ++;
          }
          else if( lockDependency == unlockDependency ){  // lock..unlock pattern
            statistic.getLocal()->directLockNumber ++;
          }
          else if(lockDependency != unlockDependency){ // if..lock..if..unlock pattern
            statistic.getLocal()->ifLockNumber ++;
          }
          else{
            cout<<"haha"<<endl;
          }
        }

      }
    }
  }
}
