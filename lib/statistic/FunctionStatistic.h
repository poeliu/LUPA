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

#ifndef FUNCTIONSTATISTIC_H_
#define FUNCTIONSTATISTIC_H_

#include "Statistic.h"
#include "../util/Log.h"
#include "llvm/Type.h"
#include "llvm/Value.h"
#include <iostream>
#include <map>
#include <string>
#include  <set>

using namespace std;
using namespace llvm;

namespace esp{

class LockData : public Statistic{
public:
  /*
   * Statistic data
   */
  uint directLockNumber;                 // Number of lock..unlock patterns
  uint ifLockNumber;                        // Number of if..lock..if..unlock patterns
  uint testLockNumber;                    // Number of if(lock) patterns
  uint otherNumber;                         // Number of other lock patterns
  uint lockUsage;                              // Number of lock usage
  uint callDeep;                                // Call deep of this lock
  bool isLockWrapper;                     // lock wrapper flag
  bool isUnlockWrapper;                 // unlock wrapper flag

  /*
   * Lock Type
   */
  Value *type;

  LockData(){
    directLockNumber = 0;
    ifLockNumber        = 0;
    testLockNumber    = 0;
    otherNumber         = 0;
    lockUsage              = 0;
    callDeep                = 0;
    isLockWrapper       = false;
    isUnlockWrapper   = false;
    type                       = NULL ;
  }

  ~LockData(){}

  void printStatistic();
};

/*
 * Class that stores statistic data
 */
class FunctionStatistic : public Statistic{
public:
  /*
   * Function name
   */
  string functionName;

  /*
   * Statistic data
   */
  uint lockNumber;                           // Number of locks for each function
  uint staticLockNumber;                 // Number of static locks
  uint dynamicLockNumber;            // Number of dynamic locks
  uint globalLockNumber;                // Number of global locks
  uint localLockNumber;                  // Number of local locks
  bool recursiveLock;                      // Is recursive function or not
  map<Value*, LockData*> locks;  // Lock variables and statistic

  /*
   * Interface functions
   */
  FunctionStatistic( string name )   {
    functionName             =  name   ;
    lockNumber                 =           0 ;
    staticLockNumber       =           0 ;
    dynamicLockNumber  =           0 ;
    globalLockNumber      =           0 ;
    localLockNumber        =           0 ;
    recursiveLock              =  false   ;
    locks = map<Value*, LockData*>();

  }

  ~FunctionStatistic(){}

  bool isLockWrapper(){
    for(map<Value*, LockData*>::iterator it = locks.begin();
        it != locks.end(); it++)
      if(!(*it).second->isLockWrapper)
        return false;
    return true;
  }

  bool isUnlockWrapper(){
    for(map<Value*, LockData*>::iterator it = locks.begin();
        it != locks.end(); it++)
      if(!(*it).second->isUnlockWrapper)
        return false;
    return true;
  }

  void printStatistic();
};

}


#endif /* FUNCTIONSTATISTIC_H_ */
