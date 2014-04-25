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
#include "Expression.h"

using namespace std;
using namespace esp;

esp::ExecutionState::ExecutionState(){
  all = true;
  none = false;
  constraints = map<string, Expression*>();
}

esp::ExecutionState::~ExecutionState(){
  constraints.clear();
}

bool esp::ExecutionState::equalsTo(ExecutionState &es){
  if ( all     !=  es.all      )
    return false;
  if (none  !=  es.none  )
    return false;
  if (all      && es.all      )
    return true;
  if (none  && es.none  )
    return true;
  if (constraints.size() != es.constraints.size())
    return false;

  for (ESIterator it = constraints.begin(); it != constraints.end(); it++) {
    ESIterator match = es.constraints.end();
    for (ESIterator cit = es.constraints.begin(); cit != es.constraints.end(); cit++) {
      if (((*it).first).compare(((*cit).first)) == 0) {
        match = cit;
        break;
      }
    }
    if (match == es.constraints.end())
      return false;
    if ((*match).second != (*it).second)
      return false;
  }
  return true;
}

bool esp::ExecutionState::addConstraint(string _name, bool value){
  none = false;
  all = false;
  bool isNew = true;

  if (constraints.find(_name) != constraints.end()){

    isNew = false;
    constraints[_name]->value = value;

  }else{

  // Add new constraint
  constraints[_name] = new Expression(_name, 1, value);
  lastCondition = _name;

  }

  return isNew;
}

bool esp::ExecutionState::addConstraint(Value *v, bool value){
  return addConstraint(parseName(v), value);
}

bool esp::ExecutionState::deleteConstraint(string _name){
  for (ESIterator it = constraints.begin(); it != constraints.end(); it++) {
    string s = (*it).first;
    if (s.find("==") != std::string::npos)
      s.resize(s.find("=="));
    if (s.compare(_name) == 0) {
      constraints.erase(it);
      return true;
    }
  }

  return false;
}

bool esp::ExecutionState::deleteConstraint(Value *value){
  return deleteConstraint(esp::parseName(value));
}

bool esp::ExecutionState::unionConstraint(string _name, bool value){
  none = false;
  bool isNew = true;
  bool deleted = false;
  ESIterator pos = constraints.end();

  if (constraints.size() != 0)
    for (ESIterator it = constraints.begin(); it != constraints.end(); it++) {
      if (((*it).first).compare(_name) == 0) {
        isNew = false;
        pos = it;
        break;
      }
    }

  if (isNew) {
    if(constraints[_name] != NULL)
      constraints[_name]->value = value;
    else
      constraints[_name] = new Expression(_name, 1, value);

  } else {
    // Merge constraints
    if (pos != constraints.end() && (*pos).second->value != value) {
      constraints.erase(pos);
      deleted = true;
      if (constraints.size() == 0)
        all = true;
    }
  }
  return deleted;
}

bool esp::ExecutionState::unionConstraint(Value *v, bool value){
  return unionConstraint(parseName(v), value);
}

bool esp::ExecutionState::unionConstraints(ExecutionState &es){
  this->all  = all | es.all;
  this->none = none& es.none;

  if(all || none){
    es.constraints.clear();
    return false;
  }

  bool result = false;
  for(ESIterator i = es.constraints.begin(); i != es.constraints.end(); i++){
    if(this->unionConstraint((*i).first, (*i).second))
      result = true;
  }

  if(es.constraints.size() != 0)
    all = false;

  return result;
}

bool esp::ExecutionState::updateConstraint(string name, bool value){
  if (constraints.size() == 0)
    return addConstraint(name, value) | true;

  ESIterator it;
  for (it = constraints.begin(); it != constraints.end(); it++) {
    if (((*it).first).compare(name) == 0) {
      if ((*it).second->value != value) {
        return false;
      }
    }
  }

  //hack for switch conditions
  if (name.find("==") != std::string::npos) {
    for (it = constraints.begin(); it != constraints.end(); it++) {
      if ((*it).first.compare(name) == 0)
        return true;
    }
    //now no existing condition equals given condition
    for (it = constraints.begin(); it != constraints.end(); it++) {
      size_t prefix = 0;
      if ((prefix = ((*it).first).find("==")) != std::string::npos) {
        if (name.substr(0, prefix).compare(((*it).first).substr(0, prefix)) == 0) return false;
      }
    }
  }
  return addConstraint(name, value) | true;

}

bool esp::ExecutionState::hasConstraint(string name){
  for (ESIterator it = constraints.begin(); it != constraints.end(); it++) {
    std::string content = (*it).first;
    if (content == name) return true;
    if (content.compare(name) == 0) return true;
  }
  return false;
}

ExecutionState* esp::ExecutionState::forkExecutionState(){
  ExecutionState *es =      new ExecutionState();
  es->all                    =      all                                ;
  es->none                =      none                            ;
  es->constraints      =      constraints                  ;
  es->lastCondition   =     lastCondition               ;
  return es;
}
