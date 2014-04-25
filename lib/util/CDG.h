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

#ifndef CDG_H_

#include "llvm/Analysis/PostDominators.h"
#include "llvm/BasicBlock.h"
#include "llvm/Module.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Analysis/PostDominators.h"
#include <map>
#include <stack>

using namespace std;
using namespace llvm;

namespace esp{
class CDG{
public:
  /*
   * Dependent node of each basic block
   * map[a] = b means a is dependent on b
   */
  map<BasicBlock*, BasicBlock*> dependences;

  /*
   * Constructor
   */
  CDG();

  ~CDG();

  /*
   * Build CDG for the given function
   */
  void buildCDG(Function *function);

  /*
   * Print CDG
   */
  void printCDG();

private:
  /*
   *  Stack for depth first searching the CFG
   */
  stack<BasicBlock*> searchingStack;

  /*
   *  Map for depth first searching the CFG
   */
  map<BasicBlock*, bool> searchingFlag;

  /*
   *  Post dominator tree of the given function
   */
  PostDominatorTree dm;

  /*
   *  Solve dependence beginning from the top
   *  node in the searching stack
   */
  void findDependences();

  /*
   *  Find dependence of the given node
   */
  void findDependences(BasicBlock *parent, BasicBlock *child);

};

}

#define CDG_H_




#endif /* CDG_H_ */
