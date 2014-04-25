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

#ifndef CFG_H_
#define CFG_H_

#include "llvm/Value.h"
#include "llvm/Instruction.h"
#include "llvm/Instructions.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/InstIterator.h"
#include <string>
#include <map>
#include <set>

using namespace std;
using namespace llvm;

namespace esp{

/*
 * Global variables declaration
 */
extern std::map<Value*, Value*> parents;        //use and define chains
extern std::map<Value*, std::string> names;     //names of variables
extern std::set<Value*> arguments;              //arguments list

/*
 * Edge that is used in the CFG or data flow graph
 */
class Edge{
public:
  Instruction *src; /* source node of the edge */
  Instruction *dst; /* destination node of the edge */
  Edge(Instruction *_src, Instruction *_dst) : src(_src), dst(_dst){}
  ~Edge(){}
};

/*
 * Determine whether a loop is countered
 * @Params
 * value: instruction where a loop may be countered
 */
bool hasLoop(Value *value);


/*
 * Return true if the given instruction node is a
 * conditional branch node
 */
bool isBranchNode(Instruction *inst);

/*
 * Return true if the given instruction node is a
 * merge node
 */
bool isMergeNode(Instruction *inst);
}


#endif /* CFG_H_ */
