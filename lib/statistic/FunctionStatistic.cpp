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

#include "FunctionStatistic.h"
#include "llvm/Support/raw_ostream.h"

using namespace std;
using namespace esp;
using namespace llvm;

void LockData::printStatistic(){
  if(isLockWrapper)
    cout<<"is lock wrapper"                                       <<"\n";
  if(isUnlockWrapper)
    cout<<"is unlock wrapper"                                   <<"\n";

  // Print out lock pattern
  cout<<"*Overall lock usages:"<<lockUsage        <<"\n";
  cout<<"*special lock pattern:"                               <<"\n";
  cout<<"lock..unlock: "   <<directLockNumber      <<"\n";
  cout<<"if..lock..if..unlock: "<<ifLockNumber        <<"\n";
  cout<<"if(lock): "           <<testLockNumber         <<"\n";
  cout<<"other patterns:" <<otherNumber              <<"\n";

  // Print out call deep of each lock
  cout<<"*call deep: "      <<callDeep                      <<"\n";

  // Print out the lock type
  string typeName = type->getType()->getDescription();
  cout<<"*lock type: "      <<typeName                   <<"\n";
}

void FunctionStatistic::printStatistic(){
  cout<<"function: "<<functionName<<"\n";

  cout<<"--------------------------------------"                  <<"\n";

  // Print out lock number and lock names
  for(map<Value*, LockData*>::iterator it = locks.begin();
      it != locks.end(); it++){
    cout<<"*lock number: "<<lockNumber                <<"\n";
    cout<<"*lock names: "                                           <<"\n";
    cout<<getValueName((*it).first)                            <<"\n";
    (*it).second->printStatistic();
  }

  cout<<"static lock: "      <<staticLockNumber      <<"\n";
  cout<<"dynamic Lock: "<< dynamicLockNumber<<"\n";
  cout<<"global lock: "     <<globalLockNumber     <<"\n";
  cout<<"local lock: "        <<localLockNumber      <<"\n";

  if(recursiveLock)
    cout<<"recursive lock"                                         <<"\n";

  // End
  cout<<endl;
}



