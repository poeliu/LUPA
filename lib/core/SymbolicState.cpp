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

#include "SymbolicState.h"
#include "../util/Log.h"
#include <iostream>

using namespace std;
using namespace esp;

void esp::Lock::addLock(){
  if(this->state != 0)
    printErrorMsg(DOUBLE_LOCK, "lock :"+name);
  this->state ++;
}

void esp::Lock::deleteLock(){
  if(this->state == 0)
    printErrorMsg(UNINIT_UNLOCK, "lock :"+name);
  else if(this->state < 0)
    printErrorMsg(DOUBLE_UNLOCK, "lock :"+name);
  this->state --;
}

esp::SymbolicState::SymbolicState(){
  as = AbstractState();
  es = ExecutionState();
}

esp::SymbolicState::~SymbolicState(){

}

bool esp::SymbolicState::equalsTo(SymbolicState &ss){
  AbstractState _as = ss.as;
  ExecutionState _es = ss.es;
  bool res = (as.equalsTo(_as) && es.equalsTo(_es));
  return res;
}

bool esp::SymbolicState::unionState(SymbolicState &ss){
  return as.unionLockState(ss.as) | es.unionConstraints(ss.es);
}

SymbolicState* esp::SymbolicState::forkSymbolicState(){
  SymbolicState *ss = new SymbolicState();
  ss->as = *(as.forkAbstractState());
  ss->es = *(es.forkExecutionState());
  return ss;
}
