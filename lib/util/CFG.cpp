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

#include "CFG.h"

using namespace esp;

// global variables definition
map<Value*, Value*> esp::parents = map<Value*, Value*>();
map<Value*, std::string> esp::names = map<Value*, std::string>();
set<Value*> esp::arguments = set<Value*>();

bool esp::hasLoop(Value *value){
  if(parents.find(value)==parents.end())
    return false;
  if(parents[value] == NULL)
    return false;

  Value *parent = parents[value];
  do{
    if(value == parent)
      return true;
    if(parents.find(parent)==parents.end())
      break;
    else
      parent = parents[parent];
  }while(parent != NULL);

  return false;
}

bool esp::isBranchNode(Instruction *inst){
  if(isa<BranchInst>(inst))
    if(dyn_cast<BranchInst>(inst)->isConditional())
      return true;
  return false;
}

bool esp::isMergeNode(Instruction *inst){
  //Merge node is always the first instruction of its basic block
  Instruction *headBB = &*(inst->getParent()->begin());
  Instruction *headFunction = &*inst_begin(inst->getParent()->getParent());
  if(inst != headBB)
    return false;
  if (inst == headFunction)
    return false;
  if(inst->getParent()->getSinglePredecessor() == NULL &&
      !inst->getParent()->hasAddressTaken())
    return true;
  return false;
}
