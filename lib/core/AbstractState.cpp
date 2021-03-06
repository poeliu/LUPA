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
#include "../util/Naming.h"
#include "../util/Log.h"

using namespace std;
using namespace esp;

esp::AbstractState::AbstractState(){
  init = true;
  this->lockStates = list<Lock*>();
  this->releases    = list<Lock*>();
}

esp::AbstractState::~AbstractState(){
  this->lockStates.clear();
  this->releases.clear();
}

bool esp::AbstractState::equalsTo(AbstractState &as){
  if (init != as.init)
    return false;
  if (lockStates.size() != as.lockStates.size())
    return false;

  // Find different locks between the own abstract state and pass-in abstract state
  // If one existed, it means the two abstract states are different
  list<Lock*> tmp;
  for (list<Lock*>::iterator it = lockStates.begin();
      it != lockStates.end(); it++) {
    tmp.insert(tmp.end(), *it);
  }

  for (ASIterator it = as.lockStates.begin(); it != as.lockStates.end(); it++) {
    bool flag = true;
    for (ASIterator cit = lockStates.begin(); cit != lockStates.end(); cit++) {
      if ((*it)->name.compare((*cit)->name) == 0){
        flag = false;
        break;
      }
    }
    if (flag)
      tmp.insert(tmp.end(), *it);

  }
  if (tmp.size() != lockStates.size())
    return false;

  tmp.clear();

  // Find different released locks between the own abstract state and
  // pass-in abstract state.
  // If one existed, it means the two abstract states are different
  for (list<Lock*>::iterator it = releases.begin();
      it != releases.end(); it++) {
    tmp.insert(tmp.end(), *it);
  }

  for (ASIterator it = as.releases.begin(); it != as.releases.end(); it++) {
    bool flag = true;
    for (ASIterator cit = releases.begin(); cit != releases.end(); cit++) {
      if ((*it)->name.compare((*cit)->name) == 0){
        flag = false;
        break;
      }
    }
    if (flag)
      tmp.insert(tmp.end(), *it);

  }
  if (tmp.size() != releases.size())
    return false;

  // Everything is the same then return true
  return true;

}

bool esp::AbstractState::addLockState(string _name, string _preCondition){
  printDebugMsg("add lock: "+_name);
  bool result = unionLockState(_name, _preCondition);
  if(lockStates.size() == 0 && releases.size() == 0)
    init = true;
  else
    init = false;
  return result;
}

bool esp::AbstractState::addLockState(Value *value, string preCondition){
  return addLockState(parseName(value), preCondition);
}

bool esp::AbstractState::deleteLockState(string _name, string _preCondition){
  printDebugMsg("delete lock: "+_name);
  bool flag = false;

  // Remove lock (if existed) from lock states
  for (ASIterator cit = lockStates.begin(); cit != lockStates.end(); cit++) {
    if (_name.compare((*cit)->name) == 0) {
      flag = true;
      lockStates.erase(cit);
      break;
    }
  }

  // Add this released lock into releases
  if(!flag){
    Lock *lock               = new Lock(_name);
    lock->preCondition = _preCondition      ;
    releases.insert(releases.end(), lock )     ;
  }

  // Set the safe flag
  if (lockStates.size() == 0 && releases.size() == 0){
    init = true;
  }

  return flag;
}


bool esp::AbstractState::deleteLockState(Value *value, string preCondition){
  return deleteLockState(parseName(value), preCondition);
}

AbstractState* esp::AbstractState::forkAbstractState(){
  AbstractState *ab = new AbstractState();
  ab->init = init;

  for(list<Lock*>::iterator it = lockStates.begin();
      it != lockStates.end(); it++){

    Lock *lock                 = new Lock((*it)->name);
    lock->state               =                    (*it)->state;
    lock->preCondition   =       (*it)->preCondition;
    ab->lockStates.insert( lockStates.end(),  lock );

  }
  return ab;
}

bool esp::AbstractState::unionLockState(string _name, string _preCondition){
  for (std::list<Lock*>::iterator it = releases.begin();
      it != releases.end(); it++) {
    string name = (*it)->name;
    if(name.compare(_name)){
      this->releases.erase(it);
      return true;
    }
  }

  Lock * lock =             new Lock(_name) ;
  lock->setPreCondition( _preCondition   );
  lockStates.insert(lockStates.end(), lock);

  return true;
}

bool esp::AbstractState::unionLockState(Value *value, string _preCondition){
  return unionLockState(parseName(value), _preCondition);
}

bool esp::AbstractState::unionLockState(AbstractState &as){
  this->init = init | as.init;

  bool result = false;
  for (ASIterator it = as.lockStates.begin(); it != as.lockStates.end(); it++) {
    if(unionLockState((*it)->name, (*it)->preCondition))
      result = true;
  }

  for(ASIterator it = as.releases.begin(); it != as.releases.end(); it++){
    if(deleteLockState((*it)->name, (*it)->preCondition))
      result = true;
  }
  return result;
}
