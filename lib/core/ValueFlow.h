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

#ifndef VALUEFLOW_H_
#define VALUEFLOW_H_

#include "llvm/Value.h"
#include "llvm/Instruction.h"
#include "llvm/Instructions.h"
#include "llvm/Module.h"
#include "llvm/Function.h"
#include <set>

using namespace std;
using namespace llvm;

namespace esp{
class Flow{
public:
  Value *src;
  Value *dst;
  CallInst *callSite;

  Flow(Value *_src, Value *_dst, CallInst *_callSite = NULL)
      :src(_src), dst(_dst){
    callSite = _callSite;
  }
  ~Flow(){src = NULL; dst = NULL;}
};

class VFG{
public:
  set<Flow*> flows;
  VFG();
  ~VFG();
  bool addFlow(Value* src, Value* dst, CallInst *callSite = NULL);
  bool buildVFG(Module *module);
  bool buildVFG(Function *function);
  bool buildVFG(Function *function, CallInst *callSite);
  set<Value*> getFlowSrc(Value *dst, CallInst *callSite = NULL);
};
}

#endif /* VALUEFLOW_H_ */
