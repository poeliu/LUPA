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

#include "CDG.h"
#include "llvm/ADT/DepthFirstIterator.h"
#include <iostream>

using namespace esp;
using namespace std;

CDG::CDG(){}

CDG::~CDG(){
  dependences.clear();
}

void CDG::buildCDG(Function *function){
  // Clear dependence graph
  dependences.clear();

  // Build post dominator tree
  dm.runOnFunction(*function);

  // The entry basic block of the given function
  BasicBlock *entry = &function->front();

  // Set the dependency of entry to NULL
  dependences[entry] = NULL;

  // Depth first searching the CFG of the given  function
  searchingStack.push(entry);

  for(succ_iterator si = succ_begin(entry);
            si != succ_end(entry) ;  si++){
    searchingStack.push(*si);
    dependences[*si] = entry;
  }

  findDependences();

}

void CDG::findDependences(){

  while(searchingStack.size() != 0){

    BasicBlock *current = searchingStack.top();
    if(searchingFlag.find(current) != searchingFlag.end())
      return;
    else
      searchingFlag[current] = true;
    //cout<<current->getNameStr()<<endl;

    for(succ_iterator si = succ_begin(current);
        si != succ_end(current) ;  si++){
        BasicBlock *successor = *si;

        if(dependences.find(*si) == dependences.end())
          findDependences(current, successor);

        searchingStack.push(successor);
        findDependences();
    }

    searchingStack.pop();
  }
}

void CDG::findDependences(BasicBlock *parent, BasicBlock *child){
  if(child != dm.getNode(parent)->getIDom()->getBlock()){

    dependences[child] = parent;

  }else{

    if(dependences.find(parent) == dependences.end()
        || dependences[parent] == NULL )
      dependences[child] = NULL;

    else{

      parent = dependences[parent];
      findDependences(parent, child);

    }
  }
}

void CDG::printCDG(){
  // Print CDG to std::out
  for(map<BasicBlock*, BasicBlock*>::iterator i = dependences.begin();
      i != dependences.end(); i++)
    if(i->second == NULL)
      cout<<"Dependence of "<<i->first->getNameStr()<<" is "
              <<"entry"<<endl;  // May be wrong
    else
      cout<<"Dependence of "<<i->first->getNameStr()<<" is "
        <<i->second->getNameStr()<<endl;
}
