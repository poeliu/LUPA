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

#include "ValueFlow.h"

using namespace esp;

VFG::VFG(){
  flows = set<Flow*>();
}
VFG::~VFG(){
  flows.clear();
}
bool VFG::addFlow(Value* src, Value* dst, CallInst *callSite){
  for(set<Flow*>::iterator it = flows.begin(); it!=flows.end(); it++){
    Flow *flow = (*it);
    if(flow->src==src && flow->dst==dst && flow->callSite== callSite){
      return false;
    }
  }
  flows.insert(new Flow(src, dst, callSite));
  return true;
}

bool VFG::buildVFG(Module *module){
  //globals flow
  for (Module::global_iterator globals = module->global_begin(); globals != module->global_end(); globals++) {
    Value *globalv = &*globals;
    for (Value::use_iterator uit = globalv->use_begin(); uit != globalv->use_end(); uit++) {
      if (isa<Instruction > (*uit)) {
        addFlow(globalv, dyn_cast<Instruction > (*uit));
      }
    }
  }

  for (Module::iterator I = module->begin(), E = module->end(); I != E; ++I) {
    Function &F = *I;
    buildVFG(&F);
  }

  return true;
}

bool VFG::buildVFG(Function *function){
  // parameters flow
  for (Function::arg_iterator args = function->arg_begin(); args != function->arg_end(); args++) {
    Value *argv = &*args;
    for (Value::use_iterator uit = argv->use_begin(); uit != argv->use_end(); uit++) {
      addFlow(argv, *uit);
    }
  }

  for (Function::iterator it = function->begin(); it != function->end(); it++) {
    BasicBlock *bb = &(*it);
    for(BasicBlock::iterator bit = bb->begin(); bit != bb->end(); bit++){
      Instruction *inst = &(*bit);
      if (isa<CallInst>(inst)) {
        CallInst * call = dyn_cast<CallInst > (inst);
        Function *func = call->getCalledFunction();
        if (func != NULL) {
          // ignore function pointers
          buildVFG(func, call);
        }
      }
    }
  }
  return true;
}

bool VFG::buildVFG(Function *function, CallInst *callSite){
  Value *arg;
  Function::arg_iterator ait = function->arg_begin();
  for (unsigned i = 1; i != callSite->getNumOperands(); i++) {
    arg = &*ait;
    addFlow(callSite->getOperand(i), arg, callSite);
    ait++;
  }
  return true;
}

set<Value*> VFG::getFlowSrc(Value *dst, CallInst *callSite){
  set<Value*> sources;
  for(set<Flow*>::iterator it = flows.begin(); it!=flows.end(); it++){
    Flow *flow = (*it);
    if(flow->dst==dst){
      if(callSite!=NULL && callSite == flow->callSite){
        sources.insert(flow->src);
      }else sources.insert(flow->src);
    }
  }
  return sources;
}
