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

#include "LockStatistic.h"
#include "llvm/Support/raw_ostream.h"

using namespace std;
using namespace esp;
using namespace llvm;

void LockStatistic::printStatistic(){
  cout<<"function: "<<functionName<<"\n";

  cout<<"--------------------------------------"                  <<"\n";

  // Print out lock number and lock names
  cout<<"*lock number: "<<lockNumber                <<"\n";
  cout<<"*lock names: "                                           <<"\n";
  for(set<string>::iterator it = lockNames.begin();
      it != lockNames.end(); it++)
    cout<<(*it)<<"\n";

  // Print out lock pattern
  cout<<"*special lock pattern:"                               <<"\n";
  cout<<"lock..unlock: "   <<directLockNumber      <<"\n";
  cout<<"if..lock..if..unlock: "<<ifLockNumber        <<"\n";
  cout<<"if(lock): "           <<testLockNumber         <<"\n";
  cout<<"static lock: "      <<staticLockNumber      <<"\n";
  cout<<"dynamic Lock: "<< dynamicLockNumber<<"\n";
  cout<<"global lock: "     <<globalLockNumber     <<"\n";
  cout<<"local lock: "        <<localLockNumber      <<"\n";

  // Print out call deep of each lock
  cout<<"*call deep of each lock"                            <<"\n";
  for(map<string, uint>::iterator it = callDeep.begin();
      it != callDeep.end(); it++)
    cout<<(*it).first<<": "<<(*it).second                  <<"\n";

  // Print out original lock variables
  cout<<"*original lock variables: "                          <<"\n";
  for(set<Value*>::iterator it = originalLocks.begin();
      it != originalLocks.end(); it++){
    if(isa<Instruction>((*it))){
      Instruction *inst = dyn_cast<Instruction>((*it));
      llvm::outs()<<(*inst)                                          <<"\n";
    }else{
      cout<<(*it)->getNameStr()                             <<"\n";
    }
  }

  // End
  cout<<endl;
}


