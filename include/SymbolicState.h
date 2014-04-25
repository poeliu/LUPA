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

#ifndef SYMBOLICSTATE_H_
#define SYMBOLICSTATE_H_

#include <string>
#include <set>
#include <list>
#include <map>
#include "Expression.h"
#include "llvm/Value.h"

using namespace std;
using namespace llvm;

namespace esp {

/*
 * Class for property simulation
 */
class Property{
public:
  string name;  /* unique name for each property variable */
  int state;    /* current state of the property          */

  /*
   * Constructor function
   * @Params
   * _name: name of the property
   * _state:init state of the property. Default 0
   */
  Property(string _name, int _state = 0){
    name = _name;
    state = _state;
  }
};

/*
 * Lock property class
 * Warning: state field is NO USE anymore
 */
class Lock : public Property{
public:
  /*
   * Name of the latest condition before lock or unlock
   * operation.
   * This is used to find if..lock..if..unlock pattern.
   * If preConditions of lock and unlock are the same then
   * this is a if..lock..if..unlock pattern
   */
  string preCondition;

  /*
   * Add lock state along with lock state checking
   * when a pthread_mutex_lock function is called
   */
  void addLock();

  /*
   * Decrease lock state along with lock state checking
   * when a pthread_mutex_unlock function is called
   */
  void deleteLock();

  /*
   * Constructor function.
   * After creating a new lock variable its state is initialized to 0
   * @Params
   * _name: name of the lock
   */
  Lock(string _name) : Property(_name){ preCondition = ""; }

  /*
   * Set the name of the preCondition
   * @Params
   * _preCondtion: name of the preCondition
   */
  void  setPreCondition(string _preCondition){
    preCondition = _preCondition;
  }
};

/*
 * Abstract state class
 */
class AbstractState {
public:
  bool init;    /* flag that represents whether all the locks are safe*/

  list<Lock*> lockStates;    /* list that stores all the lock variables*/
  list<Lock*> releases;       /* list that stores all the released lock */

  typedef list<Lock*>::iterator ASIterator;

  /*
   * Constructor
   */
  AbstractState();

  /*
   * Destructor
   */
  ~AbstractState();
  bool equalsTo(AbstractState &as);

  /*
   * Add a new lock or increase the state if existed
   * @Params
   * _name: name of the lock variable
   * _preCondition: preCondition of the lock
   * @Return
   * return true if successful
   */
  bool addLockState(string _name, string _preCondition);

  /*
   * Add a new lock or increase the state if existed
   * @Params
   * value: instruction of the lock variable declaration
   * _preCondition: preCondition of the lock
   * @Return
   * return true if successful
   */
  bool addLockState(Value *value, string _preCondition);

  /*
   * Delete a lock or decrease the state if existed
   * @Params
   * _name: name of the lock variable
   * _preCondition: preCondition of the lock
   * @Return
   * return true if successful
   */
  bool deleteLockState(string _name, string _preCondition);

  /*
   * Delete a lock or decrease the state if existed
   * @Params
   * value: instruction of the lock variable declaration
   * _preCondition: preCondition of the lock
   * @Return
   * return true if successful
   */
  bool deleteLockState(Value *value, string _preCondition);

  /*
   * union with other lock variable
   * @Params
   * _name: name of the other lock variable
   * _preCondition: preCondition of the lock
   * @Return
   * return true if successful
   */
  bool unionLockState(string _name, string _preCondition);

  /*
   * union with other lock variable
   * @Params
   * value: Value variable of the other lock variable
   * _preCondition: preCondition of the lock
   * @Return
   * return true if successful
   */
  bool unionLockState(Value *value, string _preCondition);

  /*
   * union lock states with other abstract state
   * @Params
   * as: other abstract state
   * return true if successful
   */
  bool unionLockState(AbstractState &as);

  /*
   * Copy the abstract state with the same content or not
   */
  AbstractState* forkAbstractState();
};

class ExecutionState {
public:
  bool all;
  bool none;

  map<string, Expression*> constraints;
  typedef std::map<std::string, Expression*>::iterator ESIterator;

  /*
   * Last encountered condition.
   * It is used to detect if..lock..if..unlock pattern
   */
  string lastCondition;

  /*
   * Constructor
   */
  ExecutionState();

  /*
   * Destructor
   */
  ~ExecutionState();
  bool equalsTo(ExecutionState &es);

  /*
   * Add new constraint
   * @Params
   * _name: name of the constraint.
   *    It is important if using boolean expression only
   * value: value of the constraint.
   *    Only two value is legal:true or false
   * @Return
   * return true if successful
   */
  bool addConstraint(string _name, bool value);
  bool addConstraint(Value *v, bool value);

  /*
   * Delete new constraint
   * @Params
   * _name: name of the constraint.
   *    It is important if using boolean expression only
   * @Return
   * return true if successful
   */
  bool deleteConstraint(string _name);
  bool deleteConstraint(Value *value);

  /*
   * Union with other constraint
   * @Params
   * _name: name of the constraint.
   *    It is important if using boolean expression only
   * value: value of the constraint.
   *    Only two value is legal:true or false
   * @Return
   * return true if successful
   */
  bool unionConstraint(string _name, bool value);
  bool unionConstraint(Value *v, bool value);
  bool unionConstraints(ExecutionState &es);

  /*
   * Update constraint's value
   * @Params
   * _name: name of the constraint.
   *    It is important if using boolean expression only
   * value: value of the constraint.
   *    Only two value is legal:true or false
   * @Return
   * return true if successful
   */
  bool updateConstraint(string name, bool value);

  /*
   * Find the given constraint
   * @Params
   * _name: name of the constraint.
   * @Return
   * return true if existed
   */
  bool hasConstraint(string name);

  ExecutionState* forkExecutionState();
};

/*
 * Symbolic state class
 */
class SymbolicState {
public:
  AbstractState as;
  ExecutionState es;

  /*
   * Constructor
   */
  SymbolicState();

  /*
   * Destructor
   */
  ~SymbolicState();

  bool equalsTo(SymbolicState &ss);

  /*
   * Union with other symbolic state
   * @Param
   * ss: other symbolic state
   * @Return
   * return true if successful
   */
  bool unionState(SymbolicState &ss);
  SymbolicState* forkSymbolicState();
};

}
#endif /* SYMBOLICSTATE_H_ */
